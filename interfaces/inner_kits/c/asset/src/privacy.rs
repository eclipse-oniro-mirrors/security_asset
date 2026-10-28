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
use asset_sdk::privacy::{PrivacyTargetRequest, PrivacySearchRequest, PrivacyBuffer};
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::privacy::{encode_result_bytes, MAX_BYTES, PROTOCOL_SUCCESS};
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_log::loge;
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

/// Retrieves an encoded result into caller-owned storage; no allocation ownership is transferred.
/// Output bytes and size are committed only after successful IPC, encoding and capacity validation.
///
/// # Safety
/// Non-null input and output must point to aligned, initialized request and buffer structs.
/// Input descriptors and nonempty byte ranges must remain readable and unchanged during this call.
/// Output storage must be exclusively writable for its declared capacity and must not overlap
/// the input ranges or descriptor structs. The caller retains ownership of all buffers.
#[cfg(not(feature = "AssetEmptyMode"))]
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_get_result(
    input: *const PrivacySearchRequest,
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
    let (result, response) = match guard.get_privacy_result(&input) {
        Ok(res) => res,
        Err(e) => return map_privacy_error(e.code),
    };
    if result != PROTOCOL_SUCCESS {
        return result;
    }
    let response = match response {
        Some(response) => response,
        None => {
            loge!("Missing privacy response after successful IPC");
            return INTERNAL_ERROR;
        },
    };
    let bytes = match encode_result_bytes(&response) {
        Ok(bytes) => bytes,
        Err(e) => return map_privacy_error(e.code),
    };
    if bytes.len() > output.size as usize {
        loge!("Privacy result exceeds output capacity");
        return INVALID_ARGUMENT;
    }
    std::ptr::copy_nonoverlapping(bytes.as_ptr(), output.data, bytes.len());
    output.size = bytes.len() as u32;
    PROTOCOL_SUCCESS
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
/// pointee validity is required. No output storage is accessed.
#[cfg(feature = "AssetEmptyMode")]
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_get_result(
    _input: *const PrivacySearchRequest,
    _output: *mut PrivacyBuffer,
) -> i32 {
    UNSUPPORTED
}
