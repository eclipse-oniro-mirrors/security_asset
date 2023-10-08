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

use std::slice;

use asset_common::{
    loge,
    definition::{AssetMap, DataType, ErrCode, IntoValue, Tag, Value}
};
use asset_sdk::Manager;

const RESULT_CODE_SUCCESS: i32 = 0;

unsafe fn into_map(attributes: *const Asset_Attr, attr_cnt: u32) -> Option<AssetMap> {
    if attributes.is_null() || attr_cnt == 0 {
        loge!("[FATAL] attributes is null or attr_cnt is 0");
        return None;
    }

    let mut map = AssetMap::with_capacity(attr_cnt as usize);
    for i in 0..attr_cnt {
        let attr = attributes.offset(i as isize);
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
        }
    }
    Some(map)
}

/// add asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn add_asset(attributes: *const Asset_Attr, attr_cnt: u32) -> i32 {
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

/// remove asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn remove_asset(query: *const Asset_Attr, query_cnt: u32) -> i32 {
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

/// update asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn update_asset(query: *const Asset_Attr, query_cnt: u32,
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

/// preQuery asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn pre_query_asset(query: *const Asset_Attr, query_cnt: u32, _challenge: *mut Asset_Blob) -> i32 {
    let _map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    loge!("[YYD] pre_query_asset!");
    let _manager = match Manager::build() {
        Ok(manager) => {
            match manager.pre_query(&_map) {
                Ok(r) => {
                    // format challenge info
                    let size = r.len() as u32;
                    let ptr = r.as_ptr() as *mut u8;
                    std::mem::forget(r);
                    (*_challenge).size = size;
                    (*_challenge).data = ptr;
                    0
                },
                Err(e) => {
                    e as i32
                }
            }
        },
        Err(e) => e as i32
    };

    loge!("[YZT] enter pre query");
    // if let Err(e) = manager.pre_query(&map) {
    //     e as i32
    // }
    RESULT_CODE_SUCCESS
}

/// query asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn query_asset(query: *const Asset_Attr, query_cnt: u32,
    _result_set: *mut Asset_ResultSet) -> i32 {
    let map = match into_map(query, query_cnt) {
        Some(map) => map,
        None => return ErrCode::InvalidArgument as i32,
    };

    let manager = match Manager::build() {
        Ok(manager) => manager,
        Err(e) => return e as i32,
    };

    loge!("[YZT] enter query");
    match manager.query(&map) {
        Err(e) => e as i32,
        Ok(_res) => {
            loge!("[YZT] end query");
            RESULT_CODE_SUCCESS
        }
    }
}

/// postQuery asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn post_query_asset(handle: *const Asset_Attr, handle_cnt: u32) -> i32 {
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

/// Attribute of Asset
#[repr(C)]
pub struct Asset_Attr {
    tag: u32,
    value: Asset_Value,
}

/// Blob of Asset
#[repr(C)]
pub struct Asset_Blob {
    size: u32,
    data: *mut u8,
}

#[repr(C)]
union Asset_Value {
    boolean: bool,
    uint32: u32,
    blob: std::mem::ManuallyDrop<Asset_Blob>,
}

#[repr(C)]
struct Asset_Result {
    attrs: *mut Asset_Attr,
    count: u32,
}

/// Result Set of Asset
#[repr(C)]
pub struct Asset_ResultSet {
    results: *mut Asset_Result,
    count: u32,
}
