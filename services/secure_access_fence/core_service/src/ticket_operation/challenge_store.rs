/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! This module implements the Wrapper of the SAF service.

use std::collections::HashMap;
use std::sync::{Mutex, OnceLock};
use std::time::{SystemTime, UNIX_EPOCH};

use saf_log::{loge, logi};
use saf_definition::ErrCode;
use saf_definition::macros_lib;

/// Maximum number of callers allowed in the store.
const MAX_CALLERS: usize = 256;

/// Maximum number of challenges per caller
const MAX_CHALLENGES_PER_CALLER: usize = 1024;

/// Maximum challenge validity duration in millseconds (60s)
pub const MAX_CHALLENGE_VALIDITY_MS: u64 = 60 * 1000;

/// Storage entry containing the expiration timestamp
struct ChallengeEntry {
    expire_time_ms: u64,
}

/// Per-caller challenge bucket
type CallerChallenges = HashMap<String, ChallengeEntry>;

/// Thread-safe in-memory challenge store
pub struct ChallengeStore {
    inner: Mutex<HashMap<String, CallerChallenges>>,
}

impl ChallengeStore {
    /// Creates a new empty ChallengeStore.
    pub fn new() -> Self {
        Self { inner: Mutex::new(HashMap::new()) }
    }

    /// Returns current boot time in milliseconds bia C++ TimeWrapper.
    /// Returns None if GetBootTimeMs fails.
    fn now_ms() -> Option<u64> {
        let boot_ms = crate::wrapper::ffi::GetBootTimeMs();
        if boot_ms < 0 {
            loge!("[ChallengeStore] GetBootTimeMs failed, ret = {}", boot_ms);
            return None;
        }
        Some(boot_ms as u64)
    }

    /// Stores a challenge for a specific caller with its expiration time.
    pub fn insert(&self, caller_token_id: &str, challenge: &str, expire_time_ms: u64) -> saf_definition::Result<()> {
        let mut map = self.inner.lock().unwrap();

        if let Some(bucket) = map.get_mut(caller_token_id) {
            // Existing caller: check bucket capacity
            if bucket.contains_key(challenge) {
                loge!("[ChallengeStore] challenge already exists for caller = {}, possible collision",
                    caller_token_id);
                return macros_lib::log_throw_error!(
                    ErrCode::GeneralError,
                    "challenge already exists in store"
                );
            }
            if bucket.len() >= MAX_CHALLENGES_PER_CALLER {
                Self::clean_caller_expired(bucket, caller_token_id);
                if bucket.len() >= MAX_CHALLENGES_PER_CALLER {
                    loge!("[ChallengeStore] caller={} bucket is full after cleanup, size = {}",
                        caller_token_id, bucket.len());
                    return macros_lib::log_throw_error!(
                        ErrCode::ChallengeLimitExceeded,
                        "challenge store is full for this caller"
                    );
                }
            }
            bucket.insert(challenge.to_string(), ChallengeEntry { expire_time_ms });
        } else {
            // New caller: check caller count limit first
            if map.len() >= MAX_CALLERS {
                Self::cleanup_all_expired_locked(&mut map);
                map.retain(|_, bucket| !bucket.is_empty());

                if map.len() >= MAX_CALLERS {
                    loge!("[ChallengeStore] caller count exceeds limit after cleanup, count = {}",
                        map.len());
                    return macros_lib::log_throw_error!(
                        ErrCode::ChallengeLimitExceeded,
                        "challenge store caller count is full"
                    );
                }
            }
            let mut bucket = HashMap::new();
            bucket.insert(challenge.to_string(), ChallengeEntry { expire_time_ms });
            map.insert(caller_token_id.to_string(), bucket);
        }

        logi!("[ChallengeStore] inserted challenge for caller = {}, expire_time_ms = {}",
            caller_token_id, expire_time_ms);
        Ok(())
    }

    /// Checks if a challenge exists, is not expired, and removes it (one-time use).
    /// Returns false if challenge not found, already expired, or caller bucket missing.
    pub fn check_and_remove(&self, caller_token_id: &str, challenge: &str) -> bool {
        let mut map = self.inner.lock().unwrap();
        if let Some(bucket) = map.get_mut(caller_token_id) {
            let Some(now_ms) = Self::now_ms() else {
                loge!("[ChallengeStore] clock error, deny challenge check for caller = {}", caller_token_id);
                return false;
            };
            match bucket.remove_entry(challenge) {
                Some((_, entry)) if entry.expire_time_ms >= now_ms => {
                    logi!("[ChallengeStore] challenge consumed for caller = {}, remaining = {}ms",
                        caller_token_id, entry.expire_time_ms - now_ms);
                    if bucket.is_empty() {
                        map.remove(caller_token_id);
                    }
                    true
                }
                Some((_, entry)) => {
                    loge!("[ChallengeStore] challenge expired for caller = {}, expired {}ms ago",
                        caller_token_id, now_ms - entry.expire_time_ms);
                    if bucket.is_empty() {
                        map.remove(caller_token_id);
                    }
                    false
                }
                None => {
                    loge!("[ChallengeStore] challenge not found for caller = {}, possible replay attack",
                        caller_token_id);
                    false
                }
            }
        } else {
            loge!("[ChallengeStore] no bucket for caller = {}, possible replay attack", caller_token_id);
            false
        }
    }

    /// Cleans up expired entries and returns the remaining time, return 0 if no valid challenge remain.
    pub fn max_remaining_time_ms(&self) -> u64 {
        let mut map = self.inner.lock().unwrap();
        let Some(now_ms) = Self::now_ms() else {
            loge!("[ChallengeStore] system time error, fallback to MAX_CHALLENGE_VALIDITY_MS");
            return MAX_CHALLENGE_VALIDITY_MS;
        }

        // Clean up expired entries first
        for bucket in map.values_mut() {
            bucket.retain(|_, entry| entry.expire_time_ms >= now_ms);
        }
        map.retain(|_, bucket| !bucket.is_empty());

        // Find max remaining time among valid challenges
        map.values()
            .flat_map(|bucket| bucket.values())
            .map(|entry| entry.expire_time_ms.saturating_sub(now_ms))
            .max()
            .unwrap_or(0)
    }

    /// Removes expired entries from a single caller's bucket.
    fn clean_caller_expired(bucket: &mut CallerChallenges, caller_token_id: &str) {
        let Some(now_ms) = Self::now_ms() else { return };
        
        let before = bucket.len();
        bucket.retain(|_, entry| entry.expire_time_ms >= now_ms);
        let removed = before - bucket.len();
        if removed > 0 {
            logi!("[ChallengeStore] cleanup expired for caller = {}, removed = {}", caller_token_id, removed);
        }
    }

    /// Removes expired entries across all callers' buckets
    fn cleanup_all_expired_locked(map: &mut HashMap<String, CallerChallenges>) {
        let Some(now_ms) = Self::now_ms() else { return };

        let mut total_removed = 0usize;
        for (_caller_id, bucket) in map.iter_mut() {
            let before = bucket.len();
            bucket.retain(|_, entry| entry.expire_time_ms >= now_ms);
            total_removed += before - bucket.len();
        }

        if total_removed > 0 {
            logi!("[ChallengeStore] global cleanup expired entries, removed = {}", total_removed);
        }
    }
}

// Global singleton for the challenge store.
pub static CHALLENGE_STORE: OnceLock<ChallengeStore> = OnceLock::new();

/// returns the global ChallengeStore instance, initializing it on first call
pub fn global_challenge_store() -> &'static ChallengeStore {
    CHALLENGE_STORE.get_or_init(ChallengeStore::new)
}
