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
use std::ffi::c_void;

// Logical field budget; Parcel alignment and the interface token are additional overhead.
pub const MAX_BYTES: usize = 256 * 1024;
const LENGTH_BYTES: usize = std::mem::size_of::<u32>();
const FLAG_BYTES: usize = std::mem::size_of::<u32>();
const PROTOCOL_BYTES: usize = std::mem::size_of::<u32>() + std::mem::size_of::<u32>();
// Same logical field budget as privacy_limits.h; these are not native struct sizes.
pub const TARGET_FIXED_BYTES: usize = PROTOCOL_BYTES + std::mem::size_of::<u32>();
const SEARCH_FIXED_BYTES: usize = PROTOCOL_BYTES + FLAG_BYTES;
const RESULT_FIXED_BYTES: usize = FLAG_BYTES + FLAG_BYTES;
pub const TARGET_OVERHEAD_BYTES: usize = TARGET_FIXED_BYTES + LENGTH_BYTES;
pub const MAX_ITEMS: usize = MAX_BYTES / LENGTH_BYTES;
pub const PROTOCOL_SUCCESS: i32 = 0;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyBlob {
    pub data: *const u8,
    pub size: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyBlobList {
    pub data: *const PrivacyBlob,
    pub count: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct PrivacyProtocol {
    pub data_set_size: u32,
    pub protocol_type: u32,
}

#[repr(C)]
pub struct PrivacyTargetRequest {
    pub protocol: PrivacyProtocol,
    pub hash_alg: u32,
    pub element: PrivacyBlob,
}

#[repr(C)]
pub struct PrivacySearchRequest {
    pub protocol: PrivacyProtocol,
    pub ciphertexts: PrivacyBlobList,
    pub has_symmetric: u32,
    pub symmetric: PrivacyBlobList,
}

#[repr(C)]
pub struct PrivacyBuffer {
    pub data: *mut u8,
    pub size: u32,
}

#[repr(C)]
pub struct PrivacyResult {
    pub matched: u32,
    pub has_values: u32,
    pub values: PrivacyBlobList,
    pub storage: *mut c_void,
}

pub struct TargetRequest {
    pub protocol: PrivacyProtocol,
    pub hash_alg: u32,
    pub element: Vec<u8>,
}

pub struct SearchRequest {
    pub protocol: PrivacyProtocol,
    pub ciphertexts: Vec<Vec<u8>>,
    pub symmetric: Option<Vec<Vec<u8>>>,
}

pub struct SearchResponse {
    pub matched: bool,
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

pub fn blobs(values: &[Vec<u8>]) -> Vec<PrivacyBlob> {
    values.iter().map(|value| PrivacyBlob { data: value.as_ptr(), size: value.len() as u32 }).collect()
}

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

impl PrivacyResult {
    pub unsafe fn to_value(&self) -> Result<SearchResponse> {
        if self.matched > 1 || self.has_values > 1 {
            return invalid();
        }
        let mut total = RESULT_FIXED_BYTES;
        let values = if self.has_values == 1 {
            Some(copy_list(self.values, &mut total)?)
        } else {
            None
        };
        Ok(SearchResponse { matched: self.matched == 1, values })
    }
}

impl TargetRequest {
    pub fn as_native(&self) -> PrivacyTargetRequest {
        PrivacyTargetRequest {
            protocol: self.protocol,
            hash_alg: self.hash_alg,
            element: PrivacyBlob { data: self.element.as_ptr(), size: self.element.len() as u32 },
        }
    }
}

impl SearchRequest {
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
    parcel.write::<Vec<u8>>(value).map_err(ipc_err_handle)
}

fn read_bytes(total: &mut usize, parcel: &mut MsgParcel) -> Result<Vec<u8>> {
    let value = parcel.read::<Vec<u8>>().map_err(ipc_err_handle)?;
    charge(total, LENGTH_BYTES)?;
    charge(total, value.len())?;
    Ok(value)
}

fn write_list(values: &Vec<Vec<u8>>, total: &mut usize, parcel: &mut MsgParcel) -> Result<()> {
    charge(total, LENGTH_BYTES)?;
    if values.len() > MAX_ITEMS {
        return invalid();
    }
    parcel.write(&(values.len() as u32)).map_err(ipc_err_handle)?;
    for value in values {
        write_bytes(value, total, parcel)?;
    }
    Ok(())
}

fn read_list(total: &mut usize, parcel: &mut MsgParcel) -> Result<Vec<Vec<u8>>> {
    charge(total, LENGTH_BYTES)?;
    let count = parcel.read::<u32>().map_err(ipc_err_handle)? as usize;
    if count > MAX_ITEMS {
        return invalid();
    }
    let mut values = Vec::new();
    for _ in 0..count {
        values.push(read_bytes(total, parcel)?);
    }
    Ok(values)
}

pub fn serialize_target(value: &TargetRequest, parcel: &mut MsgParcel) -> Result<()> {
    write_protocol(value.protocol, parcel)?;
    parcel.write(&value.hash_alg).map_err(ipc_err_handle)?;
    let mut total = TARGET_FIXED_BYTES;
    write_bytes(&value.element, &mut total, parcel)
}

pub fn deserialize_target(parcel: &mut MsgParcel) -> Result<TargetRequest> {
    let mut total = TARGET_FIXED_BYTES;
    Ok(TargetRequest {
        protocol: read_protocol(parcel)?,
        hash_alg: parcel.read::<u32>().map_err(ipc_err_handle)?,
        element: read_bytes(&mut total, parcel)?,
    })
}

pub fn serialize_search(value: &SearchRequest, parcel: &mut MsgParcel) -> Result<()> {
    let mut total = SEARCH_FIXED_BYTES;
    write_protocol(value.protocol, parcel)?;
    write_list(&value.ciphertexts, &mut total, parcel)?;
    parcel.write(&value.symmetric.is_some()).map_err(ipc_err_handle)?;
    if let Some(values) = &value.symmetric {
        write_list(values, &mut total, parcel)?;
    }
    Ok(())
}

pub fn deserialize_search(parcel: &mut MsgParcel) -> Result<SearchRequest> {
    let mut total = SEARCH_FIXED_BYTES;
    let protocol = read_protocol(parcel)?;
    let ciphertexts = read_list(&mut total, parcel)?;
    let symmetric = if parcel.read::<bool>().map_err(ipc_err_handle)? {
        Some(read_list(&mut total, parcel)?)
    } else {
        None
    };
    Ok(SearchRequest { protocol, ciphertexts, symmetric })
}

pub fn serialize_result(value: &SearchResponse, parcel: &mut MsgParcel) -> Result<()> {
    let mut total = RESULT_FIXED_BYTES;
    parcel.write(&value.matched).map_err(ipc_err_handle)?;
    parcel.write(&value.values.is_some()).map_err(ipc_err_handle)?;
    if let Some(values) = &value.values {
        write_list(values, &mut total, parcel)?;
    }
    Ok(())
}

pub fn deserialize_result(parcel: &mut MsgParcel) -> Result<SearchResponse> {
    let mut total = RESULT_FIXED_BYTES;
    let matched = parcel.read::<bool>().map_err(ipc_err_handle)?;
    let values = if parcel.read::<bool>().map_err(ipc_err_handle)? {
        Some(read_list(&mut total, parcel)?)
    } else {
        None
    };
    Ok(SearchResponse { matched, values })
}
