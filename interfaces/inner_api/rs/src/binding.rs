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

//! This create implement the asset

use std::slice;

use asset_common::{
    loge,
    definition::{
        AssetMap,
        DataType,
        ErrCode,
        Tag,
        Value,
        asset_type_transform::GetType
    }};
use asset_rust_sdk::Manager;

/// add asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn AddAssetC2Rust(attributes: *const AssetParam, attr_cnt: u32) -> i32 {
    loge!("[YZT] enter AddAssetC2Rust!");
    if attributes.is_null() || attr_cnt == 0 {
        loge!("[YZT] null pointer");
        return ErrCode::InvalidArgument as i32;
    }

    let mut map = AssetMap::with_capacity(attr_cnt as usize);
    for i in 0..attr_cnt {
        let attr = attributes.offset(i as isize);
        let attr_tag = match Tag::try_from((*attr).tag) {
            Ok(tag) => tag,
            Err(err_code) => return err_code as i32,
        };
        match attr_tag.data_type() {
            DataType::Uint32 => {
                map.insert(attr_tag, Value::NUMBER((*attr).value.uint32));
            },
            DataType::Bytes => {
                let blob_slice = slice::from_raw_parts((*attr).value.blob.data, (*attr).value.blob.size as usize);
                let blob_vec = blob_slice.to_vec();
                map.insert(attr_tag, Value::Bytes(blob_vec));
            },
        }
    }
    loge!("[YZT] end AddAssetC2Rust!");
    match Manager::build() {
        Ok(manager) => {
            if let Err(e) = manager.add(map) {
                e as i32
            } else {
                0
            }
        },
        Err(e) => e as i32
    }
}

/// asset param from c
#[repr(C)]
pub struct AssetParam {
    tag: u32,
    value: AssetValue,
}

#[repr(C)]
struct AssetBlob {
    size: u32,
    data: *mut u8,
}

#[repr(C)]
union AssetValue {
    int32: i32,
    uint32: u32,
    int64: i64,
    uint64: u64,
    boolean: bool,
    blob: std::mem::ManuallyDrop<AssetBlob>,
}