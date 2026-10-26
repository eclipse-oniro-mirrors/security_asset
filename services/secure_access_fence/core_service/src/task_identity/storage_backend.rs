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

//! Storage backend trait: abstracts disk I/O so business logic has zero #[cfg].
//! DiskStorage (production) uses real filesystem; MockStorage (tests) is in-memory no-op.

use std::sync::{Arc, Mutex, OnceLock};
use std::collections::HashMap;

use saf_definition::{macros_lib, ErrCode, ExecuteType, Result};
use saf_log::{loge, logi, logw};
use saf_common::JsonBuilder;
use saf_utils::{JsonValue, get_compact_json_value};

use super::store::TaskIdentityRecord;

use super::constants::{
    CHALLENGE_TIMEOUT_MS, RECORD_VERSION, TASK_BASE_DIR_PREFIX, TASK_DIR_SUFFIX,
    FILE_MODE, DIR_MODE,
};

/// Trait that abstracts all disk persistence operations.
/// Business logic in store.rs calls these methods with zero #[cfg].
pub trait StorageBackend: Send + Sync {
    fn write_record_file(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
        record: &TaskIdentityRecord
    ) -> Result<()>;
    fn read_record_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<Option<TaskIdentityRecord>>;
    fn delete_record_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<()>;
    fn write_challenge_file(&self,
        os_account_id: i32,
        caller: &str, task_id: &str,
        challenge_b64: &str,
        created_boot_ms: u64
    ) -> Result<()>;
    fn read_challenge_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Option<(String, u64)>;
    fn delete_challenge_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<()>;
    fn count_caller_files(&self, os_account_id: i32, caller: &str) -> Result<usize>;
    fn cleanup_disk_expired(&self, os_account_id: i32, expire_threshold_sec: i64, now_boot_ms: u64);
    fn cleanup_file_locks(&self);
}

/// Factory: returns DiskStorage in production, MockStorage in tests.
/// This is the ONLY place with #[cfg] — store.rs is completely clean.
pub fn create_storage() -> Box<dyn StorageBackend> {
    #[cfg(not(feature = "SAFTest"))]
    { Box::new(DiskStorage) }
    #[cfg(feature = "SAFTest")]
    { Box::new(MockStorage) }
}

fn is_challenge_expired(created_boot_ms: u64, now_boot_ms: u64) -> bool {
    if now_boot_ms < created_boot_ms {
        return true;
    }
    (now_boot_ms - created_boot_ms) > CHALLENGE_TIMEOUT_MS
}

// ======================== File lock (per-file Mutex) ========================

static FILE_LOCKS: OnceLock<Mutex<HashMap<String, Arc<Mutex<()>>>>> = OnceLock::new();

fn get_file_lock(file_path: &str) -> Arc<Mutex<()>> {
    let registry = FILE_LOCKS.get_or_init(|| Mutex::new(HashMap::new()));
    let mut map = registry.lock().unwrap_or_else(|e| {
        logw!("[TaskIdentityStore] registry lock poisoned"); e.into_inner()
    });
    map.entry(file_path.to_string())
        .or_insert_with(|| Arc::new(Mutex::new(())))
        .clone()
}

fn delete_file_with_lock(file_path: &std::path::Path) {
    let fp = file_path.to_string_lossy().to_string();
    let lock = get_file_lock(&fp);
    let _guard = lock.lock().unwrap_or_else(|e| { logw!("[TaskIdentityStore] file lock poisoned"); e.into_inner() });
    let _ = std::fs::remove_file(file_path);
}

// ======================== Path helpers ========================

fn get_task_dir(os_account_id: i32, caller: &str) -> String {
    format!("{}/{}/{}/{}", TASK_BASE_DIR_PREFIX, os_account_id, TASK_DIR_SUFFIX, caller)
}

fn get_task_file_path(os_account_id: i32, caller: &str, task_id: &str) -> String {
    format!("{}/{}.json", get_task_dir(os_account_id, caller), task_id)
}

fn get_challenge_file_path(os_account_id: i32, caller: &str, task_id: &str) -> String {
    format!("{}/{}.challenge", get_task_dir(os_account_id, caller), task_id)
}

fn ensure_task_dir_exists(os_account_id: i32, caller: &str) -> Result<()> {
    let dir_path = get_task_dir(os_account_id, caller);
    let path = std::path::Path::new(&dir_path);
    if !path.exists() {
        std::fs::create_dir_all(path).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to create task identity dir: {}", e)
        })?;
        std::fs::set_permissions(path, std::fs::Permissions::from_mode(DIR_MODE)).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to set task identity dir permissions: {}", e)
        })?;
        if let Some(parent) = path.parent() {
            let _ = std::fs::set_permissions(parent, std::fs::Permissions::from_mode(DIR_MODE));
        }
    }
    Ok(())
}

// ======================== Serialize / Deserialize ========================

fn serialize_record(record: &TaskIdentityRecord) -> Result<String> {
    let mut builder = JsonBuilder::new();
    builder.add_number(super::TaskIdentityKeys::VERSION, RECORD_VERSION);
    builder.add_string(super::TaskIdentityKeys::TASK_ATTRIBUTES, &record.task_attributes);
    builder.add_string(super::TaskIdentityKeys::IDENTITY, &record.identity);
    builder.add_string(super::TaskIdentityKeys::CHALLENGE, &record.challenge);
    builder.build()
}

fn serialize_challenge_record(challenge_b64: &str, created_boot_ms: u64) -> Result<String> {
    let mut builder = JsonBuilder::new();
    builder.add_number(super::TaskIdentityKeys::VERSION, RECORD_VERSION);
    builder.add_string(super::TaskIdentityKeys::CHALLENGE, challenge_b64);
    builder.add_number(super::TaskIdentityKeys::CREATE_TIME, i64::try_from(created_boot_ms).unwrap_or(i64::MAX));
    builder.build()
}

fn deserialize_record(json: &str) -> Option<TaskIdentityRecord> {
    let parsed = JsonValue::from_text(json).ok()?;
    let version = get_compact_json_value(&parsed, super::TaskIdentityKeys::VERSION)
        .ok().and_then(|s| s.trim_matches('"').parse::<i64>().ok()).unwrap_or(1);
    if version > RECORD_VERSION {
        loge!("[TaskIdentityStore] unsupported record version: {}", version);
        return None;
    }
    let task_attributes = get_compact_json_value(&parsed, super::TaskIdentityKeys::TASK_ATTRIBUTES).ok()?;
    let identity = get_compact_json_value(&parsed, super::TaskIdentityKeys::IDENTITY).ok()?;
    let challenge = get_compact_json_value(&parsed, super::TaskIdentityKeys::CHALLENGE).unwrap_or_default();

    // Parse taskAttributes to extract fields stored inside it
    let attr_parsed = JsonValue::from_text(&task_attributes).ok()?;
    let task_expire_sec = get_compact_json_value(&attr_parsed, super::TaskIdentityKeys::TASK_EXPIRE_SEC)
        .ok().and_then(|s| s.trim_matches('"').parse::<i64>().ok())?;
    let execute_type_val = get_compact_json_value(&attr_parsed, super::TaskIdentityKeys::EXECUTE_TYPE)
        .ok().and_then(|s| s.trim_matches('"').parse::<i32>().ok())?;
    let execute_type = match execute_type_val {
        1 => ExecuteType::Instant,
        2 => ExecuteType::Delayed,
        _ => return None,
    };
    let allow_screen_lock_execution = get_compact_json_value(&attr_parsed, super::TaskIdentityKeys::ALLOW_SCREEN_LOCK_EXECUTION)
        .ok().and_then(|s| s.trim_matches('"').parse::<bool>().ok()).unwrap_or(false);
    let allowable_duration = get_compact_json_value(&attr_parsed, super::TaskIdentityKeys::ALLOWABLE_DURATION)
        .ok().and_then(|s| s.trim_matches('"').parse::<i32>().ok()).unwrap_or(0);

    Some(TaskIdentityRecord {
        task_attributes, identity, challenge, task_expire_sec, execute_type,
        allow_screen_lock_execution, allowable_duration,
    })
}

// ======================== DiskStorage (production) ========================

use std::fs::{self, OpenOptions};
use std::os::unix::fs::{OpenOptionsExt, PermissionsExt};
use std::path::Path;

pub struct DiskStorage;

impl StorageBackend for DiskStorage {
    fn write_record_file(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
        record: &TaskIdentityRecord
    ) -> Result<()> {
        ensure_task_dir_exists(os_account_id, caller)?;
        let file_path = get_task_file_path(os_account_id, caller, task_id);
        let lock = get_file_lock(&file_path);
        let _guard = lock
            .lock().unwrap_or_else(|e| { logw!("[TaskIdentityStore] file lock poisoned"); e.into_inner() });
        let path = Path::new(&file_path);
        let tmp_path = format!("{}.tmp", file_path);
        let tmp_ref = Path::new(&tmp_path);
        let content = serialize_record(record)?;
        let write_result = (|| -> std::io::Result<()> {
            let mut file = OpenOptions::new().write(true).create(true).truncate(true)
                .mode(FILE_MODE).open(tmp_ref)?;
            std::io::Write::write_all(&mut file, content.as_bytes())?;
            file.sync_all()?;
            Ok(())
        })();
        if let Err(e) = write_result {
            let _ = fs::remove_file(tmp_ref);
            return Err(macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to write task tmp file: {}", e));
        }
        fs::rename(tmp_ref, path).map_err(|e| {
            let _ = fs::remove_file(tmp_ref);
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to rename task file: {}", e)
        })
    }

    fn read_record_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<Option<TaskIdentityRecord>> {
        let file_path = get_task_file_path(os_account_id, caller, task_id);
        let lock = get_file_lock(&file_path);
        let _guard = lock
            .lock().unwrap_or_else(|e| { logw!("[TaskIdentityStore] file lock poisoned"); e.into_inner() });
        let content = match fs::read_to_string(&file_path) {
            Ok(c) => c,
            Err(_) => return Ok(None),
        };
        match deserialize_record(&content) {
            Some(record) => Ok(Some(record)),
            None => {
                loge!("[TaskIdentityStore] failed to parse task file: {}", file_path);
                Ok(None)
            }
        }
    }

    fn delete_record_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<()> {
        let file_path = get_task_file_path(os_account_id, caller, task_id);
        let lock = get_file_lock(&file_path);
        let _guard = lock
            .lock().unwrap_or_else(|e| { logw!("[TaskIdentityStore] file lock poisoned"); e.into_inner() });
        let path = Path::new(&file_path);
        if !path.exists() { return Ok(()); }
        fs::remove_file(path).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to delete task file: {}", e)
        })
    }

    fn write_challenge_file(
        &self,
        os_account_id: i32,
        caller: &str,
        task_id: &str,
        challenge_b64: &str,
        created_boot_ms: u64
    ) -> Result<()> {
        ensure_task_dir_exists(os_account_id, caller)?;
        let file_path = get_challenge_file_path(os_account_id, caller, task_id);
        let lock = get_file_lock(&file_path);
        let _guard = lock
            .lock().unwrap_or_else(|e| { logw!("[TaskIdentityStore] file lock poisoned"); e.into_inner() });
        let path = Path::new(&file_path);
        let tmp_path = format!("{}.tmp", file_path);
        let tmp_ref = Path::new(&tmp_path);
        let content = serialize_challenge_record(challenge_b64, created_boot_ms)?;
        let write_result = (|| -> std::io::Result<()> {
            let mut file = OpenOptions::new().write(true).create(true).truncate(true)
                .mode(FILE_MODE).open(tmp_ref)?;
            std::io::Write::write_all(&mut file, content.as_bytes())?;
            file.sync_all()?;
            Ok(())
        })();
        if let Err(e) = write_result {
            let _ = fs::remove_file(tmp_ref);
            return Err(macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to write challenge tmp file: {}", e));
        }
        fs::rename(tmp_ref, path).map_err(|e| {
            let _ = fs::remove_file(tmp_ref);
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to rename challenge file: {}", e)
        })
    }

    fn read_challenge_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Option<(String, u64)> {
        let file_path = get_challenge_file_path(os_account_id, caller, task_id);
        let content = fs::read_to_string(file_path).ok()?;
        let parsed = JsonValue::from_text(content).ok()?;
        let challenge = get_compact_json_value(&parsed, super::TaskIdentityKeys::CHALLENGE).ok()?;
        let created_boot_ms = get_compact_json_value(&parsed, super::TaskIdentityKeys::CREATE_TIME)
            .ok().and_then(|s| s.trim_matches('"').parse::<u64>().ok()).unwrap_or(0);
        Some((challenge, created_boot_ms))
    }

    fn delete_challenge_file(&self, os_account_id: i32, caller: &str, task_id: &str) -> Result<()> {
        let file_path = get_challenge_file_path(os_account_id, caller, task_id);
        let lock = get_file_lock(&file_path);
        let _guard = lock
            .lock().unwrap_or_else(|e| { logw!("[TaskIdentityStore] file lock poisoned"); e.into_inner() });
        let path = Path::new(&file_path);
        if !path.exists() { return Ok(()); }
        fs::remove_file(path).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to delete challenge file: {}", e)
        })
    }

    fn count_caller_files(&self, os_account_id: i32, caller: &str) -> Result<usize> {
        let dir = get_task_dir(os_account_id, caller);
        let path = Path::new(&dir);
        if !path.exists() { return Ok(0); }
        fs::read_dir(path)
            .map(|entries| entries.filter_map(|e| e.ok()).count())
            .map_err(|e| {
                macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                    "Failed to count caller files: {}", e)
            })
    }

    fn cleanup_disk_expired(&self, os_account_id: i32, now_sec: i64, now_boot_ms: u64) {
        let task_base = format!("{}/{}/{}", TASK_BASE_DIR_PREFIX, os_account_id, TASK_DIR_SUFFIX);
        let caller_dirs = match fs::read_dir(&task_base) {
            Ok(d) => d,
            Err(_) => return,
        };
        for caller_dir in caller_dirs.filter_map(|e| e.ok()) {
            cleanup_caller_dir(&caller_dir.path(), now_sec, now_boot_ms);
        }
        logi!("[TaskIdentityStore] disk cleanup done for os_account_id={}", os_account_id);
    }

    fn cleanup_file_locks(&self) {
        let Some(registry) = FILE_LOCKS.get() else { return; };
        let mut map = registry.lock().unwrap_or_else(|e| {
            logw!("[TaskIdentityStore] registry lock poisoned"); e.into_inner()
        });
        let before = map.len();
        map.retain(|_, arc| Arc::strong_count(arc) > 1);
        let removed = before - map.len();
        if removed > 0 {
            logi!("[TaskIdentityStore] cleaned {} stale file locks ({} → {})", removed, before, map.len());
        }
    }
}

/// Scans one caller's directory and deletes expired files.
fn cleanup_caller_dir(caller_path: &Path, now_sec: i64, now_boot_ms: u64) {
    let files = match fs::read_dir(caller_path) {
        Ok(f) => f,
        Err(_) => return,
    };
    for file_path in files.filter_map(|e| e.ok()).map(|e| e.path()) {
        cleanup_one_file(&file_path, now_sec, now_boot_ms);
    }
}

/// Checks a single file and deletes it if expired or corrupted.
fn cleanup_one_file(file_path: &Path, now_sec: i64, now_boot_ms: u64) {
    let is_challenge = file_path.extension()
        .map(|ext| ext == super::TaskIdentityKeys::CHALLENGE).unwrap_or(false);
    let content = match fs::read_to_string(file_path) {
        Ok(c) => c,
        Err(_) => return,
    };
    let should_delete = if is_challenge {
        should_delete_challenge(&content, now_boot_ms)
    } else {
        should_delete_task(&content, now_sec)
    };
    if should_delete {
        delete_file_with_lock(file_path);
    }
}

/// Returns true if a challenge file is expired or corrupted.
fn should_delete_challenge(content: &str, now_boot_ms: u64) -> bool {
    if let Ok(parsed) = JsonValue::from_text(content) {
        let created_boot_ms = get_compact_json_value(&parsed, super::TaskIdentityKeys::CREATE_TIME)
            .ok().and_then(|s| s.trim_matches('"').parse::<u64>().ok()).unwrap_or(0);
        is_challenge_expired(created_boot_ms, now_boot_ms)
    } else {
        true
    }
}

/// Returns true if a task file is expired or corrupted.
fn should_delete_task(content: &str, now_sec: i64) -> bool {
    if let Some(record) = deserialize_record(content) {
        (now_sec as u64) > record.task_expire_sec as u64
    } else {
        true
    }
}

// ======================== MockStorage (tests) ========================

pub struct MockStorage;

impl StorageBackend for MockStorage {
    fn write_record_file(&self, _os_account_id: i32, _caller: &str, _task_id: &str, _record: &TaskIdentityRecord) -> Result<()> { Ok(()) }
    fn read_record_file(&self, _os_account_id: i32, _caller: &str, _task_id: &str) -> Result<Option<TaskIdentityRecord>> { Ok(None) }
    fn delete_record_file(&self, _os_account_id: i32, _caller: &str, _task_id: &str) -> Result<()> { Ok(()) }
    fn write_challenge_file(&self, _os_account_id: i32, _caller: &str, _task_id: &str, _challenge_b64: &str, _created_boot_ms: u64) -> Result<()> { Ok(()) }
    fn read_challenge_file(&self, _os_account_id: i32, _caller: &str, _task_id: &str) -> Option<(String, u64)> { None }
    fn delete_challenge_file(&self, _os_account_id: i32, _caller: &str, _task_id: &str) -> Result<()> { Ok(()) }
    fn count_caller_files(&self, _os_account_id: i32, _caller: &str) -> Result<usize> { Ok(0) }
    fn cleanup_disk_expired(&self, _os_account_id: i32, _expire_threshold_sec: i64, _now_boot_ms: u64) {}
    fn cleanup_file_locks(&self) {}
}
