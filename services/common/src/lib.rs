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

use std::os::raw::c_char;

use asset_definition::{impl_enum_trait, log_throw_error, AssetError, ErrCode, Result};
mod calling_info;
mod counter;
mod process_info;
mod task_manager;
pub use calling_info::{CallingInfo, Group};
pub use counter::{AutoCounter, Counter};
pub use process_info::{ProcessInfo, ProcessInfoDetail};
pub use task_manager::TaskManager;
/// success code.
pub const SUCCESS: i32 = 0;
/// root user upper bound.
pub const ROOT_USER_UPPERBOUND: u32 = 99;
/// Separator in owner info of calling info between app id and app index.
pub const OWNER_INFO_SEPARATOR: char = '_';
/// Separator in group of calling info between developer id and group id.
pub const GROUP_SEPARATOR: char = ',';

/// Immutable asset blob
#[repr(C)]
pub struct ConstAssetBlob {
    /// Data size
    pub size: u32,
    /// Immutable data
    pub data: *const u8,
}

/// Immutable asset blob array
#[repr(C)]
pub struct ConstAssetBlobArray {
    /// blobs size
    pub size: u32,
    /// Immutable blobs
    pub blobs: *const ConstAssetBlob,
}

/// Modify asset c char
#[repr(C)]
pub struct ModifyAssetBlob {
    /// Modify
    pub modify: bool,
    /// Immutable
    pub data: *const c_char,
}

/// Mutable asset blob array
#[repr(C)]
pub struct MutAssetBlobArray {
    /// blobs size
    pub size: u32,
    /// Mutable blobs
    pub blobs: *mut ModifyAssetBlob,
}

/// Mutable asset blob array
#[repr(C)]
pub struct MutAssetBlobArray {
    /// blobs size
    pub size: u32,
    /// Mutable blobs
    pub blobs: *mut MutAssetBlob,
}

impl_enum_trait! {
    /// The type of the calling.
    #[repr(C)]
    #[derive(PartialEq, Eq)]
    #[derive(Copy, Clone)]
    #[derive(Debug)]
    pub enum OwnerType {
        /// The calling is an application.
        Hap = 0,
        /// The calling is a native process.
        Native = 1,
        /// The calling is a group of applications.
        HapGroup = 2,
    }
}

/// Transfer error code to AssetError
pub fn transfer_error_code(err_code: ErrCode) -> AssetError {
    match err_code {
        ErrCode::AccessDenied => {
            AssetError::new(ErrCode::AccessDenied, "[FATAL]HUKS verify auth token failed".to_string())
        },
        ErrCode::StatusMismatch => {
            AssetError::new(ErrCode::StatusMismatch, "[FATAL]Screen status does not match".to_string())
        },
        ErrCode::InvalidArgument => AssetError::new(ErrCode::InvalidArgument, "[FATAL]Invalid argument.".to_string()),
        ErrCode::BmsError => AssetError::new(ErrCode::BmsError, "[FATAL]Get owner info from bms failed.".to_string()),
        ErrCode::AccessTokenError => {
            AssetError::new(ErrCode::AccessTokenError, "[FATAL]Get process info failed.".to_string())
        },
        _ => AssetError::new(ErrCode::CryptoError, "[FATAL]HUKS execute crypt failed".to_string()),
    }
}

extern "C" {
    fn GetUserIdByUid(uid: u64, userId: &mut u32) -> bool;
    fn IsUserIdExist(userId: i32, exist: &mut bool) -> bool;
}

/// Calculate user id.
pub fn get_user_id(uid: u64) -> Result<u32> {
    unsafe {
        let mut user_id = 0;
        if GetUserIdByUid(uid, &mut user_id) {
            Ok(user_id)
        } else {
            log_throw_error!(ErrCode::AccountError, "[FATAL]Get user id failed.")
        }
    }
}

/// Check user id exist.
pub fn is_user_id_exist(user_id: i32) -> Result<bool> {
    unsafe {
        let mut exist = false;
        if IsUserIdExist(user_id, &mut exist) {
            Ok(exist)
        } else {
            log_throw_error!(ErrCode::AccountError, "[FATAL]Check user id failed.")
        }
    }
}
