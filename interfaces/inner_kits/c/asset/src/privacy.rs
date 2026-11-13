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
use asset_sdk::privacy::{PrivacyTargetRequest, PrivacyDecryptRequest, PrivacyBuffer};
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::privacy::{DecryptRequest, PrivacyBlob, TargetRequest, PROTOCOL_SUCCESS};
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_log::loge;
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::full::Manager;
#[cfg(not(feature = "AssetEmptyMode"))]
use asset_sdk::{ErrCode, Result};

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

#[cfg(not(feature = "AssetEmptyMode"))]
fn blob_size(blob: PrivacyBlob) -> Option<usize> {
    let size = blob.size as usize;
    if size > isize::MAX as usize || (size != 0 && blob.data.is_null()) {
        return None;
    }
    Some(size)
}

#[cfg(not(feature = "AssetEmptyMode"))]
fn send_privacy_request(
    request: impl FnOnce(&mut Manager) -> Result<(i32, Vec<u8>)>,
) -> std::result::Result<(i32, Vec<u8>), i32> {
    let manager = Manager::build().map_err(|e| map_privacy_error(e.code))?;
    let mut guard = manager.lock().map_err(|_| {
        loge!("Privacy request failed to lock Asset Manager");
        INTERNAL_ERROR
    })?;
    request(&mut guard).map_err(|e| map_privacy_error(e.code))
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
extern "C" fn asset_privacy_gen_target(
    input: *const PrivacyTargetRequest,
    output: *mut PrivacyBuffer,
) -> i32 {
    if input.is_null() || output.is_null() {
        loge!("Privacy FFI input or output is null");
        return INVALID_ARGUMENT;
    }
    // SAFETY: The C caller supplies an exclusive, initialized descriptor.
    let output = unsafe { &mut *output };
    if output.data.is_null() {
        return INVALID_ARGUMENT;
    }
    // SAFETY: The C caller supplies an initialized request and readable element bytes.
    let request = unsafe { &*input };
    let size = match blob_size(request.element) {
        Some(size) => size,
        None => {
            loge!("Invalid privacy element buffer");
            return INVALID_ARGUMENT;
        },
    };
    let element = if size == 0 {
        Vec::new()
    } else {
        // SAFETY: The length and non-null pointer were checked above. The caller owns the storage.
        unsafe { std::slice::from_raw_parts(request.element.data, size) }.to_vec()
    };
    let input = TargetRequest { protocol: request.protocol, hash_alg: request.hash_alg, element };
    let (result, bytes) = match send_privacy_request(|manager| manager.gen_privacy_target(&input)) {
        Ok(res) => res,
        Err(code) => return code,
    };
    if result != PROTOCOL_SUCCESS {
        return result;
    }
    if bytes.len() > output.size as usize {
        return INVALID_ARGUMENT;
    }
    // SAFETY: The capacity was checked above and the C caller guarantees non-overlap.
    unsafe { std::ptr::copy_nonoverlapping(bytes.as_ptr(), output.data, bytes.len()) };
    output.size = bytes.len() as u32;
    PROTOCOL_SUCCESS
}

/// Decrypts one ciphertext through Asset IPC, borrowing input and committing output only on success.
///
/// # Safety
/// Non-null input and output must point to aligned, initialized request and buffer structs.
/// All nonempty input ranges must remain readable and unchanged until this call returns.
/// The output buffer must be exclusively writable for its declared capacity and must not
/// overlap the input or either descriptor struct. The caller retains ownership of all buffers.
#[cfg(not(feature = "AssetEmptyMode"))]
#[no_mangle]
extern "C" fn asset_privacy_decrypt_ciphertext(
    input: *const PrivacyDecryptRequest,
    output: *mut PrivacyBuffer,
) -> i32 {
    if input.is_null() || output.is_null() {
        loge!("Privacy FFI input or output is null");
        return INVALID_ARGUMENT;
    }
    // SAFETY: The C caller supplies an exclusive, initialized descriptor.
    let output = unsafe { &mut *output };
    if output.data.is_null() {
        return INVALID_ARGUMENT;
    }
    // SAFETY: The C caller supplies an initialized request and readable ciphertext bytes.
    let request = unsafe { &*input };
    let size = match blob_size(request.ciphertext) {
        Some(size) if size != 0 => size,
        _ => {
            loge!("Invalid privacy ciphertext buffer");
            return INVALID_ARGUMENT;
        },
    };
    // SAFETY: The length and non-null pointer were checked above. The caller owns the storage.
    let ciphertext = unsafe { std::slice::from_raw_parts(request.ciphertext.data, size) }.to_vec();
    let input = DecryptRequest { protocol: request.protocol, ciphertext };
    let (result, bytes) = match send_privacy_request(|manager| manager.decrypt_privacy_ciphertext(&input)) {
        Ok(res) => res,
        Err(code) => return code,
    };
    if result != PROTOCOL_SUCCESS {
        return result;
    }
    if bytes.len() > output.size as usize {
        return INVALID_ARGUMENT;
    }
    // SAFETY: The capacity was checked above and the C caller guarantees non-overlap.
    unsafe { std::ptr::copy_nonoverlapping(bytes.as_ptr(), output.data, bytes.len()) };
    output.size = bytes.len() as u32;
    PROTOCOL_SUCCESS
}

/// Returns UNSUPPORTED without reading or writing the request or output in empty mode.
#[cfg(feature = "AssetEmptyMode")]
#[no_mangle]
extern "C" fn asset_privacy_gen_target(
    _input: *const PrivacyTargetRequest,
    _output: *mut PrivacyBuffer,
) -> i32 {
    UNSUPPORTED
}

/// Returns UNSUPPORTED without reading or writing the request or result in empty mode.
#[cfg(feature = "AssetEmptyMode")]
#[no_mangle]
extern "C" fn asset_privacy_decrypt_ciphertext(
    _input: *const PrivacyDecryptRequest,
    _output: *mut PrivacyBuffer,
) -> i32 {
    UNSUPPORTED
}
