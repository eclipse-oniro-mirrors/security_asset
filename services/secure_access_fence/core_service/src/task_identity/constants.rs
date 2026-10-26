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

//! Task identity constants: JSON keys, limits, paths, timeouts.
//! Centralized here for easy maintenance. Keep in sync with napi_common.h.

// ============================================================================
// JSON key constants.
// IMPORTANT: Keep in sync with TaskIdentityKeys in:
//   frameworks/common/napi/inc/napi_common.h
// ============================================================================

/// All JSON keys used by task identity module.
pub struct TaskIdentityKeys;
impl TaskIdentityKeys {
    // taskAttributes fields
    pub const TASK_ID: &str = "taskId";
    pub const TASK_EXPIRE_TIME: &str = "taskExpireTime";
    pub const TASK_EXPIRE_SEC: &str = "taskExpireSec";
    pub const EXECUTE_TYPE: &str = "executeType";
    pub const ALLOW_SCREEN_LOCK_EXECUTION: &str = "allowScreenLockExecution";
    pub const ALLOWABLE_DURATION: &str = "allowableDuration";
    pub const AUTH_TOKEN: &str = "authToken";
    pub const ATL: &str = "atl";
    // TaskIdentity fields
    pub const TASK_ATTRIBUTES: &str = "taskAttributes";
    pub const IDENTITY: &str = "identity";
    pub const CHALLENGE: &str = "challenge";
    // Disk record fields
    pub const VERSION: &str = "version";
    pub const CREATE_TIME: &str = "createTime";
}

// ============================================================================
// Limits and sizes.
// IMPORTANT: Keep in sync with napi_common.h constants.
// ============================================================================

/// Max path segment length for taskId, caller_token, etc.
pub const MAX_PATH_SEGMENT_LEN: usize = 256;
/// Max auth token size in bytes (DoS prevention).
pub const MAX_AUTH_TOKEN_SIZE: usize = 1024;
/// Max allowable duration in seconds.
pub const MAX_ALLOWABLE_DURATION: i32 = 60;
/// Default allowable duration in seconds (when not provided, i.e., 0).
pub const DEFAULT_ALLOWABLE_DURATION: i32 = 60;
/// Max task IDs per revoke call.
pub const MAX_REVOKE_TASK_IDS: usize = 100;
/// Max task attributes JSON size in bytes (DoS prevention).
pub const MAX_TASK_ATTRIBUTES_SIZE: usize = 8192;

// ============================================================================
// Challenge constants.
// ============================================================================

/// Challenge size in bytes.
pub const CHALLENGE_SIZE: u32 = 32;
/// Combined challenge size (challenge1 + challenge2) for splitting.
pub const COMBINED_CHALLENGE_SIZE: usize = 64;
/// Challenge lifetime in milliseconds (10 minutes), using boot time.
pub const CHALLENGE_TIMEOUT_MS: u64 = 600_000;

// ============================================================================
// Memory cache constants.
// ============================================================================

/// Max entries in each LRU Vec (tasks and challenges, independently).
pub const VEC_CAPACITY: usize = 32;
/// Max files per caller directory on disk.
pub const MAX_FILES_PER_CALLER: usize = 10000;

// ============================================================================
// Time constants.
// ============================================================================

/// Milliseconds per second, for sec↔ms conversion.
pub const MS_PER_SEC: u64 = 1000;

// ============================================================================
// Disk persistence constants.
// ============================================================================

/// JSON record version.
pub const RECORD_VERSION: i64 = 1;
/// Base directory prefix for task files.
pub const TASK_BASE_DIR_PREFIX: &str = "/data/service/el2";
/// Task directory suffix (appended to base + os_account_id).
pub const TASK_DIR_SUFFIX: &str = "secure_access_fence/agent_plugin/taskAttributes";
/// File permission mode (owner rw, group r).
pub const FILE_MODE: u32 = 0o640;
/// Directory permission mode (owner rwx, group rx).
pub const DIR_MODE: u32 = 0o750;

// ============================================================================
// Path traversal patterns.
// IMPORTANT: Keep in sync with PATH_TRAVERSAL_PATTERNS in napi_common.cpp.
// ============================================================================

/// Path traversal patterns to detect in file path segments.
pub const PATH_TRAVERSAL_PATTERNS: &[&str] = &[
    "../", "..//", "./", ".\\", "//", "\\", "/", "/..", "\\..", "..",
];
