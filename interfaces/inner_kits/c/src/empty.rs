/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

//! This module implements the function of Asset SDK from C to RUST.

use asset_log::loge;
use asset_sdk::{ErrCode, SyncResult};
use crate::*;

fn check_empty_mode() -> i32 {
    loge!("[FATAL][RUST SDK]Asset service is not supported in empty mode");
    ErrCode::Unsupported as i32
}

/// Function called from C programming language to Rust programming language for adding Asset.
#[no_mangle]
pub extern "C" fn add_asset(_attributes: *const AssetAttr, _attr_cnt: u32) -> i32 {
    check_empty_mode()
}

/// Function called from C programming language to Rust programming language for removing Asset.
#[no_mangle]
pub extern "C" fn remove_asset(_query: *const AssetAttr, _query_cnt: u32) -> i32 {
    check_empty_mode()
}

/// Function called from C programming language to Rust programming language for updating Asset.
#[no_mangle]
pub extern "C" fn update_asset(
    _query: *const AssetAttr,
    _query_cnt: u32,
    _attrs_to_update: *const AssetAttr,
    _update_cnt: u32,
) -> i32 {
    check_empty_mode()
}

/// Function called from C programming language to Rust programming language for pre querying Asset.
///
/// # Safety
///
/// The caller must ensure that the challenge pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn pre_query_asset(_query: *const AssetAttr, _query_cnt: u32, _challenge: *mut AssetBlob) -> i32 {
    check_empty_mode()
}

/// Function called from C programming language to Rust programming language for querying Asset.
///
/// # Safety
///
/// The caller must ensure that the result_set pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn query_asset(_query: *const AssetAttr, _query_cnt: u32, _result_set: *mut AssetResultSet) -> i32 {
    check_empty_mode()
}

/// Function called from C programming language to Rust programming language for post quering Asset.
#[no_mangle]
pub extern "C" fn post_query_asset(_handle: *const AssetAttr, _handle_cnt: u32) -> i32 {
    check_empty_mode()
}

/// Function called from C programming language to Rust programming language for querying sync result.
///
/// # Safety
///
/// The caller must ensure that the sync_result pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn query_sync_result(
    _query: *const AssetAttr,
    _query_cnt: u32,
    _sync_result: *mut SyncResult,
) -> i32 {
    check_empty_mode()
}
