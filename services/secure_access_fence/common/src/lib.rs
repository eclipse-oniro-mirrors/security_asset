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

//! This crate defines the common constants.

mod counter;
mod task_manager;
mod calling_info;
pub use counter::{AutoCounter, Counter};
pub use task_manager::TaskManager;
pub use calling_info::CallingInfo;

// Re-export JsonBuilder from saf_utils
pub use saf_utils::{JsonBuilder, new_object, object_add_string, object_add_number};

#[cfg(not(feature = "SAFTest"))]
use saf_definition::{macros_lib, ErrCode, Result};
use std::convert::TryFrom;

#[cfg(not(feature = "SAFTest"))]
extern "C" {
    fn GetOsAccountIdFromUid(uid: i32, userId: &mut i32) -> bool;
}

/// Calculate user id from uid.
#[cfg(not(feature = "SAFTest"))]
pub fn get_user_id(uid: u64) -> Result<i32> {
    let uid_i32 = i32::try_from(uid)
        .map_err(|_| macros_lib::log_and_into_saf_error!(ErrCode::InvalidOsAccountId,
            "[FATAL]Uid overflow i32 range"))?;
    unsafe {
        let mut user_id: i32 = 0;
        if GetOsAccountIdFromUid(uid_i32, &mut user_id) {
            Ok(user_id)
        } else {
            macros_lib::log_throw_error!(ErrCode::InvalidOsAccountId, "[FATAL]Get user id failed.")
        }
    }
}

/// The type of the common event.
#[repr(C)]
#[derive(PartialEq, Eq)]
#[derive(Copy, Clone)]
#[derive(Debug)]
pub enum CommonEventType {
    /// Unknown event.
    Unknown = 0,
    /// Package remove event.
    PackageRemoved = 1,
    /// Package added event.
    PackageAdded = 2,
    /// Restore start event.
    RestoreStart = 3,
}

impl TryFrom<&str> for CommonEventType {
    type Error = macros_lib::SAFError;

    fn try_from(s: &str) -> std::result::Result<Self, Self::Error> {
        if s == "usual.event.PACKAGE_REMOVED" {
            return Ok(CommonEventType::PackageRemoved);
        } else if s == "usual.event.PACKAGE_ADDED" {
            return Ok(CommonEventType::PackageAdded);
        } else if s == "usual.event.RESTORE_START" {
            return Ok(CommonEventType::RestoreStart);
        }
        Ok(CommonEventType::Unknown)
    }
}

// ======================== SAFTest mock implementations ========================

#[cfg(feature = "SAFTest")]
use saf_definition::{macros_lib, Result};
#[cfg(feature = "SAFTest")]
use lazy_static::lazy_static;
#[cfg(feature = "SAFTest")]
use std::sync::RwLock;

#[cfg(feature = "SAFTest")]
lazy_static! {
    static ref MOCK_LOCAL_UDID: RwLock<String> = RwLock::new(String::from("mock_local_udid"));
    static ref MOCK_USER_ID: RwLock<i32> = RwLock::new(100);
}

#[cfg(feature = "SAFTest")]
pub fn get_user_id(_uid: u64) -> Result<i32> {
    Ok(*MOCK_USER_ID.read().unwrap())
}

#[cfg(feature = "SAFTest")]
pub fn set_mock_user_id(user_id: i32) {
    *MOCK_USER_ID.write().unwrap() = user_id;
}

#[cfg(feature = "SAFTest")]
pub fn get_local_udid() -> Result<String> {
    Ok(MOCK_LOCAL_UDID.read().unwrap().clone())
}

#[cfg(feature = "SAFTest")]
pub fn set_mock_local_udid(udid: &str) {
    *MOCK_LOCAL_UDID.write().unwrap() = udid.to_string();
}

#[cfg(feature = "SAFTest")]
pub fn reset_mock_local_udid() {
    *MOCK_LOCAL_UDID.write().unwrap() = "mock_local_udid".to_string();
    *MOCK_USER_ID.write().unwrap() = 100;
}
