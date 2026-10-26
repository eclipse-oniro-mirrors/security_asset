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

//! Typed C bindings, following the existing full.rs Manager/guard convention.
use asset_sdk::privacy::{PrivacyTargetRequest, PrivacySearchRequest, PrivacyBuffer,
    PrivacyResult, PrivacyBlobList};
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::privacy::{PrivacyBlob, SearchResponse, blobs, MAX_BYTES, PROTOCOL_SUCCESS};
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_log::loge;
use std::ptr::null;
use std::ptr::null_mut;
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::full::Manager;
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::ErrCode;

const UNSUPPORTED: i32 = 2;
#[cfg(not(feature = "AssetEmptyMode"))]
const INVALID_ARGUMENT: i32 = 1;
#[cfg(not(feature = "AssetEmptyMode"))]
const INTERNAL_ERROR: i32 = 9;

#[cfg(not(feature = "AssetEmptyMode"))]
struct ResultStorage {
    values: Vec<Vec<u8>>,
    blobs: Vec<PrivacyBlob>,
}

#[cfg(not(feature = "AssetEmptyMode"))]
fn into_result(response: SearchResponse) -> PrivacyResult {
    let has_values = response.values.is_some();
    let mut result_storage = Box::new(ResultStorage {
        values: response.values.unwrap_or_default(),
        blobs: Vec::new(),
    });
    result_storage.blobs = blobs(&result_storage.values);
    PrivacyResult {
        matched: response.matched as u32,
        has_values: has_values as u32,
        values: PrivacyBlobList { data: result_storage.blobs.as_ptr(), count: result_storage.blobs.len() as u32 },
        storage: Box::into_raw(result_storage) as *mut std::ffi::c_void,
    }
}

#[cfg(not(feature = "AssetEmptyMode"))]
fn map_privacy_error(code: ErrCode) -> i32 {
    match code {
        ErrCode::InvalidArgument => INVALID_ARGUMENT,
        ErrCode::Unsupported => UNSUPPORTED,
        _ => INTERNAL_ERROR,
    }
}

/// Generates a target through Asset IPC, borrowing input and committing output only on success.
///
/// # Safety
/// Non-null input and output must point to aligned, initialized request and buffer structs.
/// All nonempty input ranges must remain readable and unchanged until this call returns.
/// The output buffer must be exclusively writable for its declared capacity and must not
/// overlap the input or either descriptor struct. The caller retains ownership of all buffers.
#[cfg(not(feature = "AssetEmptyMode"))]
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_gen_target(
    input: *const PrivacyTargetRequest,
    output: *mut PrivacyBuffer,
) -> i32 {
    if input.is_null() || output.is_null() {
        loge!("Privacy FFI input or output is null");
        return INVALID_ARGUMENT;
    }
    let output = &mut *output;
    if output.data.is_null() || output.size as usize > MAX_BYTES {
        return INVALID_ARGUMENT;
    }
    let input = match (*input).to_value() {
        Ok(input) => input,
        Err(e) => return map_privacy_error(e.code),
    };
    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return map_privacy_error(e.code),
    };
    let mut guard = match manager.lock() {
        Ok(guard) => guard,
        Err(_) => {
            loge!("Privacy request failed to lock Asset Manager");
            return INTERNAL_ERROR;
        },
    };
    let (result, bytes) = match guard.gen_privacy_target(&input) {
        Ok(res) => res,
        Err(e) => return map_privacy_error(e.code),
    };
    if result != PROTOCOL_SUCCESS {
        return result;
    }
    if bytes.len() > output.size as usize {
        return INVALID_ARGUMENT;
    }
    std::ptr::copy_nonoverlapping(bytes.as_ptr(), output.data, bytes.len());
    output.size = bytes.len() as u32;
    PROTOCOL_SUCCESS
}

/// Retrieves an owned result through Asset IPC; release it with asset_privacy_free_result.
///
/// # Safety
/// Non-null input must point to an aligned, initialized request. Its descriptor arrays and
/// nonempty byte ranges must be readable and unchanged until this call returns.
/// Non-null output must be aligned, initialized, exclusively writable and disjoint from input.
/// It must be zero-initialized or previously freed; do not overwrite an outstanding result.
#[cfg(not(feature = "AssetEmptyMode"))]
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_get_result(
    input: *const PrivacySearchRequest,
    output: *mut PrivacyResult,
) -> i32 {
    if input.is_null() || output.is_null() {
        loge!("Privacy FFI input or output is null");
        return INVALID_ARGUMENT;
    }
    let input = match (*input).to_value() {
        Ok(input) => input,
        Err(e) => return map_privacy_error(e.code),
    };
    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return map_privacy_error(e.code),
    };
    let mut guard = match manager.lock() {
        Ok(guard) => guard,
        Err(_) => {
            loge!("Privacy request failed to lock Asset Manager");
            return INTERNAL_ERROR;
        },
    };
    let (result, response) = match guard.get_privacy_result(&input) {
        Ok(res) => res,
        Err(e) => return map_privacy_error(e.code),
    };
    if result != PROTOCOL_SUCCESS {
        return result;
    }
    match response {
        Some(res) => {
            *output = into_result(res);
            PROTOCOL_SUCCESS
        },
        None => {
            loge!("Missing privacy response after successful IPC");
            INTERNAL_ERROR
        },
    }
}

/// Returns UNSUPPORTED without reading or writing the request or output in empty mode.
///
/// # Safety
/// This build does not dereference either pointer, so null pointers are accepted and no
/// pointee validity is required. No result allocation is created.
#[cfg(feature = "AssetEmptyMode")]
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_gen_target(
    _input: *const PrivacyTargetRequest,
    _output: *mut PrivacyBuffer,
) -> i32 {
    UNSUPPORTED
}

/// Returns UNSUPPORTED without reading or writing the request or result in empty mode.
///
/// # Safety
/// This build does not dereference either pointer, so null pointers are accepted and no
/// pointee validity is required. Existing result storage is not released or replaced.
#[cfg(feature = "AssetEmptyMode")]
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_get_result(
    _input: *const PrivacySearchRequest,
    _output: *mut PrivacyResult,
) -> i32 {
    UNSUPPORTED
}

/// Releases a result allocated by this module and clears it; null output is accepted.
///
/// # Safety
/// Non-null output must point to an aligned, initialized, exclusively accessible result struct.
/// It must be zero-initialized, previously cleared, or an unchanged, unreleased result returned
/// by this module. Do not pass a copied ownership handle or storage from another allocator.
/// No outstanding reader may access the result's descriptors or bytes during or after release.
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_free_result(output: *mut PrivacyResult) {
    if output.is_null() {
        return;
    }
    let output = &mut *output;
    #[cfg(not(feature = "AssetEmptyMode"))]
    if !output.storage.is_null() {
        drop(Box::from_raw(output.storage as *mut ResultStorage));
    }
    output.storage = null_mut();
    output.values = PrivacyBlobList { data: null(), count: 0 };
    output.matched = 0;
    output.has_values = 0;
}
