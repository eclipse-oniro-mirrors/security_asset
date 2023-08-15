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

mod asset_deserialize;
mod asset_request;
mod asset_serialize;

use ipc_rust::MsgParcel;

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
}
