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

//! Single-target generation and single-ciphertext decryption over ordinary Asset IPC.
use crate::{ipc_err_handle, macros_lib, ErrCode, MsgParcel, Result};

use std::convert::TryFrom;

/// Capacity of the caller-owned buffer for one encrypted target reply.
pub const MAX_TARGET_RESULT_BYTES: usize = 192 * 1024;
/// BGV-2048 decoded slots, encoded as little-endian u64 values.
pub const MAX_DECRYPT_RESULT_BYTES: usize = 2048 * std::mem::size_of::<u64>();
/// Protocol operation success, separate from the outer Asset IPC status.
pub const PROTOCOL_SUCCESS: i32 = 0;

/// Same-process borrowed bytes; pointers are never serialized into Parcel.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyBlob {
    /// Readable bytes; may be null only when size is zero.
    pub data: *const u8,
    /// Number of readable bytes.
    pub size: u32,
}

/// Protocol selectors shared with the C ABI.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyProtocol {
    /// Data set size enum value.
    pub data_set_size: u32,
    /// PSI or PIR enum value.
    pub protocol_type: u32,
}

/// Borrowed target request for the same-process C/Rust boundary.
#[repr(C)]
pub struct PrivacyTargetRequest {
    /// Protocol selectors.
    pub protocol: PrivacyProtocol,
    /// Hash algorithm; zero means NONE.
    pub hash_alg: u32,
    /// Target element bytes.
    pub element: PrivacyBlob,
}

/// Exactly one HE ciphertext; no associated-value fields exist in this ABI.
#[repr(C)]
pub struct PrivacyDecryptRequest {
    /// Protocol selectors.
    pub protocol: PrivacyProtocol,
    /// One independently decryptable ciphertext batch.
    pub ciphertext: PrivacyBlob,
}

/// Caller-owned output; no allocation ownership crosses FFI.
#[repr(C)]
pub struct PrivacyBuffer {
    /// Exclusively writable caller storage.
    pub data: *mut u8,
    /// Writable capacity on entry; actual length on success.
    pub size: u32,
}

/// Owned target fields used by the Parcel serializer.
pub struct TargetRequest {
    /// Protocol selectors.
    pub protocol: PrivacyProtocol,
    /// Hash algorithm; zero means NONE.
    pub hash_alg: u32,
    /// Plain target element.
    pub element: Vec<u8>,
}

/// Owned single-batch HE decryption request.
pub struct DecryptRequest {
    /// Protocol selectors.
    pub protocol: PrivacyProtocol,
    /// One HE ciphertext; symmetric ciphertexts remain in the caller process.
    pub ciphertext: Vec<u8>,
}

fn invalid<T>() -> Result<T> {
    macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
        ErrCode::InvalidArgument, "Invalid privacy single-message request")
}

impl TargetRequest {
    /// Borrows target bytes; storage must outlive the native call and lengths must fit in u32.
    pub fn as_native(&self) -> PrivacyTargetRequest {
        PrivacyTargetRequest { protocol: self.protocol, hash_alg: self.hash_alg,
            element: PrivacyBlob { data: self.element.as_ptr(), size: self.element.len() as u32 } }
    }
}

impl DecryptRequest {
    /// Borrows one ciphertext; storage must outlive the native call and length must fit in u32.
    pub fn as_native(&self) -> PrivacyDecryptRequest {
        PrivacyDecryptRequest { protocol: self.protocol,
            ciphertext: PrivacyBlob { data: self.ciphertext.as_ptr(), size: self.ciphertext.len() as u32 } }
    }
}

/// Writes a length-prefixed inline field; never creates shared memory or transfer tasks.
pub fn write_payload(bytes: &[u8], parcel: &mut MsgParcel) -> Result<()> {
    let size = match u32::try_from(bytes.len()) {
        Ok(size) => size,
        Err(_) => return invalid(),
    };
    parcel.write(&size).map_err(ipc_err_handle)?;
    if !bytes.is_empty() {
        parcel.write_buffer(bytes).map_err(ipc_err_handle)?;
    }
    Ok(())
}

/// Reads an inline field, validating the declared size before copying from Parcel.
pub fn read_payload(parcel: &mut MsgParcel) -> Result<Vec<u8>> {
    let size = parcel.read::<u32>().map_err(ipc_err_handle)? as usize;
    if size > parcel.readable() {
        return invalid();
    }
    if size == 0 { return Ok(Vec::new()); }
    parcel.read_buffer(size).map_err(ipc_err_handle)
}

fn write_protocol(protocol: PrivacyProtocol, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write(&protocol.data_set_size).map_err(ipc_err_handle)?;
    parcel.write(&protocol.protocol_type).map_err(ipc_err_handle)
}

fn read_protocol(parcel: &mut MsgParcel) -> Result<PrivacyProtocol> {
    Ok(PrivacyProtocol { data_set_size: parcel.read::<u32>().map_err(ipc_err_handle)?,
        protocol_type: parcel.read::<u32>().map_err(ipc_err_handle)? })
}

/// Writes one target request in the Asset Parcel field order.
pub fn serialize_target(value: &TargetRequest, parcel: &mut MsgParcel) -> Result<()> {
    write_protocol(value.protocol, parcel)?;
    parcel.write(&value.hash_alg).map_err(ipc_err_handle)?;
    write_payload(&value.element, parcel)
}

/// Reads one target request in the Asset Parcel field order.
pub fn deserialize_target(parcel: &mut MsgParcel) -> Result<TargetRequest> {
    let protocol = read_protocol(parcel)?;
    let hash_alg = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let element = read_payload(parcel)?;
    Ok(TargetRequest { protocol, hash_alg, element })
}

/// Writes exactly one HE ciphertext; there is no symmetric ciphertext field.
pub fn serialize_decrypt(value: &DecryptRequest, parcel: &mut MsgParcel) -> Result<()> {
    if value.ciphertext.is_empty() {
        return invalid();
    }
    write_protocol(value.protocol, parcel)?;
    write_payload(&value.ciphertext, parcel)
}

/// Reads one independently decryptable HE batch.
pub fn deserialize_decrypt(parcel: &mut MsgParcel) -> Result<DecryptRequest> {
    let protocol = read_protocol(parcel)?;
    let ciphertext = read_payload(parcel)?;
    if ciphertext.is_empty() {
        return invalid();
    }
    Ok(DecryptRequest { protocol, ciphertext })
}
