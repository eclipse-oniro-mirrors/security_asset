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

mod asset_request;

pub use asset_common_lib::asset_type;
use asset_common_lib::{
    asset_log_error, asset_log_info,
    asset_type::{AssetMap, AssetResult, AssetStatusCode, Tag, Value},
};
use crate::asset_request::AssetIpcProxy;
use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

/// insert data into asset
pub fn asset_insert(_code: i32) -> AssetResult<AssetStatusCode> {
    asset_log_info!("enter asser insert");
    if let Ok(sender) = AssetIpcProxy::new() {
        let mut map = AssetMap::new();
        map.insert(Tag::AuthType, Value::NUMBER(5));
        sender.insert(&map)?; // ingore reply
        match sender.insert(&map) {
            Ok(res) => {
                if let Some(v) = res.get(&Tag::AuthType) {
                    asset_log_info!("res is {}", @public(v));
                } else {
                    asset_log_error!("asset_insert failed!");
                }
                Ok(AssetStatusCode::Success)
            },
            Err(e) => Err(e),
        }
    } else {
        Err(AssetStatusCode::Failed)
    }
}

/// add an asset
pub fn add(input: AssetMap) -> AssetResult<AssetMap> {
    asset_log_info!("enter assert add");
    AssetIpcProxy::new()?.add(&input)
}

/// the mock function
pub fn add_asset(_input: AssetMap) -> AssetStatusCode {
    asset_log_info!("enter assert add");
    AssetStatusCode::Success
}