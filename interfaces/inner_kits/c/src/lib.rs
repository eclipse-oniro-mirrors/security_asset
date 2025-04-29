/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

//! This module implements the function of Asset SDK from C to RUST.

use core::ffi::c_void;
use std::{
    convert::TryFrom,
    mem::size_of,
    ptr::{copy_nonoverlapping, null_mut},
    result::Result,
    slice,
};

use asset_log::loge;
use asset_sdk::{log_throw_error, AssetError, AssetMap, Conversion, DataType, ErrCode, Manager, SyncResult, Tag, Value};

const MAX_MAP_CAPACITY: u32 = 64;
const RESULT_CODE_SUCCESS: i32 = 0;
extern "C" {
    fn AssetMalloc(size: u32) -> *mut c_void;
}

fn into_map(attributes: *const AssetAttr, attr_cnt: u32) -> Option<AssetMap> {
    if attributes.is_null() && attr_cnt != 0 {
        loge!("[FATAL][RUST SDK]Attributes is null.");
        return None;
    }
    if attr_cnt > MAX_MAP_CAPACITY {
        loge!("[FATAL][RUST SDK]Number of attributes exceeds limit.");
        return None;
    }

    let mut map = AssetMap::with_capacity(attr_cnt as usize);
    for i in 0..attr_cnt {
        unsafe {
            let attr = attributes.add(i as usize);
            let attr_tag = match Tag::try_from((*attr).tag) {
                Ok(tag) => tag,
                Err(_) => return None,
            };
            match attr_tag.data_type() {
                DataType::Bool => {
                    map.insert(attr_tag, Value::Bool((*attr).value.boolean));
                },
                DataType::Number => {
                    map.insert(attr_tag, Value::Number((*attr).value.uint32));
                },
                DataType::Bytes => {
                    if (*attr).value.blob.data.is_null() || (*attr).value.blob.size == 0 {
                        loge!("[FATAL][RUST SDK]Blob data is empty.");
                        return None;
                    }
                    let blob_slice = slice::from_raw_parts((*attr).value.blob.data, (*attr).value.blob.size as usize);
                    let blob_vec = blob_slice.to_vec();
                    map.insert(attr_tag, Value::Bytes(blob_vec));
                },
            };
        }
    }
    Some(map)
}

/// Function called from C programming language to Rust programming language for adding Asset.
#[no_mangle]
pub extern "C" fn add_asset(attributes: *const AssetAttr, attr_cnt: u32) -> i32 {
    let map = match into_map(attributes, attr_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    if let Err(e) = manager.add(&map) {
        e.code as i32
    } else {
        RESULT_CODE_SUCCESS
    }
}

/// Function called from C programming language to Rust programming language for removing Asset.
#[no_mangle]
pub extern "C" fn remove_asset(query: *const AssetAttr, query_cnt: u32) -> i32 {
    let map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    if let Err(e) = manager.remove(&map) {
        e.code as i32
    } else {
        RESULT_CODE_SUCCESS
    }
}

/// Function called from C programming language to Rust programming language for updating Asset.
#[no_mangle]
pub extern "C" fn update_asset(
    query: *const AssetAttr,
    query_cnt: u32,
    attrs_to_update: *const AssetAttr,
    update_cnt: u32,
) -> i32 {
    let query_map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let update_map = match into_map(attrs_to_update, update_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    if let Err(e) = manager.update(&query_map, &update_map) {
        e.code as i32
    } else {
        RESULT_CODE_SUCCESS
    }
}

/// Function called from C programming language to Rust programming language for pre querying Asset.
///
/// # Safety
///
/// The caller must ensure that the challenge pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn pre_query_asset(query: *const AssetAttr, query_cnt: u32, challenge: *mut AssetBlob) -> i32 {
    let map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    if challenge.is_null() {
        loge!("[FATAL][RUST SDK]challenge is null");
        return ErrCode::InvalidArgument as i32;
    }

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    let res = match manager.pre_query(&map) {
        Err(e) => return e.code as i32,
        Ok(res) => res,
    };

    match AssetBlob::try_from(&res) {
        Err(e) => e.code as i32,
        Ok(b) => {
            *challenge = b;
            RESULT_CODE_SUCCESS
        },
    }
}

/// Function called from C programming language to Rust programming language for querying Asset.
///
/// # Safety
///
/// The caller must ensure that the result_set pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn query_asset(query: *const AssetAttr, query_cnt: u32, result_set: *mut AssetResultSet) -> i32 {
    let map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    if result_set.is_null() {
        loge!("[FATAL][RUST SDK]result set is null");
        return ErrCode::InvalidArgument as i32;
    }

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    let res = match manager.query(&map) {
        Err(e) => return e.code as i32,
        Ok(res) => res,
    };

    match AssetResultSet::try_from(&res) {
        Err(e) => e.code as i32,
        Ok(s) => {
            *result_set = s;
            RESULT_CODE_SUCCESS
        },
    }
}

/// Function called from C programming language to Rust programming language for post quering Asset.
#[no_mangle]
pub extern "C" fn post_query_asset(handle: *const AssetAttr, handle_cnt: u32) -> i32 {
    let map = match into_map(handle, handle_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    if let Err(e) = manager.post_query(&map) {
        e.code as i32
    } else {
        RESULT_CODE_SUCCESS
    }
}

/// Function called from C programming language to Rust programming language for querying sync result.
#[no_mangle]
pub extern "C" fn query_sync_result(query: *const AssetAttr, query_cnt: u32, sync_result: *mut SyncResult) -> i32 {
    let map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::ParamVerificationFailed as i32,
    };

    if sync_result.is_null() {
        loge!("[FATAL][RUST SDK]result set is null");
        return ErrCode::ParamVerificationFailed as i32;
    }

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e.code as i32,
    };

    match manager.query_sync_result(&map) {
        Err(e) => e.code as i32,
        Ok(res) => {
            *sync_result = res; // TODO: remove unsafe
            RESULT_CODE_SUCCESS
        }
    }
}

/// Attribute of Asset with a c representation.
#[repr(C)]
pub struct AssetAttr {
    tag: u32,
    value: AssetValue,
}

/// Blob of Asset with a c representation.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct AssetBlob {
    size: u32,
    data: *mut u8,
}

impl TryFrom<&Vec<u8>> for AssetBlob {
    type Error = AssetError;

    fn try_from(vec: &Vec<u8>) -> Result<Self, Self::Error> {
        let mut blob = AssetBlob { size: vec.len() as u32, data: null_mut() };

        blob.data = unsafe { AssetMalloc(blob.size) as *mut u8 };
        if blob.data.is_null() {
            return log_throw_error!(
                ErrCode::OutOfMemory,
                "[FATAL][RUST SDK]Unable to allocate memory for Asset_Blob."
            );
        }
        unsafe { copy_nonoverlapping(vec.as_ptr(), blob.data, blob.size as usize) };
        Ok(blob)
    }
}

#[repr(C)]
union AssetValue {
    boolean: bool,
    uint32: u32,
    blob: AssetBlob,
}

impl TryFrom<&Value> for AssetValue {
    type Error = AssetError;

    fn try_from(value: &Value) -> Result<Self, Self::Error> {
        let mut out = AssetValue { boolean: false };
        match value {
            Value::Bool(v) => out.boolean = *v,
            Value::Number(v) => out.uint32 = *v,
            Value::Bytes(v) => out.blob = AssetBlob::try_from(v)?,
        }
        Ok(out)
    }
}

#[repr(C)]
struct AssetResult {
    count: u32,
    attrs: *mut AssetAttr,
}

impl TryFrom<&AssetMap> for AssetResult {
    type Error = AssetError;

    fn try_from(map: &AssetMap) -> Result<Self, Self::Error> {
        let mut result = AssetResult { count: map.len() as u32, attrs: null_mut() };

        result.attrs =
            unsafe { AssetMalloc(result.count.wrapping_mul(size_of::<AssetAttr>() as u32)) as *mut AssetAttr };
        if result.attrs.is_null() {
            return log_throw_error!(
                ErrCode::OutOfMemory,
                "[FATAL][RUST SDK]Unable to allocate memory for Asset_Result."
            );
        }

        for (i, (tag, value)) in map.iter().enumerate() {
            unsafe {
                let attr = result.attrs.add(i);
                (*attr).tag = *tag as u32;
                (*attr).value = AssetValue::try_from(value)?;
            }
        }
        Ok(result)
    }
}

/// ResultSet of Asset with a c representation.
#[repr(C)]
pub struct AssetResultSet {
    count: u32,
    results: *mut AssetResult,
}

impl TryFrom<&Vec<AssetMap>> for AssetResultSet {
    type Error = AssetError;

    fn try_from(maps: &Vec<AssetMap>) -> Result<Self, Self::Error> {
        let mut result_set = AssetResultSet { count: maps.len() as u32, results: null_mut() };
        result_set.results =
            unsafe { AssetMalloc(result_set.count.wrapping_mul(size_of::<AssetResult>() as u32)) as *mut AssetResult };
        if result_set.results.is_null() {
            return log_throw_error!(
                ErrCode::OutOfMemory,
                "[FATAL][RUST SDK]Unable to allocate memory for Asset_ResultSet."
            );
        }
        for (i, map) in maps.iter().enumerate() {
            unsafe {
                let result = result_set.results.add(i);
                *result = AssetResult::try_from(map)?;
            }
        }
        Ok(result_set)
    }
}
