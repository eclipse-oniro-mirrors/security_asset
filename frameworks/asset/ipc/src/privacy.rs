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

//! Typed PSI/PIR DTOs and the same field-by-field Parcel pattern as Asset maps.
use crate::{ipc_err_handle, macros_lib, ErrCode, MsgParcel, Result};

/// Per-operation resource budget; large messages are transported through shared memory.
pub const MAX_BYTES: usize = 64 * 1024 * 1024;
const LENGTH_BYTES: usize = std::mem::size_of::<u32>();
const FLAG_BYTES: usize = std::mem::size_of::<u32>();
const PROTOCOL_BYTES: usize = std::mem::size_of::<u32>() + std::mem::size_of::<u32>();
/// Logical size of the protocol and hash fields; this is not the native request struct size.
pub const TARGET_FIXED_BYTES: usize = PROTOCOL_BYTES + std::mem::size_of::<u32>();
const SEARCH_FIXED_BYTES: usize = PROTOCOL_BYTES + FLAG_BYTES;
const RESULT_FIXED_BYTES: usize = FLAG_BYTES + FLAG_BYTES;
/// Logical target request overhead, including the element length field.
pub const TARGET_OVERHEAD_BYTES: usize = TARGET_FIXED_BYTES + LENGTH_BYTES;
/// Independent bound on list descriptors, including lists containing only empty values.
pub const MAX_ITEMS: usize = 65536;
/// Maximum target ciphertext result, independent of the search request resource budget.
pub const MAX_TARGET_RESULT_BYTES: usize = 256 * 1024;

mod transport;
pub use transport::{read_payload, write_payload};
/// Protocol operation success code, separate from the outer Asset IPC status.
pub const PROTOCOL_SUCCESS: i32 = 0;

/// Borrowed byte range for same-process C/Rust calls; pointers are never serialized into Parcel.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyBlob {
    /// Readable byte storage; may be null only when size is zero.
    pub data: *const u8,
    /// Number of initialized bytes available at data.
    pub size: u32,
}

/// Borrowed list of byte ranges for same-process C/Rust calls.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyBlobList {
    /// Aligned descriptor storage; may be null only when count is zero.
    pub data: *const PrivacyBlob,
    /// Number of initialized descriptors available at data.
    pub count: u32,
}

/// C-compatible protocol selectors, using the values defined by the protocol library.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyProtocol {
    /// Data set size enum value.
    pub data_set_size: u32,
    /// PSI or PIR protocol enum value.
    pub protocol_type: u32,
}

/// Borrowed target generation request; its buffers remain owned by the caller.
#[repr(C)]
pub struct PrivacyTargetRequest {
    /// Protocol selectors for target generation.
    pub protocol: PrivacyProtocol,
    /// Hash algorithm enum value; zero selects NONE.
    pub hash_alg: u32,
    /// Plaintext target bytes, borrowed until the synchronous call returns.
    pub element: PrivacyBlob,
}

/// Borrowed encrypted search result to be decrypted and processed.
#[repr(C)]
pub struct PrivacySearchRequest {
    /// Protocol selectors used to generate the encrypted search result.
    pub protocol: PrivacyProtocol,
    /// Encrypted HE batches in their original order.
    pub ciphertexts: PrivacyBlobList,
    /// Presence flag: one includes symmetric, zero means it is absent.
    pub has_symmetric: u32,
    /// Complete associated-value frames, used only when has_symmetric is one.
    pub symmetric: PrivacyBlobList,
}

/// Caller-owned output buffer for a synchronous native call.
#[repr(C)]
pub struct PrivacyBuffer {
    /// Writable storage that remains valid until the call returns.
    pub data: *mut u8,
    /// Writable capacity on entry; actual byte length after a successful call.
    pub size: u32,
}

/// Synchronous same-process result receiver; the writer copies bytes before returning.
#[repr(C)]
pub struct PrivacyOutput {
    /// Caller-owned output context; never serialized or passed across processes.
    pub context: *mut std::ffi::c_void,
    /// Borrows data only for the duration of this callback. Zero means success.
    pub write: Option<unsafe extern "C" fn(*mut std::ffi::c_void, *const u8, u32) -> i32>,
}

/// Copies a synchronous result into a caller-owned Vec; no allocation ownership crosses FFI.
///
/// # Safety
/// context must point to an exclusively borrowed Vec<u8>; data must be readable for size bytes
/// and must not alias that Vec. Both ranges must remain valid until this function returns.
pub unsafe extern "C" fn collect_privacy_bytes(context: *mut std::ffi::c_void, data: *const u8, size: u32) -> i32 {
    if context.is_null() || data.is_null() || size == 0 || size as usize > MAX_BYTES {
        return 1;
    }
    let output = &mut *(context as *mut Vec<u8>);
    *output = std::slice::from_raw_parts(data, size as usize).to_vec();
    PROTOCOL_SUCCESS
}

/// Owned target generation fields used by the Rust Parcel serializer.
pub struct TargetRequest {
    /// Protocol selectors for target generation.
    pub protocol: PrivacyProtocol,
    /// Hash algorithm enum value; zero selects NONE.
    pub hash_alg: u32,
    /// Plaintext target bytes.
    pub element: Vec<u8>,
}

/// Owned encrypted batches and optional associated-value frames for a search result request.
pub struct SearchRequest {
    /// Protocol selectors used by the encrypted search result.
    pub protocol: PrivacyProtocol,
    /// Encrypted HE batches in their original order.
    pub ciphertexts: Vec<Vec<u8>>,
    /// Optional complete associated-value frames; Some(empty) differs from None.
    pub symmetric: Option<Vec<Vec<u8>>>,
}

/// Owned result decoded from a caller-owned native buffer or a service reply.
pub struct SearchResponse {
    /// Whether any item matched the target.
    pub matched: bool,
    /// Optional decrypted associated values; Some(empty) differs from None.
    pub values: Option<Vec<Vec<u8>>>,
}

fn invalid<T>() -> Result<T> {
    macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
        ErrCode::InvalidArgument, "Invalid privacy request size or fields")
}

fn charge(total: &mut usize, size: usize) -> Result<()> {
    if size > MAX_BYTES || *total > MAX_BYTES - size {
        return invalid();
    }
    *total += size;
    Ok(())
}

/// Builds borrowed byte descriptors; the vectors and their allocations must outlive use of the descriptors.
/// Values must already satisfy the IPC byte budget so their lengths fit in u32.
pub fn blobs(values: &[Vec<u8>]) -> Vec<PrivacyBlob> {
    values.iter().map(|value| PrivacyBlob { data: value.as_ptr(), size: value.len() as u32 }).collect()
}

/// Copies one native range after checking its declared length against the logical byte budget.
///
/// # Safety
/// A nonempty range must remain readable and unchanged for blob.size bytes until this call returns.
unsafe fn copy_blob(blob: PrivacyBlob, total: &mut usize) -> Result<Vec<u8>> {
    charge(total, LENGTH_BYTES)?;
    charge(total, blob.size as usize)?;
    if blob.size == 0 {
        return Ok(Vec::new());
    }
    if blob.data.is_null() {
        return invalid();
    }
    Ok(std::slice::from_raw_parts(blob.data, blob.size as usize).to_vec())
}

/// Copies native descriptors and byte ranges while accumulating their combined logical size.
///
/// # Safety
/// A nonempty list must contain count aligned, initialized PrivacyBlob descriptors. Every
/// nonempty range must remain readable and unchanged for its declared size until this call returns.
unsafe fn copy_list(list: PrivacyBlobList, total: &mut usize) -> Result<Vec<Vec<u8>>> {
    charge(total, LENGTH_BYTES)?;
    if list.count as usize > MAX_ITEMS {
        return invalid();
    }
    if list.count == 0 {
        return Ok(Vec::new());
    }
    if list.data.is_null() {
        return invalid();
    }
    let mut values = Vec::new();
    for blob in std::slice::from_raw_parts(list.data, list.count as usize) {
        values.push(copy_blob(*blob, total)?);
    }
    Ok(values)
}

impl PrivacyTargetRequest {
    /// Copies borrowed target bytes into an owned request after checking the byte budget.
    ///
    /// # Safety
    /// Nonempty element storage must be readable and initialized for element.size bytes.
    /// It must remain allocated and must not be mutated for the duration of this call.
    pub unsafe fn to_value(&self) -> Result<TargetRequest> {
        let mut total = TARGET_FIXED_BYTES;
        let element = copy_blob(self.element, &mut total)?;
        Ok(TargetRequest {
            protocol: self.protocol,
            hash_alg: self.hash_alg,
            element,
        })
    }
}

impl PrivacySearchRequest {
    /// Copies encrypted batches and any present associated-value frames into an owned request.
    ///
    /// # Safety
    /// Nonempty lists must point to aligned, initialized arrays of count PrivacyBlob descriptors.
    /// Every nonempty byte range must be readable for its declared size. Lists and byte storage
    /// must remain allocated and must not be mutated until this call returns.
    pub unsafe fn to_value(&self) -> Result<SearchRequest> {
        if self.has_symmetric > 1 {
            return invalid();
        }
        let mut total = SEARCH_FIXED_BYTES;
        let ciphertexts = copy_list(self.ciphertexts, &mut total)?;
        let symmetric = if self.has_symmetric == 1 {
            Some(copy_list(self.symmetric, &mut total)?)
        } else {
            None
        };
        Ok(SearchRequest { protocol: self.protocol, ciphertexts, symmetric })
    }
}

/// Encodes a native result as little-endian u32 flags, followed (when values are present)
/// by a u32 count and repeated u32 length + bytes. The complete buffer is limited to MAX_BYTES.
pub fn encode_result_bytes(value: &SearchResponse) -> Result<Vec<u8>> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&(value.matched as u32).to_le_bytes());
    bytes.extend_from_slice(&(value.values.is_some() as u32).to_le_bytes());
    let mut total = RESULT_FIXED_BYTES;
    if let Some(values) = &value.values {
        if values.len() > MAX_ITEMS {
            return invalid();
        }
        charge(&mut total, LENGTH_BYTES)?;
        bytes.extend_from_slice(&(values.len() as u32).to_le_bytes());
        for value in values {
            charge(&mut total, LENGTH_BYTES)?;
            charge(&mut total, value.len())?;
            bytes.extend_from_slice(&(value.len() as u32).to_le_bytes());
            bytes.extend_from_slice(value);
        }
    }
    Ok(bytes)
}

fn read_result_word(bytes: &[u8], offset: &mut usize) -> Result<u32> {
    if bytes.len() - *offset < LENGTH_BYTES {
        return invalid();
    }
    let mut word = [0u8; LENGTH_BYTES];
    word.copy_from_slice(&bytes[*offset..*offset + LENGTH_BYTES]);
    *offset += LENGTH_BYTES;
    Ok(u32::from_le_bytes(word))
}

/// Decodes the same-process FFI result buffer, preserving None versus Some(empty).
/// Rejects invalid flags, oversized counts, truncation and trailing bytes.
pub fn decode_result_bytes(bytes: &[u8]) -> Result<SearchResponse> {
    if bytes.len() > MAX_BYTES {
        return invalid();
    }
    let mut offset = 0;
    let matched = read_result_word(bytes, &mut offset)?;
    let has_values = read_result_word(bytes, &mut offset)?;
    if matched > 1 || has_values > 1 {
        return invalid();
    }
    let values = if has_values == 1 {
        let count = read_result_word(bytes, &mut offset)? as usize;
        if count > MAX_ITEMS || count > (bytes.len() - offset) / LENGTH_BYTES {
            return invalid();
        }
        let mut values = Vec::new();
        for _ in 0..count {
            let length = read_result_word(bytes, &mut offset)? as usize;
            if length > bytes.len() - offset {
                return invalid();
            }
            values.push(bytes[offset..offset + length].to_vec());
            offset += length;
        }
        Some(values)
    } else {
        None
    };
    if offset != bytes.len() {
        return invalid();
    }
    Ok(SearchResponse { matched: matched == 1, values })
}

impl TargetRequest {
    /// Borrows this request as a C-compatible view; keep element allocated and unchanged while it is used.
    /// The element length must already satisfy the IPC byte budget before conversion to u32.
    pub fn as_native(&self) -> PrivacyTargetRequest {
        PrivacyTargetRequest {
            protocol: self.protocol,
            hash_alg: self.hash_alg,
            element: PrivacyBlob { data: self.element.as_ptr(), size: self.element.len() as u32 },
        }
    }
}

impl SearchRequest {
    /// Creates a C-compatible request borrowing the supplied descriptor arrays.
    /// Descriptors must represent this request's batches and frames and satisfy the IPC size budget.
    /// Keep both arrays and all referenced byte allocations alive and unchanged while the view is used.
    pub fn as_native(&self, ciphertexts: &[PrivacyBlob], symmetric: &[PrivacyBlob]) -> PrivacySearchRequest {
        PrivacySearchRequest {
            protocol: self.protocol,
            ciphertexts: PrivacyBlobList { data: ciphertexts.as_ptr(), count: ciphertexts.len() as u32 },
            has_symmetric: self.symmetric.is_some() as u32,
            symmetric: PrivacyBlobList { data: symmetric.as_ptr(), count: symmetric.len() as u32 },
        }
    }
}

fn write_protocol(value: PrivacyProtocol, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write(&value.data_set_size).map_err(ipc_err_handle)?;
    parcel.write(&value.protocol_type).map_err(ipc_err_handle)
}

fn read_protocol(parcel: &mut MsgParcel) -> Result<PrivacyProtocol> {
    Ok(PrivacyProtocol {
        data_set_size: parcel.read::<u32>().map_err(ipc_err_handle)?,
        protocol_type: parcel.read::<u32>().map_err(ipc_err_handle)?,
    })
}

fn write_bytes(value: &Vec<u8>, total: &mut usize, parcel: &mut MsgParcel) -> Result<()> {
    charge(total, LENGTH_BYTES)?;
    charge(total, value.len())?;
    write_payload(value, parcel)
}

fn read_bytes(total: &mut usize, parcel: &mut MsgParcel) -> Result<Vec<u8>> {
    let value = read_payload(parcel)?;
    charge(total, LENGTH_BYTES)?;
    charge(total, value.len())?;
    Ok(value)
}

/// Writes target fields in the shared Parcel order, rejecting requests over the byte budget.
pub fn serialize_target(value: &TargetRequest, parcel: &mut MsgParcel) -> Result<()> {
    write_protocol(value.protocol, parcel)?;
    parcel.write(&value.hash_alg).map_err(ipc_err_handle)?;
    let mut total = TARGET_FIXED_BYTES;
    write_bytes(&value.element, &mut total, parcel)
}

/// Reads target fields from Parcel and validates their combined logical byte budget.
pub fn deserialize_target(parcel: &mut MsgParcel) -> Result<TargetRequest> {
    let mut total = TARGET_FIXED_BYTES;
    Ok(TargetRequest {
        protocol: read_protocol(parcel)?,
        hash_alg: parcel.read::<u32>().map_err(ipc_err_handle)?,
        element: read_bytes(&mut total, parcel)?,
    })
}

/// Writes one complete request through inline storage or read-only shared memory.
pub fn serialize_search(value: &SearchRequest, parcel: &mut MsgParcel) -> Result<()> {
    let bytes = encode_search_bytes(value)?;
    write_payload(&bytes, parcel)
}

/// Reads encrypted batches and optional frames, rejecting invalid counts or oversized fields.
pub fn deserialize_search(parcel: &mut MsgParcel) -> Result<SearchRequest> {
    decode_search_bytes(&read_payload(parcel)?)
}

/// Writes match status and optional values, preserving absent and present empty lists.
pub fn serialize_result(value: &SearchResponse, parcel: &mut MsgParcel) -> Result<()> {
    write_payload(&encode_result_bytes(value)?, parcel)
}

/// Reads an inline or shared-memory result and validates the complete encoded snapshot.
pub fn deserialize_result(parcel: &mut MsgParcel) -> Result<SearchResponse> {
    decode_result_bytes(&read_payload(parcel)?)
}

fn append_list(values: &[Vec<u8>], bytes: &mut Vec<u8>) -> Result<()> {
    if values.len() > MAX_ITEMS {
        return invalid();
    }
    let mut total = bytes.len();
    charge(&mut total, LENGTH_BYTES)?;
    bytes.extend_from_slice(&(values.len() as u32).to_le_bytes());
    for value in values {
        charge(&mut total, LENGTH_BYTES)?;
        charge(&mut total, value.len())?;
        bytes.extend_from_slice(&(value.len() as u32).to_le_bytes());
        bytes.extend_from_slice(value);
    }
    Ok(())
}

fn parse_list(bytes: &[u8], offset: &mut usize) -> Result<Vec<Vec<u8>>> {
    let count = read_result_word(bytes, offset)? as usize;
    if count > MAX_ITEMS || count > (bytes.len() - *offset) / LENGTH_BYTES {
        return invalid();
    }
    let mut values = Vec::new();
    for _ in 0..count {
        let len = read_result_word(bytes, offset)? as usize;
        if len > bytes.len() - *offset {
            return invalid();
        }
        values.push(bytes[*offset..*offset + len].to_vec());
        *offset += len;
    }
    Ok(values)
}

/// Encodes the complete request into a bounded, little-endian field stream without native pointers.
pub fn encode_search_bytes(value: &SearchRequest) -> Result<Vec<u8>> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&value.protocol.data_set_size.to_le_bytes());
    bytes.extend_from_slice(&value.protocol.protocol_type.to_le_bytes());
    bytes.extend_from_slice(&(value.symmetric.is_some() as u32).to_le_bytes());
    append_list(&value.ciphertexts, &mut bytes)?;
    if let Some(values) = &value.symmetric {
        append_list(values, &mut bytes)?;
    }
    Ok(bytes)
}

/// Validates a complete request snapshot, including counts, lengths, flags and trailing data.
pub fn decode_search_bytes(bytes: &[u8]) -> Result<SearchRequest> {
    if bytes.len() > MAX_BYTES {
        return invalid();
    }
    let mut offset = 0;
    let protocol = PrivacyProtocol {
        data_set_size: read_result_word(bytes, &mut offset)?,
        protocol_type: read_result_word(bytes, &mut offset)?,
    };
    let flag = read_result_word(bytes, &mut offset)?;
    if flag > 1 {
        return invalid();
    }
    let ciphertexts = parse_list(bytes, &mut offset)?;
    let symmetric = if flag == 1 { Some(parse_list(bytes, &mut offset)?) } else { None };
    if offset != bytes.len() {
        return invalid();
    }
    Ok(SearchRequest { protocol, ciphertexts, symmetric })
}
