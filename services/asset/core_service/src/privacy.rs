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

//! Asset Stub has already checked the interface token and protected service lifetime.
use asset_ipc::privacy::*;
use asset_ipc::{IpcCode, IPC_SUCCESS};
use asset_log::{loge, logi};
use ipc::{parcel::MsgParcel, IpcResult, IpcStatusCode, Skeleton};
use std::convert::TryFrom;

extern "C" {
    fn AssetGenPrivacyTarget(uid: u32, input: *const PrivacyTargetRequest, output: *mut PrivacyBuffer) -> i32;
    fn AssetGetPrivacyResult(uid: u32, input: *const PrivacySearchRequest, output: *mut PrivacyBuffer) -> i32;
}

fn gen_privacy_target(uid: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    logi!("HPIC Stub genPrivacyTarget received");
    let input = deserialize_target(data).map_err(|_| {
        loge!("HPIC Stub genPrivacyTarget deserialize failed");
        IpcStatusCode::InvalidValue
    })?;
    let request = input.as_native();
    let mut bytes = vec![0u8; MAX_BYTES];
    let mut output = PrivacyBuffer { data: bytes.as_mut_ptr(), size: bytes.len() as u32 };
    let result = unsafe { AssetGenPrivacyTarget(uid, &request, &mut output) };
    if output.size as usize > bytes.len() {
        loge!("HPIC Stub service output exceeds capacity");
        return Err(IpcStatusCode::Failed);
    }
    reply.write(&IPC_SUCCESS)?;
    reply.write(&result)?;
    if result == PROTOCOL_SUCCESS {
        bytes.truncate(output.size as usize);
        reply.write::<Vec<u8>>(&bytes)?;
        logi!("HPIC Stub genPrivacyTarget reply written, bytes={}", bytes.len());
    } else {
        loge!("HPIC Stub genPrivacyTarget failed, ret={}", result);
    }
    Ok(())
}

fn get_privacy_result(uid: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    logi!("HPIC Stub getSearchResult received");
    let input = deserialize_search(data).map_err(|_| {
        loge!("HPIC Stub getSearchResult deserialize failed");
        IpcStatusCode::InvalidValue
    })?;
    let ciphertexts = blobs(&input.ciphertexts);
    let symmetric = blobs(input.symmetric.as_deref().unwrap_or(&[]));
    let request = input.as_native(&ciphertexts, &symmetric);
    let mut bytes = vec![0u8; MAX_BYTES];
    let mut output = PrivacyBuffer { data: bytes.as_mut_ptr(), size: bytes.len() as u32 };
    let result = unsafe { AssetGetPrivacyResult(uid, &request, &mut output) };
    if output.size as usize > bytes.len() {
        loge!("HPIC Stub service output exceeds capacity");
        return Err(IpcStatusCode::Failed);
    }
    reply.write(&IPC_SUCCESS)?;
    reply.write(&result)?;
    if result == PROTOCOL_SUCCESS {
        bytes.truncate(output.size as usize);
        let response = decode_result_bytes(&bytes).map_err(|_| {
            loge!("HPIC Stub getSearchResult native result decode failed");
            IpcStatusCode::InvalidValue
        })?;
        serialize_result(&response, reply).map_err(|_| {
            loge!("HPIC Stub getSearchResult reply serialization failed");
            IpcStatusCode::InvalidValue
        })?;
        logi!("HPIC Stub getSearchResult reply written, bytes={}", bytes.len());
    } else {
        loge!("HPIC Stub getSearchResult failed, ret={}", result);
    }
    Ok(())
}
/// Dispatches a privacy request on the Binder thread after capturing the actual calling UID.
pub(crate) fn request(code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let uid = u32::try_from(Skeleton::calling_uid()).map_err(|_| IpcStatusCode::Failed)?;
    if code == IpcCode::PrivacyGenTarget as u32 {
        return gen_privacy_target(uid, data, reply);
    }
    if code == IpcCode::PrivacyGetResult as u32 {
        return get_privacy_result(uid, data, reply);
    }
    Err(IpcStatusCode::InvalidValue)
}
