/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This module is used to get the system time.

use std::time::{SystemTime, UNIX_EPOCH};
// todo yzt 看一下systemtime和instant的区别

use asset_definition::{log_throw_error, ErrCode, Result};

/// Get the current time from the system, in milliseconds.
pub fn system_time_in_millis() -> Result<Vec<u8>> {
    match SystemTime::now().duration_since(UNIX_EPOCH) {
        Ok(d) => Ok(d.as_millis().to_string().as_bytes().to_vec()),
        Err(e) => log_throw_error!(ErrCode::GetSystemTimeError, "[FATAL]Get system time failed, err: {}", e),
    }
}

/// Get the current time from the system, in seconds.
pub fn system_time_in_seconds() -> Result<u64> {
    match SystemTime::now().duration_since(UNIX_EPOCH) {
        Ok(d) => Ok(d.as_secs()),
        Err(e) => log_throw_error!(ErrCode::GetSystemTimeError, "[FATAL]Get system time failed, err: {}", e),
    }
}
