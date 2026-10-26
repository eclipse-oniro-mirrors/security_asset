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
use asset_log::loge;
use ipc::{parcel::MsgParcel, IpcResult, IpcStatusCode, Skeleton};
use std::convert::TryFrom;
use std::ptr::{null, null_mut};

extern "C" {
    fn AssetGenPrivacyTarget(uid: u32, input: *const PrivacyTargetRequest, output: *mut PrivacyBuffer) -> i32;
    fn AssetGetPrivacyResult(uid: u32, input: *const PrivacySearchRequest, output: *mut PrivacyResult) -> i32;
    fn AssetFreePrivacyResult(output: *mut PrivacyResult);
}

struct NativeResult(PrivacyResult);
impl Drop for NativeResult {
    fn drop(&mut self) {
        unsafe {
            AssetFreePrivacyResult(&mut self.0);
        }
    }
}

fn generate(uid: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let input = deserialize_target(data).map_err(|_| IpcStatusCode::InvalidValue)?;
    let request = input.as_native();
    let mut bytes = vec![0u8; MAX_BYTES];
    let mut output = PrivacyBuffer { data: bytes.as_mut_ptr(), size: bytes.len() as u32 };
    let result = unsafe { AssetGenPrivacyTarget(uid, &request, &mut output) };
    if output.size as usize > bytes.len() {
        return Err(IpcStatusCode::Failed);
    }
    reply.write(&IPC_SUCCESS)?;
    reply.write(&result)?;
    if result == PROTOCOL_SUCCESS {
        bytes.truncate(output.size as usize);
        reply.write::<Vec<u8>>(&bytes)?;
    } else {
        loge!("Generate privacy target failed: {}", result);
    }
    Ok(())
}

fn get_result(uid: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let input = deserialize_search(data).map_err(|_| IpcStatusCode::InvalidValue)?;
    let ciphertexts = blobs(&input.ciphertexts);
    let symmetric = blobs(input.symmetric.as_deref().unwrap_or(&[]));
    let request = input.as_native(&ciphertexts, &symmetric);
    let mut output = NativeResult(PrivacyResult {
        matched: 0,
        has_values: 0,
        values: PrivacyBlobList { data: null(), count: 0 },
        storage: null_mut(),
    });
    let result = unsafe { AssetGetPrivacyResult(uid, &request, &mut output.0) };
    reply.write(&IPC_SUCCESS)?;
    reply.write(&result)?;
    if result == PROTOCOL_SUCCESS {
        let response = unsafe { output.0.to_value() }.map_err(|_| IpcStatusCode::InvalidValue)?;
        serialize_result(&response, reply).map_err(|_| IpcStatusCode::InvalidValue)?;
    } else {
        loge!("Get privacy result failed: {}", result);
    }
    Ok(())
}
/// Dispatches a privacy request on the Binder thread after capturing the actual calling UID.
pub(crate) fn request(code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let uid = u32::try_from(Skeleton::calling_uid()).map_err(|_| IpcStatusCode::Failed)?;
    if code == IpcCode::PrivacyGenTarget as u32 {
        return generate(uid, data, reply);
    }
    if code == IpcCode::PrivacyGetResult as u32 {
        return get_result(uid, data, reply);
    }
    Err(IpcStatusCode::InvalidValue)
}
