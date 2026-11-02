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

//! Asset Stub has checked the interface token and protects this single IPC call's lifetime.
use asset_ipc::privacy::*;
use asset_ipc::{IpcCode, IPC_SUCCESS};
use asset_log::{loge, logi};
use ipc::{parcel::MsgParcel, IpcResult, IpcStatusCode, Skeleton};
use std::convert::TryFrom;

extern "C" {
    fn AssetGenPrivacyTarget(uid: u32, input: *const PrivacyTargetRequest, output: *mut PrivacyBuffer) -> i32;
    fn AssetDecryptPrivacyCiphertext(uid: u32, input: *const PrivacyDecryptRequest, output: *mut PrivacyBuffer) -> i32;
}

fn gen_privacy_target(uid: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    logi!("PRIVACY_COMPUTATION Stub genPrivacyTarget received");
    let input = deserialize_target(data).map_err(|_| {
        loge!("PRIVACY_COMPUTATION Stub genPrivacyTarget deserialize failed");
        IpcStatusCode::InvalidValue
    })?;
    let request = input.as_native();
    let mut bytes = vec![0u8; MAX_TARGET_RESULT_BYTES];
    let mut output = PrivacyBuffer { data: bytes.as_mut_ptr(), size: bytes.len() as u32 };
    let result = unsafe { AssetGenPrivacyTarget(uid, &request, &mut output) };
    if output.size as usize > bytes.len() {
        loge!("PRIVACY_COMPUTATION Stub service output exceeds capacity");
        return Err(IpcStatusCode::Failed);
    }
    reply.write(&IPC_SUCCESS)?;
    reply.write(&result)?;
    if result == PROTOCOL_SUCCESS {
        bytes.truncate(output.size as usize);
        write_payload(&bytes, reply).map_err(|_| IpcStatusCode::Failed)?;
        logi!("PRIVACY_COMPUTATION Stub genPrivacyTarget reply written, bytes={}", bytes.len());
    } else {
        loge!("PRIVACY_COMPUTATION Stub genPrivacyTarget failed, ret={}", result);
    }
    Ok(())
}

fn decrypt_privacy_ciphertext(uid: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let input = deserialize_decrypt(data).map_err(|_| {
        loge!("PRIVACY_COMPUTATION Stub decryptCiphertext deserialize failed");
        IpcStatusCode::InvalidValue
    })?;
    logi!("PRIVACY_COMPUTATION Stub decryptCiphertext begin, bytes={}", input.ciphertext.len());
    let request = input.as_native();
    let mut bytes = vec![0u8; MAX_DECRYPT_RESULT_BYTES];
    let mut output = PrivacyBuffer { data: bytes.as_mut_ptr(), size: bytes.len() as u32 };
    let status = unsafe { AssetDecryptPrivacyCiphertext(uid, &request, &mut output) };
    if status == PROTOCOL_SUCCESS && output.size as usize != MAX_DECRYPT_RESULT_BYTES {
        loge!("PRIVACY_COMPUTATION Stub decryptCiphertext invalid plaintext length={}", output.size);
        return Err(IpcStatusCode::Failed);
    }
    reply.write(&IPC_SUCCESS)?;
    reply.write(&status)?;
    if status == PROTOCOL_SUCCESS {
        bytes.truncate(output.size as usize);
        write_payload(&bytes, reply).map_err(|_| IpcStatusCode::Failed)?;
        logi!("PRIVACY_COMPUTATION Stub decryptCiphertext success, bytes={}", bytes.len());
    } else {
        loge!("PRIVACY_COMPUTATION Stub decryptCiphertext failed, ret={}", status);
    }
    Ok(())
}

/// Captures the actual Binder UID separately for every single-batch operation.
pub(crate) fn request(code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let uid = u32::try_from(Skeleton::calling_uid()).map_err(|_| IpcStatusCode::Failed)?;
    if code == IpcCode::PrivacyGenTarget as u32 {
        return gen_privacy_target(uid, data, reply);
    }
    if code == IpcCode::PrivacyDecryptCiphertext as u32 {
        return decrypt_privacy_ciphertext(uid, data, reply);
    }
    Err(IpcStatusCode::InvalidValue)
}
