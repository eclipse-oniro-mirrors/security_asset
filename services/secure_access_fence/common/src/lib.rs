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

use saf_definition::macros_lib;
use std::convert::TryFrom;

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
    /// Package changed event.
    PackageChanged = 3,
    /// Restore start event.
    RestoreStart = 4,
}

impl TryFrom<&str> for CommonEventType {
    type Error = macros_lib::SAFError;

    fn try_from(s: &str) -> std::result::Result<Self, Self::Error> {
        if s == "usual.event.PACKAGE_REMOVED" {
            return Ok(CommonEventType::PackageRemoved);
        } else if s == "usual.event.PACKAGE_ADDED" {
            return Ok(CommonEventType::PackageAdded);
        } else if s == "usual.event.PACKAGE_CHANGED" {
            return Ok(CommonEventType::PackageChanged);
        } else if s == "usual.event.RESTORE_START" {
            return Ok(CommonEventType::RestoreStart);
        }
        Ok(CommonEventType::Unknown)
    }
}

/// Immutable SAF blob
#[repr(C)]
pub struct ConstSAFBlob {
    /// Data size
    pub size: u32,
    /// Immutable data
    pub data: *const u8,
}
