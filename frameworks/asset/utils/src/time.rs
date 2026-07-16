/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use std::os::raw::{c_int, c_long};

use asset_definition::{macros_lib, ErrCode, Result};

const CLOCK_REALTIME: c_int = 0;
const SECS_TO_MILLIS: i64 = 1000;
const NANOS_TO_MILLIS: i64 = 1_000_000;

#[repr(C)]
struct Timespec {
    tv_sec: c_long,
    tv_nsec: c_long,
}

extern "C" {
    fn clock_gettime(clock_id: c_int, tp: *mut Timespec) -> c_int;
}

fn get_timespec() -> Result<Timespec> {
    let mut ts = Timespec { tv_sec: 0, tv_nsec: 0 };
    if unsafe { clock_gettime(CLOCK_REALTIME, &mut ts) } != 0 {
        return macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
            ErrCode::GetSystemTimeError, "[FATAL]clock_gettime failed");
    }
    if ts.tv_sec < 0 {
        return macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
            ErrCode::GetSystemTimeError, "[FATAL]32-bit time_t overflow (Y2038)");
    }
    Ok(ts)
}

/// Get the current time from the system, in milliseconds.
pub fn system_time_in_millis() -> Result<Vec<u8>> {
    let ts = get_timespec().map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?;
    let millis = ts.tv_sec as i64 * SECS_TO_MILLIS + ts.tv_nsec as i64 / NANOS_TO_MILLIS;
    Ok(millis.to_string().as_bytes().to_vec())
}

/// Get the current time from the system, in seconds.
pub fn system_time_in_seconds() -> Result<u64> {
    let ts = get_timespec().map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?;
    Ok(ts.tv_sec as u64)
}
