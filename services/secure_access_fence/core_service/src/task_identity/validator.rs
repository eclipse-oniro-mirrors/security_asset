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

//! Pure input validation for task identity parameters.
//! No side effects, no store/disk/crypto dependencies.
//! Called by stub.rs (IPC layer) for early interception and mod.rs (defense-in-depth).

use saf_definition::{macros_lib, ErrCode, ExecuteType, Result, TaskIdentityInfo};

// ============================================================================
// IMPORTANT: The constants and path traversal patterns below MUST be kept in
// sync with the C++ NAPI layer definitions in:
//   frameworks/common/napi/inc/napi_common.h
//   frameworks/common/napi/src/napi_common.cpp
// If you modify any value here, update the C++ files and vice versa.
// ============================================================================

/// Max path segment length for taskId, caller_token, etc.
const MAX_PATH_SEGMENT_LEN: usize = 256;
/// Max auth token size in bytes (DoS prevention).
const MAX_AUTH_TOKEN_SIZE: usize = 4096;
/// Max allowable duration in seconds — 24 hours (passed to IAM as ms).
const MAX_ALLOWABLE_DURATION: i32 = 86400;
/// Max task IDs per revoke call.
const MAX_REVOKE_TASK_IDS: usize = 100;

/// Path traversal patterns to detect in file path segments.
/// IMPORTANT: Keep in sync with PATH_TRAVERSAL_PATTERNS in napi_common.cpp.
const PATH_TRAVERSAL_PATTERNS: &[&str] = &[
    "../", "..//", "./", ".\\", "//", "\\", "/", "/..", "\\..", "..",
];

/// Checks if a string contains path traversal characters.
fn contains_path_traversal(segment: &str) -> bool {
    PATH_TRAVERSAL_PATTERNS.iter().any(|p| segment.contains(p))
}

/// Unified path segment validation for strings embedded in file paths (taskId, caller_token, etc.).
/// Checks: non-empty, length ≤256, no null byte, no path traversal patterns.
pub fn validate_path_segment(segment: &str) -> Result<()> {
    if segment.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "path segment is empty");
    }
    if segment.len() > MAX_PATH_SEGMENT_LEN {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "path segment too long: {}", segment.len());
    }
    if segment.contains('\0') {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "path segment contains null byte");
    }
    if contains_path_traversal(segment) {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "path segment contains traversal characters");
    }
    Ok(())
}

/// Validates TaskIdentityInfo:
/// - taskId path safety
/// - taskExpireTime: -1 (never) or positive; INSTANT cannot be -1
/// - authToken size ≤4096; each value must be in u8 range (0~255)
/// - allowableDuration: must be >0 when provided (non-zero)
pub fn validate_task_id_info(info: &TaskIdentityInfo) -> Result<()> {
    validate_path_segment(&info.task_id)?;
    if info.task_expire_time == 0 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "taskExpireTime must not be 0; use -1 for never");
    }
    if info.task_expire_time < -1 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "taskExpireTime must be -1 or positive");
    }
    if info.execute_type == ExecuteType::Instant && info.task_expire_time == -1 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "taskExpireTime cannot be -1 for INSTANT tasks");
    }
    if info.auth_token.len() > MAX_AUTH_TOKEN_SIZE {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "authToken too large: {} > {}", info.auth_token.len(), MAX_AUTH_TOKEN_SIZE);
    }
    if info.allowable_duration != 0 {
        if info.allowable_duration <= 0 {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
                "allowableDuration must be positive when provided");
        }
        if info.allowable_duration > MAX_ALLOWABLE_DURATION {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
                "allowableDuration exceeds max: {} > {}", info.allowable_duration, MAX_ALLOWABLE_DURATION);
        }
    }
    Ok(())
}

/// Validates revoke parameters: array length 1~100, each non-empty taskId via validate_path_segment.
pub fn validate_revoke_params(task_ids: &[String]) -> Result<()> {
    if task_ids.is_empty() || task_ids.len() > MAX_REVOKE_TASK_IDS {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "taskIds count must be 1~{}, got {}", MAX_REVOKE_TASK_IDS, task_ids.len());
    }
    for tid in task_ids {
        validate_path_segment(tid)?;
    }
    Ok(())
}
