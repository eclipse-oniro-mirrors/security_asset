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

pub use asset_common::definition;
use asset_common::{
    loge, logi,
    definition::{AssetMap, Result, ErrCode, Tag, Value},
};
use crate::asset_request::AssetProxy;
use hilog_rust::hilog;
use std::ffi::{c_char, CString};

/// insert data into asset
pub fn asset_insert(_code: i32) -> Result<ErrCode> {
    logi!("enter asser insert");
    if let Ok(sender) = AssetProxy::build() {
        let mut map = AssetMap::new();
        map.insert(Tag::AuthType, Value::NUMBER(5));
        sender.insert(&map)?; // ingore reply
        match sender.insert(&map) {
            Ok(res) => {
                if let Some(v) = res.get(&Tag::AuthType) {
                    logi!("res is {}", @public(v));
                } else {
                    loge!("asset_insert failed!");
                }
                Ok(ErrCode::Success)
            },
            Err(e) => Err(e),
        }
    } else {
        Err(ErrCode::Failed)
    }
}

// /// add an asset
// pub fn add(input: AssetMap) -> Result<AssetMap> {
//     logi!("enter assert add");
//     AssetProxy::new()?.add(&input)
// }

/// add an asset
pub fn add(input: AssetMap) -> Result<()> {
    logi!("[YZT][RUST SDK]enter asset add");
    AssetProxy::build()?.add(&input)
}