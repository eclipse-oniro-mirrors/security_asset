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

//! Challenge cache for remote control verification.

#[cfg(not(feature = "SAFTest"))]
use std::fs::{self, File, OpenOptions};
#[cfg(not(feature = "SAFTest"))]
use std::io::{BufRead, BufReader, Write};
#[cfg(not(feature = "SAFTest"))]
use std::os::unix::fs::{OpenOptionsExt, PermissionsExt};
#[cfg(not(feature = "SAFTest"))]
use std::path::Path;
#[cfg(not(feature = "SAFTest"))]
use std::sync::Mutex;

use saf_definition::{macros_lib, ErrCode, Result, DeviceIdHeader};
#[cfg(not(feature = "SAFTest"))]
use saf_log::{loge, logi};
#[cfg(not(feature = "SAFTest"))]
use saf_utils::{JsonValue, get_compact_json_value};
#[cfg(not(feature = "SAFTest"))]
use saf_common::JsonBuilder;

#[cfg(not(feature = "SAFTest"))]
static CHALLENGE_CACHE_LOCK: Mutex<()> = Mutex::new(());

#[cfg(not(feature = "SAFTest"))]
const CACHE_DIR_PREFIX: &str = "/data/service/el2";
#[cfg(not(feature = "SAFTest"))]
const CACHE_DIR_SUFFIX: &str = "secure_access_fence/agent_plugin";
#[cfg(not(feature = "SAFTest"))]
const CACHE_FILE_NAME: &str = "challenge_cache_list.txt";
#[cfg(not(feature = "SAFTest"))]
const FILE_MODE: u32 = 0o640;

#[cfg(not(feature = "SAFTest"))]
#[derive(Debug, Clone)]
struct ChallengeCacheEntry {
    timestamp: u64,
    controller_device_id: String,
    controlled_device_id: String,
}

#[cfg(not(feature = "SAFTest"))]
fn serialize_cache_entry(entry: &ChallengeCacheEntry) -> Result<String> {
    let mut builder = JsonBuilder::new();
    builder.add_u64("timestamp", entry.timestamp);
    builder.add_string("controllerDeviceId", &entry.controller_device_id);
    builder.add_string("controlledDeviceId", &entry.controlled_device_id);
    builder.build()
}

#[cfg(not(feature = "SAFTest"))]
fn deserialize_cache_entry(json: &str) -> Result<ChallengeCacheEntry> {
    let parsed = JsonValue::from_text(json).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "Failed to parse cache entry JSON: {}", e)
    })?;
    let timestamp_str = get_compact_json_value(&parsed, "timestamp")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();
    let timestamp = timestamp_str.parse::<u64>().map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "Failed to parse timestamp: {}", e)
    })?;
    let controller_device_id = get_compact_json_value(&parsed, "controllerDeviceId")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();
    let controlled_device_id = get_compact_json_value(&parsed, "controlledDeviceId")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();
    Ok(ChallengeCacheEntry { timestamp, controller_device_id, controlled_device_id })
}

#[cfg(not(feature = "SAFTest"))]
fn get_cache_file_path(os_account_id: i32) -> String {
    format!("{}/{}/{}", CACHE_DIR_PREFIX, os_account_id, CACHE_DIR_SUFFIX)
}

#[cfg(not(feature = "SAFTest"))]
fn get_cache_file_full_path(os_account_id: i32) -> String {
    format!("{}/{}", get_cache_file_path(os_account_id), CACHE_FILE_NAME)
}

#[cfg(not(feature = "SAFTest"))]
fn ensure_cache_dir_exists(os_account_id: i32) -> Result<()> {
    let dir_path = get_cache_file_path(os_account_id);
    let path = Path::new(&dir_path);
    
    if !path.exists() {
        fs::create_dir_all(path).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to create cache dir {}: {}", dir_path, e)
        })?;
        
        fs::set_permissions(path, fs::Permissions::from_mode(0o750)).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to set dir permissions {}: {}", dir_path, e)
        })?;
    }
    Ok(())
}

#[cfg(not(feature = "SAFTest"))]
pub fn cache_challenge(os_account_id: i32, challenge: &str, timestamp: u64, device_id_header: &DeviceIdHeader)
    -> Result<()> {
    let _lock = CHALLENGE_CACHE_LOCK.lock().unwrap_or_else(|e| e.into_inner());
    
    ensure_cache_dir_exists(os_account_id)?;
    
    let entry = ChallengeCacheEntry {
        timestamp,
        controller_device_id: device_id_header.controller_device_id.clone(),
        controlled_device_id: device_id_header.controlled_device_id.clone(),
    };
    let entry_json = serialize_cache_entry(&entry)?;
    let line = format!("{}|{}\n", challenge, entry_json);

    let file_path = get_cache_file_full_path(os_account_id);
    let path = Path::new(&file_path);

    let mut file = OpenOptions::new()
        .write(true)
        .create(true)
        .append(true)
        .mode(FILE_MODE)
        .open(path)
        .map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to open cache file {}: {}", file_path, e)
        })?;

    file.write_all(line.as_bytes()).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
            "Failed to write cache file {}: {}", file_path, e)
    })?;

    logi!("[challenge_cache] Cached challenge for os_account_id={}", os_account_id);
    Ok(())
}

#[cfg(not(feature = "SAFTest"))]
pub fn verify_and_remove_challenge(
    os_account_id: i32,
    challenge: &str,
    device_id_header: &DeviceIdHeader
) -> Result<bool> {
    let _lock = CHALLENGE_CACHE_LOCK.lock().unwrap_or_else(|e| e.into_inner());
    
    let file_path = get_cache_file_full_path(os_account_id);
    let path = Path::new(&file_path);

    if !path.exists() {
        loge!("[challenge_cache] Cache file not found");
        return macros_lib::log_throw_error!(ErrCode::ReplayAttackDetected,
            "Cache file not found");
    }

    let (found, remaining_lines) = find_and_remove_challenge_in_file(
        &file_path, challenge,
        &device_id_header.controller_device_id,
        &device_id_header.controlled_device_id,
    )?;
    
    if found {
        rewrite_cache_file(&file_path, &remaining_lines)?;
        logi!("[challenge_cache] Verified and removed challenge for os_account_id={}", os_account_id);
    } else {
        loge!("[challenge_cache] Challenge not found: {}", challenge);
        return macros_lib::log_throw_error!(ErrCode::ReplayAttackDetected,
            "Challenge not found: {}", challenge);
    }
    
    Ok(found)
}

#[cfg(not(feature = "SAFTest"))]
fn find_and_remove_challenge_in_file(
    file_path: &str,
    challenge: &str,
    expected_controller_id: &str,
    expected_controlled_id: &str,
) -> Result<(bool, Vec<String>)> {
    let file = File::open(file_path).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
            "Failed to open cache file {}: {}", file_path, e)
    })?;

    let reader = BufReader::new(file);
    let mut found = false;
    let mut remaining_lines = Vec::new();

    for line in reader.lines() {
        let line = line.map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to read cache file: {}", e)
        })?;

        if line.is_empty() {
            continue;
        }

        if let Some((cached_challenge, cached_entry_json)) = line.split_once('|') {
            if cached_challenge == challenge {
                if let Ok(entry) = deserialize_cache_entry(cached_entry_json) {
                    if entry.controller_device_id == expected_controller_id
                        && entry.controlled_device_id == expected_controlled_id {
                            found = true;
                            continue;
                    } else {
                        loge!("[challenge_cache] DeviceIdHeader mismatch for challenge");
                        return macros_lib::log_throw_error!(ErrCode::ReplayAttackDetected,
                            "DeviceIdHeader mismatch for challenge");
                    }
                } else {
                    loge!("[challenge_cache] DeviceIdHeader mismatch for challenge");
                    return macros_lib::log_throw_error!(ErrCode::ReplayAttackDetected,
                        "DeviceIdHeader mismatch for challenge");
                }
            }
        }
        remaining_lines.push(line);
    }

    Ok((found, remaining_lines))
}

#[cfg(not(feature = "SAFTest"))]
fn rewrite_cache_file(file_path: &str, lines: &[String]) -> Result<()> {
    let path = Path::new(file_path);
    let tmp_path = format!("{}.tmp", file_path);
    let tmp_path_ref = Path::new(&tmp_path);
    let mut file = OpenOptions::new()
        .write(true)
        .create(true)
        .truncate(true)
        .mode(FILE_MODE)
        .open(tmp_path_ref)
        .map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to open tmp cache file for writing {}: {}", file_path, e)
        })?;
    
    for line in lines {
        writeln!(file, "{}", line).map_err(|e| {
            let _ = fs::remove_file(tmp_path_ref);
            macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to write tmp cache file: {}", e)
        })?;
    }

    file.sync_all().map_err(|e| {
        let _ = fs::remove_file(tmp_path_ref);
        macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to sync tmp cache file: {}", e)
    })?;

    fs::rename(tmp_path_ref, path).map_err(|e| {
        let _ = fs::remove_file(tmp_path_ref);
        macros_lib::log_and_into_saf_error!(ErrCode::FileOperationError,
                "Failed to rename tmp cache file: {}", e)
    })?;
    
    Ok(())
}

// ======================== SAFTest mock implementations ========================

#[cfg(feature = "SAFTest")]
use lazy_static::lazy_static;
#[cfg(feature = "SAFTest")]
use std::collections::HashMap;
#[cfg(feature = "SAFTest")]
use std::sync::Mutex;

#[cfg(feature = "SAFTest")]
lazy_static! {
    static ref MOCK_CHALLENGE_CACHE: Mutex<HashMap<String, (u64, String, String)>> = Mutex::new(HashMap::new());
}

#[cfg(feature = "SAFTest")]
pub fn cache_challenge(os_account_id: i32, challenge: &str, timestamp: u64, device_id_header: &DeviceIdHeader)
    -> Result<()> {
    let key = format!("{}:{}", os_account_id, challenge);
    let value = (timestamp, device_id_header.controller_device_id.clone(),
        device_id_header.controlled_device_id.clone());
    MOCK_CHALLENGE_CACHE.lock().unwrap().insert(key, value);
    Ok(())
}

#[cfg(feature = "SAFTest")]
pub fn verify_and_remove_challenge(
    os_account_id: i32,
    challenge: &str,
    device_id_header: &DeviceIdHeader,
) -> Result<bool> {
    let key = format!("{}:{}", os_account_id, challenge);
    let mut cache = MOCK_CHALLENGE_CACHE.lock().unwrap();
    match cache.remove(&key) {
        Some((_, cached_controller, cached_controlled)) => {
            if cached_controller == device_id_header.controller_device_id
                && cached_controlled == device_id_header.controlled_device_id {
                    Ok(true)
            } else {
                return macros_lib::log_throw_error!(ErrCode::ReplayAttackDetected,
                    "DeviceIdHeader mismatch for challenge");
            }
        }
        None => Ok(true),
    }
}

#[cfg(feature = "SAFTest")]
pub fn reset_mock_challenge_cache() {
    MOCK_CHALLENGE_CACHE.lock().unwrap().clear();
}