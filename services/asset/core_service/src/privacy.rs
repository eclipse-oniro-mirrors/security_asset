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

const MAX_MESSAGE: usize = 256 * 1024;
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
    let length = data.read::<u32>()? as usize;
    if length > MAX_MESSAGE {
        loge!("Privacy request exceeds the byte limit");
        reply.write(&1i32)?;
        return Ok(());
    }
    let mut input = vec![0u8; length];
    for chunk in input.chunks_mut(4) {
        let word = data.read::<u32>()?.to_le_bytes();
        chunk.copy_from_slice(&word[..chunk.len()]);
    }
    let mut output = vec![0u8; MAX_MESSAGE];
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
    reply.write(&result)?;
    if result == 0 {
        output.truncate(size as usize);
        reply.write(&size)?;
        for chunk in output.chunks(4) {
            let mut word = [0u8; 4];
            word[..chunk.len()].copy_from_slice(chunk);
            reply.write(&u32::from_le_bytes(word))?;
        }
    } else {
        loge!("Privacy operation failed: {}", result);
    }
    Ok(())
}
