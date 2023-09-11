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

use std::{
    slice,
    ffi::{c_char, CString},
};
use hilog_rust::{hilog, HiLogLabel, LogType};

use asset_common::{
    asset_log_info,
    asset_log_error,
    definition::{
        AssetMap,
        DataType,
        ErrCode,
        Tag,
        Value,
        asset_type_transform::GetType
    }};
use asset_rust_sdk::{asset_insert, add_asset};

// asset_rust_sdk的crate名字叫asset_sdk或asset, libasset

/// blablabla as documentation
#[no_mangle]
pub extern "C" fn AssetInsert(code: i32) -> i32
{
    asset_log_info!("receive code {} in AssetInsert", @public(code));
    match asset_insert(code) {
        Ok(res) => {
            res as i32
        },
        Err(res) => {
            // asset_log_error!("err");
            res as i32
        }
    }
}

/// add asset c2rust
/// # Safety
/// dereference pointer
#[no_mangle]
pub unsafe extern "C" fn AddAssetC2Rust(attributes: *const AssetParam, attr_cnt: u32) -> i32 {
    asset_log_error!("[YZT] enter AddAssetC2Rust!");
    if attributes.is_null() || attr_cnt == 0 { // todo: 待确认是否需要校验
        return ErrCode::InvalidArgument as i32;
    }

    let mut map = AssetMap::with_capacity(attr_cnt as usize);
    for i in 0..attr_cnt {
        let attr = attributes.offset(i as isize);
        let attr_tag = match Tag::try_from((*attr).tag) {
            Ok(tag) => tag,
            Err(err_code) => return err_code as i32,
        };
        match attr_tag.get_type() {
            Ok(DataType::Bool) => {
                map.insert(attr_tag, Value::BOOL((*attr).value.boolean));
            },
            Ok(DataType::Uint32) => {
                map.insert(attr_tag, Value::NUMBER((*attr).value.uint32));
            },
            Ok(DataType::Bytes) => {
                let blob_slice = slice::from_raw_parts((*attr).value.blob.data, (*attr).value.blob.size as usize);
                let blob_vec = blob_slice.to_vec();
                map.insert(attr_tag, Value::Bytes(blob_vec));
            },
            _ => {
                return ErrCode::InvalidArgument as i32;
            },
        }
    }
    asset_log_error!("[YZT] end AddAssetC2Rust!");
    add_asset(map) as i32
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