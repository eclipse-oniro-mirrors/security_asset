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

//! Task identity store: Vec-based memory cache + storage backend (persistence).
//! tasks use LRU Vec (front=LRU, back=MRU); challenges use FIFO Vec (consume on read).
//! All disk I/O is delegated to StorageBackend trait — zero #[cfg] in this file.

use std::collections::HashSet;
use std::sync::{Mutex, OnceLock};

use saf_definition::{macros_lib, ErrCode, ExecuteType, Result};
use saf_log::{loge, logi, logw};

use super::constants::{VEC_CAPACITY, MAX_FILES_PER_CALLER, MS_PER_SEC};
use super::storage_backend::{create_storage, StorageBackend};

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

// (caller, taskId)
type StoreKey = (String, String);

pub struct ChallengeEntry {
    pub challenge: String,
    pub created_boot_ms: u64,
}

struct TaskIdentityStoreInner {
    // front=LRU (evict), back=MRU (most recently used). get() moves to back.
    tasks: Vec<(StoreKey, TaskIdentityRecord)>,
    // front=oldest, back=newest. get() removes immediately (consume, one-time use).
    challenges: Vec<(StoreKey, ChallengeEntry)>,
}

impl TaskIdentityStoreInner {
    fn new() -> Self {
        Self {
            tasks: Vec::with_capacity(VEC_CAPACITY),
            challenges: Vec::with_capacity(VEC_CAPACITY),
        }
    }

    // ==================== tasks (LRU Vec) ====================

    fn tasks_find(&self, key: &StoreKey) -> Option<usize> {
        self.tasks.iter().position(|(k, _)| k == key)
    }

    fn tasks_get(&mut self, key: &StoreKey) -> Option<&TaskIdentityRecord> {
        if let Some(idx) = self.tasks_find(key) {
            let item = self.tasks.remove(idx);
            self.tasks.push(item);
            self.tasks.last().map(|(_, v)| v)
        } else {
            None
        }
    }

    fn tasks_put(&mut self, key: StoreKey, value: TaskIdentityRecord) {
        if let Some(idx) = self.tasks_find(&key) {
            self.tasks.remove(idx);
        } else if self.tasks.len() >= VEC_CAPACITY {
            self.tasks.remove(0);
        }
        self.tasks.push((key, value));
    }

    fn tasks_remove(&mut self, key: &StoreKey) {
        if let Some(idx) = self.tasks_find(key) {
            self.tasks.remove(idx);
        }
    }

    fn tasks_contains(&self, key: &StoreKey) -> bool {
        self.tasks_find(key).is_some()
    }

    // ==================== challenges (FIFO Vec, consume on read) ====================

    fn challenges_find(&self, key: &StoreKey) -> Option<usize> {
        self.challenges.iter().position(|(k, _)| k == key)
    }

    fn challenges_put(&mut self, key: StoreKey, value: ChallengeEntry) {
        if self.challenges.len() >= VEC_CAPACITY {
            self.challenges.remove(0);
        }
        self.challenges.push((key, value));
    }

    fn challenges_take(&mut self, key: &StoreKey) -> Option<ChallengeEntry> {
        if let Some(idx) = self.challenges_find(key) {
            Some(self.challenges.remove(idx).1)
        } else {
            None
        }
    }
}

/// Thread-safe task identity store.
pub struct TaskIdentityStore {
    inner: Mutex<TaskIdentityStoreInner>,
    storage: Box<dyn StorageBackend>,
}

impl TaskIdentityStore {
    fn new(storage: Box<dyn StorageBackend>) -> Self {
        Self {
            inner: Mutex::new(TaskIdentityStoreInner::new()),
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

    fn is_safe_segment(segment: &str) -> bool {
        super::validator::validate_path_segment(segment).is_ok()
    }

    pub fn insert_challenge(
        &self,
        os_account_id: i32,
        caller: u32,
        task_id: &str,
        challenge: &str,
    ) -> Result<()> {
        record_os_account_id(os_account_id);
        if !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        let file_count = self.storage.count_caller_files(os_account_id, caller)?;
        if file_count >= MAX_FILES_PER_CALLER {
            return macros_lib::log_throw_error!(ErrCode::ChallengeLimitExceeded,
                "caller file count limit reached: {} has {}", caller, file_count);
        }
        let created_boot_ms = Self::now_boot_ms();
        let entry = ChallengeEntry {
            challenge: challenge.to_string(),
            created_boot_ms,
        };
        self.storage.write_challenge_file(os_account_id, caller, task_id, &entry)?;
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            inner.challenges_put(key, entry);
        }
        logi!("[TaskIdentityStore] challenge inserted for caller={}, task_id={}", caller, task_id.len());
        Ok(())
    }

    /// Gets and consumes the challenge for (caller, task_id).
    /// The challenge is deleted from memory and disk after being read.
    pub fn get_challenge(
        &self,
        os_account_id: i32,
        caller: u32,
        task_id: &str,
    ) -> Result<String> {
        record_os_account_id(os_account_id);
        let now_boot_ms = Self::now_boot_ms();
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            if let Some(entry) = inner.challenges_take(&key) {
                let _ = self.storage.delete_challenge_file(os_account_id, caller, task_id);
                if super::storage_backend::is_challenge_expired(entry.created_boot_ms, now_boot_ms) {
                    logw!("[TaskIdentityStore] challenge memory expired for caller={}, task_id={}", caller, task_id);
                    return macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound,
                        "challenge expired");
                }
                logi!("[TaskIdentityStore] challenge found in memory for caller={}", caller);
                return Ok(entry.challenge);
            }
        }
        if let Some((challenge, created_boot_ms)) = self.storage.read_challenge_file(os_account_id, caller, task_id) {
            let _ = self.storage.delete_challenge_file(os_account_id, caller, task_id);
            if super::storage_backend::is_challenge_expired(created_boot_ms, now_boot_ms) {
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

    /// Inserts a task identity record (disk write → LRU Vec insert).
    /// If full (32), evicts the least recently used entry (front).
    pub fn insert_task_entry(
        &self,
        os_account_id: i32,
        caller: u32,
        task_id: &str,
        record: TaskIdentityRecord,
    ) -> Result<()> {
        record_os_account_id(os_account_id);
        if !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        let file_count = self.storage.count_caller_files(os_account_id, caller)?;
        if file_count >= MAX_FILES_PER_CALLER {
            return macros_lib::log_throw_error!(ErrCode::ChallengeLimitExceeded,
                "caller file count limit reached: {} has {}", caller, file_count);
        }
        self.storage.write_record_file(os_account_id, caller, task_id, &record)?;
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            inner.tasks_put(key, record);
        }
        logi!("[TaskIdentityStore] task entry inserted for caller={}, task_id len={}", caller, task_id.len());
        Ok(())
    }

    /// Caches a record from disk into memory (no disk I/O).
    fn cache_to_memory(&self, _os_account_id: i32, caller: u32, task_id: &str, record: TaskIdentityRecord) {
        let mut inner = match self.lock_inner() {
            Ok(g) => g,
            Err(_) => return,
        };
        let key = (caller.to_string(), task_id.to_string());
        if inner.tasks_contains(&key) {
            return;
        }
        inner.tasks_put(key, record);
    }

    /// Retrieves a task record by (caller, task_id).
    /// Memory first (LRU touch); disk fallback (caches back to memory on hit).
    pub fn get_task_record(
        &self,
        os_account_id: i32,
        caller: u32,
        task_id: &str,
    ) -> Result<TaskIdentityRecord> {
        record_os_account_id(os_account_id);
        if !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            if let Some(record) = inner.tasks_get(&key) {
                logi!("[TaskIdentityStore] record found in memory for caller={}", caller);
                return Ok(record.clone());
            }
        }
        let disk_record = self.read_and_cache_record(os_account_id, caller, task_id);
        if let Some(record) = disk_record {
            logi!("[TaskIdentityStore] record found on disk for caller={}", caller);
            return Ok(record);
        }
        macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound, "task identity not found")
    }

    /// Reads a record from disk and caches it in memory. Returns None if not found.
    fn read_and_cache_record(
        &self,
        os_account_id: i32,
        caller: u32,
        task_id: &str,
    ) -> Option<TaskIdentityRecord> {
        let record = self.storage.read_record_file(os_account_id, caller, task_id)?;
        self.cache_to_memory(os_account_id, caller, task_id, record.clone());
        Some(record)
    }

    /// Deletes a task record (disk first → memory second). Does NOT touch challenges.
    /// Idempotent — missing disk file or memory entry is not an error.
    pub fn delete_task_entry(&self, os_account_id: i32, caller: u32, task_id: &str) -> Result<()> {
        record_os_account_id(os_account_id);
        if !Self::is_safe_segment(task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "invalid caller or task id");
        }
        {
            let mut inner = self.lock_inner()?;
            let key = (caller.to_string(), task_id.to_string());
            inner.tasks_remove(&key);
        }
        self.storage.delete_record_file(os_account_id, caller, task_id)?;
        Ok(())
    }

    /// Clears all memory cache (tasks + challenges) and spawns async disk cleanup
    /// for all seen os_account_ids. Intended for SA on_idle.
    /// Returns the number of os account ids that scheduled to clean.
    pub fn on_idle_cleanup(&self) -> usize {
        let mut inner = self.inner.lock().unwrap_or_else(|e| {
            logw!("[TaskIdentityStore] mutex poisoned during cleanup, recovering");
            e.into_inner()
        });
        inner.tasks.clear();
        inner.challenges.clear();
        drop(inner);

        let os_account_ids = take_seen_os_account_ids();
        if os_account_ids.is_empty() {
            logi!("[TaskIdentityStore] on_idle: memory cleared, no os_account_ids to clean");
            return 0;
        }
        let count = os_account_ids.len();
        ylong_runtime::spawn_blocking(move || {
            let store = global_task_identity_store();
            for os_account_id in os_account_ids {
                store.cleanup_disk_files(os_account_id);
            }
            logi!("[TaskIdentityStore] on_idle: disk cleanup done for {} os_account_ids", count);
        });
        logi!("[TaskIdentityStore] on_idle: memory cleared, disk cleanup scheduled for {} os_account_ids", count);
        count
    }

    /// Cleans expired task and challenge files from disk.
    /// Scans all files under taskAttributes/{caller}/, deletes expired .json (tasks) and .challenge files.
    /// Intended for async background cleanup at SA startup.
    pub fn cleanup_disk_files(&self, os_account_id: i32) {
        let now_sec = match Self::now_sec() {
            Ok(s) => s,
            Err(e) => {
                logw!("[TaskIdentityStore] cleanup_disk_files: get time failed, skipping: {}", e.msg);
                return;
            }
        };
        let now_sec_i64 = i64::try_from(now_sec).unwrap_or(i64::MAX);
        let now_boot_ms = Self::now_boot_ms();
        self.storage.cleanup_disk_expired(os_account_id, now_sec_i64, now_boot_ms);
        self.storage.cleanup_file_locks();
        logi!("[TaskIdentityStore] disk cleanup done for os_account_id={}", os_account_id);
    }
}

// ======================== Seen os_account_ids tracking ========================

static SEEN_OS_ACCOUNT_IDS: OnceLock<Mutex<HashSet<i32>>> = OnceLock::new();

fn record_os_account_id(os_account_id: i32) {
    let set = SEEN_OS_ACCOUNT_IDS.get_or_init(|| Mutex::new(HashSet::new()));
    let mut guard = set.lock().unwrap_or_else(|e| e.into_inner());
    guard.insert(os_account_id);
}

fn take_seen_os_account_ids() -> Vec<i32> {
    let Some(set) = SEEN_OS_ACCOUNT_IDS.get() else { return Vec::new() };
    let mut guard = set.lock().unwrap_or_else(|e| e.into_inner());
    let ids: Vec<i32> = guard.iter().copied().collect();
    guard.clear();
    ids
}

// ======================== Global singleton ========================

static TASK_IDENTITY_STORE: OnceLock<TaskIdentityStore> = OnceLock::new();

/// Returns the global TaskIdentityStore instance, initializing it on first call.
pub fn global_task_identity_store() -> &'static TaskIdentityStore {
    TASK_IDENTITY_STORE.get_or_init(|| TaskIdentityStore::new(create_storage()))
}
