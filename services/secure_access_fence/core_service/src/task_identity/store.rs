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

//! Task identity store: LRU memory cache + storage backend (persistence).
//! All disk I/O is delegated to StorageBackend trait — zero #[cfg] in this file.

use std::collections::{HashMap, VecDeque};
use std::sync::{Mutex, OnceLock};

use saf_definition::{macros_lib, ErrCode, ExecuteType, Result};
use saf_log::{loge, logi, logw};

use super::storage_backend::{create_storage, StorageBackend};

/// Maximum number of entries kept in each LRU cache (entries, challenges).
const LRU_CAPACITY: usize = 32;
const MS_PER_SEC: u64 = 1000;
const TASK_HARD_DELETE_AFTER_SEC: i64 = 24 * 3600;
const CHALLENGE_TIMEOUT_MS: u64 = 600_000;  // 10 min

/// Simple LRU cache: HashMap for O(1) lookup + VecDeque for access order.
/// front = LRU (evict first), back = MRU (most recently used).
struct LruMap<K: std::hash::Hash + Eq + Clone, V> {
    map: HashMap<K, V>,
    order: VecDeque<K>,
}

impl<K: std::hash::Hash + Eq + Clone, V> LruMap<K, V> {
    fn new() -> Self {
        Self { map: HashMap::new(), order: VecDeque::new() }
    }

    fn contains_key(&self, key: &K) -> bool {
        self.map.contains_key(key)
    }

    fn get(&mut self, key: &K) -> Option<&V> {
        if self.map.contains_key(key) {
            self.touch(key);
            self.map.get(key)
        } else {
            None
        }
    }

    fn put(&mut self, key: K, value: V) {
        if self.map.contains_key(&key) {
            self.map.insert(key.clone(), value);
            self.touch(&key);
            return;
        }
        if self.map.len() >= LRU_CAPACITY {
            if let Some(lru_key) = self.order.pop_front() {
                self.map.remove(&lru_key);
            }
        }
        self.order.push_back(key.clone());
        self.map.insert(key, value);
    }

    fn remove(&mut self, key: &K) -> Option<V> {
        self.order.retain(|k| k != key);
        self.map.remove(key)
    }

    fn touch(&mut self, key: &K) {
        self.order.retain(|k| k != key);
        self.order.push_back(key.clone());
    }
}

#[derive(Debug, Clone)]
pub struct TaskIdentityRecord {
    pub task_attributes: String,
    pub identity: String,
    pub challenge: String,
    pub task_expire_sec: i64,
    pub execute_type: ExecuteType,
    pub allow_screen_lock_execution: bool,
    pub allowable_duration: i32,
}

type StoreKey = (String, String);

pub struct ChallengeEntry {
    pub challenge: String,
    pub created_boot_ms: u64,
}

struct TaskIdentityStoreInner {
    entries: LruMap<StoreKey, TaskIdentityRecord>,
    challenges: LruMap<StoreKey, ChallengeEntry>,
}

/// Thread-safe task identity store.
pub struct TaskIdentityStore {
    inner: Mutex<TaskIdentityStoreInner>,
    storage: Box<dyn StorageBackend>,
}

impl TaskIdentityStore {
    fn new(storage: Box<dyn StorageBackend>) -> Self {
        Self {
            inner: Mutex::new(TaskIdentityStoreInner {
                entries: LruMap::new(),
                challenges: LruMap::new(),
            }),
            storage,
        }
    }

    fn now_sec() -> Result<u64> {
        match saf_utils::system_time_in_millis() {
            Ok(ms) => Ok(ms / MS_PER_SEC),
            Err(e) => {
                logw!("[TaskIdentityStore] get system time failed: {}", e.msg);
                macros_lib::log_throw_error!(ErrCode::GeneralError, "get system time failed")
            }
        }
    }

    fn lock_inner(&self) -> Result<std::sync::MutexGuard<TaskIdentityStoreInner>> {
        self.inner.lock().map_err(|_| {
            loge!("[TaskIdentityStore] inner mutex poisoned");
            macros_lib::SAFError::new(ErrCode::GeneralError, "inner mutex poisoned".to_string())
        })
    }

    fn now_boot_ms() -> u64 {
        let raw = crate::wrapper::ffi::GetBootTimeMs();
        if raw < 0 { 0 } else { raw as u64 }
    }

    fn is_challenge_expired(created_boot_ms: u64, now_boot_ms: u64) -> bool {
        now_boot_ms > created_boot_ms && (now_boot_ms - created_boot_ms) > CHALLENGE_TIMEOUT_MS
    }

    fn is_safe_segment(segment: &str) -> bool {
        super::validator::validate_path_segment(segment).is_ok()
    }

    pub fn insert_challenge(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
        challenge: &str,
    ) -> Result<()> {
        if !Self::is_safe_segment(caller) || !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        let created_boot_ms = Self::now_boot_ms();
        self.storage.write_challenge_file(os_account_id, caller, task_id, challenge, created_boot_ms)?;
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            inner.challenges.put(key, ChallengeEntry {
                challenge: challenge.to_string(),
                created_boot_ms,
            });
        }
        logi!("[TaskIdentityStore] challenge inserted for caller={}, task_id={}", caller, task_id.len());
        Ok(())
    }

    /// Gets and consumes the challenge for (caller, task_id).
    /// The challenge is deleted from memory and disk after being read,
    /// preventing replay attacks.
    pub fn get_challenge(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
    ) -> Result<String> {
        let now_boot_ms = Self::now_boot_ms();
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            if let Some(entry) = inner.challenges.get(&key) {
                let challenge_str = entry.challenge.clone();
                let created_boot_ms = entry.created_boot_ms;
                inner.challenges.remove(&key);
                let _ = self.storage.delete_challenge_file(os_account_id, caller, task_id);
                if Self::is_challenge_expired(created_boot_ms, now_boot_ms) {
                    logw!("[TaskIdentityStore] challenge expired in memory for caller={}, task_id={}", caller, task_id);
                    return macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound,
                        "challenge expired");
                }
                logi!("[TaskIdentityStore] challenge found in memory for caller={}", caller);
                return Ok(challenge_str);
            }
        }
        if let Some((challenge, created_boot_ms)) = self.storage.read_challenge_file(os_account_id, caller, task_id) {
            let _ = self.storage.delete_challenge_file(os_account_id, caller, task_id);
            if Self::is_challenge_expired(created_boot_ms, now_boot_ms) {
                logw!("[TaskIdentityStore] challenge expired on disk for caller={}, task_id={}", caller, task_id);
                return macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound,
                    "challenge expired");
            }
            logi!("[TaskIdentityStore] challenge found on disk for caller={}", caller);
            return Ok(challenge);
        }
        logw!("[TaskIdentityStore] no challenge for caller={}, task_id={}", caller, task_id);
        Err(macros_lib::log_and_into_saf_error!(
            ErrCode::TaskIdentityNotFound, "challenge not found"
        ))
    }

    /// Inserts a task identity record (disk write → LRU memory insert).
    /// If LRU is full (32), evicts the least recently used entry.
    pub fn insert_task_entry(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
        record: TaskIdentityRecord,
    ) -> Result<()> {
        if !Self::is_safe_segment(caller) || !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        self.storage.write_record_file(os_account_id, caller, task_id, &record)?;
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            inner.entries.put(key, record);
        }
        logi!("[TaskIdentityStore] task entry inserted for caller={}, task_id len={}", caller, task_id.len());
        Ok(())
    }

    /// Caches a record from disk into memory (no disk I/O).
    /// Silently skips if key already exists — LRU eviction handled by put.
    fn cache_to_memory(&self, _os_account_id: i32, caller: &str, task_id: &str, record: TaskIdentityRecord) {
        let mut inner = match self.lock_inner() {
            Ok(g) => g,
            Err(_) => return,
        };
        let key = (caller.to_string(), task_id.to_string());
        if inner.entries.contains_key(&key) {
            return;
        }
        inner.entries.put(key, record);
    }

    /// Retrieves a task record by (caller, task_id).
    /// Memory first (LRU touch); disk fallback (caches back to memory on hit).
    pub fn get_task_record(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
    ) -> Result<TaskIdentityRecord> {
        if !Self::is_safe_segment(caller) || !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            if let Some(record) = inner.entries.get(&key) {
                logi!("[TaskIdentityStore] record found in memory for caller={}", caller);
                return Ok(record.clone());
            }
        }
        let disk_record = self.storage.read_record_file(os_account_id, caller, task_id)
            .map_err(|e| {
                logw!("[TaskIdentityStore] disk read failed, err={}", e.code);
                e
            })?;
        if let Some(record) = disk_record {
            self.cache_to_memory(os_account_id, caller, task_id, record.clone());
            logi!("[TaskIdentityStore] record found on disk for caller={}", caller);
            return Ok(record);
        }
        macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound, "task identity not found")
    }

    /// Deletes a task record (memory first → disk second). Does NOT touch challenges.
    /// Idempotent — missing disk file or memory entry is not an error.
    pub fn delete_task_entry(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<()> {
        if !Self::is_safe_segment(caller) || !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            inner.entries.remove(&key);
        }
        self.storage.delete_record_file(os_account_id, caller, task_id)?;
        Ok(())
    }

    /// Cleans expired INSTANT tasks (>1 day) from disk + clears all memory cache.
    /// Intended for SA on_idle. DELAYED tasks on disk are not affected.
    pub fn clean_expired_instant_tasks(&self, os_account_id: i32) {
        // On system time failure, now_sec falls back to 0. This is safe:
        // hard_threshold = 0 - TASK_HARD_DELETE_AFTER_SEC → negative (saturating),
        // so no record satisfies task_expire_sec < hard_threshold → nothing deleted (conservative).
        // Expired records are still rejected by get_task_record's independent time check.
        // The next successful now_sec() call will clean up properly.
        let now_sec = Self::now_sec().unwrap_or(0);
        let now_sec_i64 = i64::try_from(now_sec).unwrap_or(i64::MAX);
        let hard_threshold = now_sec_i64.saturating_sub(TASK_HARD_DELETE_AFTER_SEC);
        let now_boot_ms = Self::now_boot_ms();

        {
            let mut inner = self.inner.lock().unwrap_or_else(|e| {
                logw!("[TaskIdentityStore] mutex poisoned during cleanup, recovering");
                e.into_inner()
            });
            inner.entries = LruMap::new();
            inner.challenges = LruMap::new();
        }

        self.storage.cleanup_disk_expired(os_account_id, hard_threshold, now_boot_ms);
        self.storage.cleanup_file_locks();
        logi!("[TaskIdentityStore] on_idle: memory cache cleared, disk cleanup done for os_account_id={}",
            os_account_id);
    }

    /// Enumerates all os_account_id directories that have task or challenge data on disk.
    pub fn enumerate_os_account_ids(&self) -> Vec<i32> {
        self.storage.enumerate_os_account_ids()
    }
}

// ======================== Global singleton ========================

static TASK_IDENTITY_STORE: OnceLock<TaskIdentityStore> = OnceLock::new();

/// Returns the global TaskIdentityStore instance, initializing it on first call.
pub fn global_task_identity_store() -> &'static TaskIdentityStore {
    TASK_IDENTITY_STORE.get_or_init(|| TaskIdentityStore::new(create_storage()))
}
