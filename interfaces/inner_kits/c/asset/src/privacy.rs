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

//! Internal C bindings for the privacy operations; no separate SA client.
#[repr(C)]
pub struct PrivacyClientInput {
    data: *const u8,
    size: u32,
}
#[repr(C)]
pub struct PrivacyClientOutput {
    data: *mut u8,
    size: u32,
}
#[cfg(not(feature = "AssetEmptyMode"))]
const MAX_PAYLOAD: usize = 256 * 1024;
#[cfg(not(feature = "AssetEmptyMode"))]
const INVALID_ARGUMENT: i32 = 1;
#[cfg(feature = "AssetEmptyMode")]
const UNSUPPORTED: i32 = 2;
#[cfg(not(feature = "AssetEmptyMode"))]
const INTERNAL_ERROR: i32 = 9;

#[cfg(not(feature = "AssetEmptyMode"))]
unsafe fn request(input: *const PrivacyClientInput, output: *mut PrivacyClientOutput, generate: bool) -> i32 {
    use asset_sdk::full::{IpcCode, Manager};
    if input.is_null() || output.is_null() {
        return INVALID_ARGUMENT;
    }
    let input = &*input;
    let output = &mut *output;
    if input.data.is_null()
        || output.data.is_null()
        || input.size as usize > MAX_PAYLOAD
        || output.size as usize > MAX_PAYLOAD
    {
        return INVALID_ARGUMENT;
    }
    let payload = std::slice::from_raw_parts(input.data, input.size as usize);
    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(_) => return INTERNAL_ERROR,
    };
    let mut manager = match manager.lock() {
        Ok(manager) => manager,
        Err(_) => return INTERNAL_ERROR,
    };
    let code = if generate { IpcCode::PrivacyGenTarget } else { IpcCode::PrivacyGetResult };
    match manager.privacy_request(payload, code) {
        Ok((0, bytes)) => {
            if bytes.len() > output.size as usize {
                return INVALID_ARGUMENT;
            }
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), output.data, bytes.len());
            output.size = bytes.len() as u32;
            0
        },
        Ok((code, _)) => code,
        Err(_) => INTERNAL_ERROR,
    }
}

#[cfg(feature = "AssetEmptyMode")]
unsafe fn request(_input: *const PrivacyClientInput, _output: *mut PrivacyClientOutput, _generate: bool) -> i32 {
    UNSUPPORTED
}

/// Generate an encrypted target through the Asset Manager.
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_gen_target(
    input: *const PrivacyClientInput, output: *mut PrivacyClientOutput,
) -> i32 {
    request(input, output, true)
}

/// Decrypt a protocol result through the Asset Manager.
#[no_mangle]
pub unsafe extern "C" fn asset_privacy_get_result(
    input: *const PrivacyClientInput, output: *mut PrivacyClientOutput,
) -> i32 {
    request(input, output, false)
}
