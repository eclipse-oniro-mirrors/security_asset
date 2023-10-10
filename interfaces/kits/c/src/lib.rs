/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

use std::{
    convert::TryFrom,
    mem::{ManuallyDrop, size_of},
    result::Result, slice
};

use libc::size_t;

use asset_common::{
    loge,
    definition::{AssetMap, DataType, ErrCode, IntoValue, Tag, Value}
};
use asset_sdk::Manager;

const RESULT_CODE_SUCCESS: i32 = 0;

fn into_map(attributes: *const Asset_Attr, attr_cnt: u32) -> Option<AssetMap> {
    if attributes.is_null() {
        loge!("[FATAL][RUST SDK]Attributes is null.");
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
                }
                DataType::Uint32 => {
                    map.insert(attr_tag, Value::Number((*attr).value.uint32));
                },
                DataType::Bytes => {
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
pub extern "C" fn add_asset(attributes: *const Asset_Attr, attr_cnt: u32) -> i32 {
    let map = match into_map(attributes, attr_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e as i32,
    };

    if let Err(e) = manager.add(&map) {
        e as i32
    } else {
        RESULT_CODE_SUCCESS
    }
}

/// Function called from C programming language to Rust programming language for removing Asset.
#[no_mangle]
pub extern "C" fn remove_asset(query: *const Asset_Attr, query_cnt: u32) -> i32 {
    let map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e as i32,
    };

    if let Err(e) = manager.remove(&map) {
        e as i32
    } else {
        RESULT_CODE_SUCCESS
    }
}

/// Function called from C programming language to Rust programming language for updating Asset.
#[no_mangle]
pub extern "C" fn update_asset(query: *const Asset_Attr, query_cnt: u32,
    attributes_to_update: *const Asset_Attr, update_cnt: u32) -> i32 {
    let query_map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let update_map = match into_map(attributes_to_update, update_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e as i32,
    };

    if let Err(e) = manager.update(&query_map, &update_map) {
        e as i32
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
pub unsafe extern "C" fn pre_query_asset(query: *const Asset_Attr, query_cnt: u32, challenge: *mut Asset_Blob) -> i32 {
    let _map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    if challenge.is_null() {
        loge!("[FATAL][RUST SDK]challenge is null");
        return ErrCode::InvalidArgument as i32;
    }

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e as i32,
    };

    let res = match manager.pre_query(&_map) {
        Err(e) => return e as i32,
        Ok(res) => res,
    };

    match Asset_Blob::try_from(&res) {
        Err(e) => e as i32,
        Ok(b) => {
            *challenge =b;
            RESULT_CODE_SUCCESS
        }
    }
}

/// Function called from C programming language to Rust programming language for querying Asset.
///
/// # Safety
///
/// The caller must ensure that the result_set pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn query_asset(query: *const Asset_Attr, query_cnt: u32,
    result_set: *mut Asset_ResultSet) -> i32 {
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
        Err(e) => return e as i32,
    };

    let res = match manager.query(&map) {
        Err(e) => return e as i32,
        Ok(res) => res,
    };

    match Asset_ResultSet::try_from(&res) {
        Err(e) => e as i32,
        Ok(s) => {
            *result_set = s;
            RESULT_CODE_SUCCESS
        }
    }
}

/// Function called from C programming language to Rust programming language for post quering Asset.
#[no_mangle]
pub extern "C" fn post_query_asset(handle: *const Asset_Attr, handle_cnt: u32) -> i32 {
    let _map = match into_map(handle, handle_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let _manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e as i32,
    };

    loge!("[YZT] enter post query");
    // if let Err(e) = manager.post_query(&map) {
    //     e as i32
    // }
    RESULT_CODE_SUCCESS
}

/// Attribute of Asset with a c representation.
#[repr(C)]
pub struct Asset_Attr {
    tag: u32,
    value: Asset_Value,
}

/// Blob of Asset with a c representation.
#[repr(C)]
pub struct Asset_Blob {
    size: u32,
    data: *mut u8,
}

impl TryFrom<&Vec<u8>> for Asset_Blob {
    type Error = ErrCode;

    fn try_from(vec: &Vec<u8>) -> Result<Self, Self::Error> {
        let mut blob = Asset_Blob {
            size: vec.len() as u32,
            data: std::ptr::null_mut(),
        };

        blob.data = unsafe { libc::malloc(blob.size as size_t) as *mut u8 };
        if blob.data.is_null() {
            loge!("[FATAL][RUST SDK]Unable to allocate memory for Asset_Blob.");
            return Err(ErrCode::OutOfMemory);
        }
        loge!("[RUST SDK][YZT] malloc for blob.data 0x{:x}", blob.data as usize);
        unsafe { std::ptr::copy_nonoverlapping(vec.as_ptr(), blob.data, blob.size as usize) };
        Ok(blob)
    }
}

#[repr(C)]
union Asset_Value {
    boolean: bool,
    uint32: u32,
    blob: ManuallyDrop::<Asset_Blob>,
}

impl TryFrom<&Value> for Asset_Value {
    type Error = ErrCode;

    fn try_from(value: &Value) -> Result<Self, Self::Error> {
        let mut out = Asset_Value { boolean: false };
        match value {
            Value::Bool(v) => out.boolean = *v,
            Value::Number(v) => out.uint32 = *v,
            Value::Bytes(v) => out.blob = ManuallyDrop::new(Asset_Blob::try_from(v)?),
        }
        Ok(out)
    }
}

#[repr(C)]
struct Asset_Result {
    count: u32,
    attrs: *mut Asset_Attr,
}

impl TryFrom<&AssetMap> for Asset_Result {
    type Error = ErrCode;

    fn try_from(map: &AssetMap) -> Result<Self, Self::Error> {
        let mut result = Asset_Result {
            count: map.len() as u32,
            attrs: std::ptr::null_mut(),
        };

        result.attrs = unsafe {
            libc::malloc(result.count.wrapping_mul(size_of::<Asset_Attr>() as u32) as size_t) as *mut Asset_Attr
        };
        if result.attrs.is_null() {
            loge!("[FATAL][RUST SDK]Unable to allocate memory for Asset_Result.");
            return Err(ErrCode::OutOfMemory);
        }

        loge!("[RUST SDK][YZT] malloc for result.attrs 0x{:x}", result.attrs as usize);
        for (i, (tag, value)) in map.iter().enumerate() {
            unsafe {
                let attr = result.attrs.add(i);
                (*attr).tag = *tag as u32;
                (*attr).value = Asset_Value::try_from(value)?;
            }
        }
        Ok(result)
    }
}

/// ResultSet of Asset with a c representation.
#[repr(C)]
pub struct Asset_ResultSet {
    count: u32,
    results: *mut Asset_Result,
}

impl TryFrom<&Vec<AssetMap>> for Asset_ResultSet {
    type Error = ErrCode;

    fn try_from(maps: &Vec<AssetMap>) -> Result<Self, Self::Error> {
        let mut result_set = Asset_ResultSet {
            count: maps.len() as u32,
            results: std::ptr::null_mut(),
        };
        result_set.results = unsafe {
            libc::malloc(result_set.count.wrapping_mul(size_of::<Asset_Result>() as u32) as size_t) as *mut Asset_Result
        };
        if result_set.results.is_null() {
            loge!("[FATAL][RUST SDK]Unable to allocate memory for Asset_ResultSet.");
            return Err(ErrCode::OutOfMemory);
        }
        loge!("[RUST SDK][YZT] malloc for resultSet.results 0x{:x}", result_set.results as usize);
        for (i, map) in maps.iter().enumerate() {
            unsafe {
                let result = result_set.results.add(i);
                *result = Asset_Result::try_from(map)?;
            }
        }
        Ok(result_set)
    }
}