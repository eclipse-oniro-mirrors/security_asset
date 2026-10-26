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

//! PSI/PIR requests use the same stop/idle counter and interface token checks as Asset.
use ipc::{parcel::MsgParcel, IpcResult, IpcStatusCode, Skeleton};
use asset_log::loge;
use std::convert::TryFrom;

use asset_ipc::{deserialize_privacy_payload, serialize_privacy_payload, MAX_PRIVACY_PAYLOAD, IPC_SUCCESS};
// Keep field order and integer widths identical to privacy_request.h.
#[repr(C)]
struct AssetPrivacyInput {
    code: u32,
    uid: u32,
    data: *const u8,
    size: u32,
}

#[repr(C)]
struct AssetPrivacyOutput {
    data: *mut u8,
    size: u32,
}

extern "C" {
    fn AssetPrivacyRequest(input: *const AssetPrivacyInput, output: *mut AssetPrivacyOutput) -> i32;
}

pub(crate) fn request(code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    // Capture Binder identity before invoking any native code or asynchronous work.
    let uid = u32::try_from(Skeleton::calling_uid()).map_err(|_| IpcStatusCode::Failed)?;
    let input = deserialize_privacy_payload(data).map_err(|_| IpcStatusCode::InvalidValue)?;
    let mut output = vec![0u8; MAX_PRIVACY_PAYLOAD];
    let request = AssetPrivacyInput {
        code,
        uid,
        data: input.as_ptr(),
        size: input.len() as u32,
    };
    let mut response = AssetPrivacyOutput {
        data: output.as_mut_ptr(),
        size: output.len() as u32,
    };
    let result = unsafe { AssetPrivacyRequest(&request, &mut response) };
    let size = response.size;
    if size as usize > output.len() {
        return Err(IpcStatusCode::Failed);
    }
    reply.write(&IPC_SUCCESS)?;
    reply.write(&result)?;
    if result == 0 {
        output.truncate(size as usize);
        serialize_privacy_payload(&output, reply).map_err(|_| IpcStatusCode::InvalidValue)?;
    } else {
        loge!("Privacy operation failed: {}", result);
    }
    Ok(())
}
