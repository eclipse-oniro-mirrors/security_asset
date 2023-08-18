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

//! This create implement the ipc_client

#![allow(dead_code)]

mod asset_request;

use asset_common_lib::{asset_log_info,
    asset_type::{AssetStatusCode, SerializeAsset, AssetMap, DeserializeAsset, AssetResult}};
use ipc_rust::MsgParcel;

use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

#[allow(dead_code)]
pub struct AssetIpcSender {
    parcel_send: MsgParcel, // ipc parcel, n, private
    parcel_reply: Option<MsgParcel>, // reply, n, private
                            // send, v
                            // serialize, v
                            // deserialize, v
}

impl AssetIpcSender {
    pub fn new() -> Option<Self> {
        MsgParcel::new().map(|parcel| AssetIpcSender {
            parcel_send: parcel,
            parcel_reply: None,
        })
    }

    fn serialize(&mut self, value: &impl SerializeAsset) -> AssetResult<()> {
        value.serialize(&mut self.parcel_send)
    }

    fn deserialize(&self) -> AssetResult<AssetMap> {
        match &self.parcel_reply {
            Some(p) => {
                AssetMap::deserialize(p)
            },
            _ => {
                Err(AssetStatusCode::Failed)
            }
        }
    }

    pub fn read_reply() -> AssetResult<AssetMap> {
        Err(AssetStatusCode::Failed)
    }

    pub fn read_request(&self) -> AssetResult<AssetMap> {
        let map = AssetMap::deserialize(&self.parcel_send)?;
        asset_log_info!("map size is {}", @public(map.len()));
        Ok(AssetStatusCode::Ok)
    }
}
