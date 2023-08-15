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

use std::ffi::{c_char, CString};
use hilog_rust::{info, hilog, HiLogLabel, LogType};
mod asset_ipc_client;
use asset_common_lib::{asset_log_info, asset_type::{AssetResult, AssetStatusCode, AssetIpcCode}};
use crate::asset_ipc_client::AssetIpcSender;

/// insert data into asset
pub fn asset_insert(_code: i32) -> AssetResult<AssetStatusCode>
{
    asset_log_info!("AssetSdkLib", "xxxx");
    if let Some(sender) = AssetIpcSender::new() {
        sender.send_request(AssetIpcCode::Insert, "test")
    } else {
        Err(AssetStatusCode::Failed)
    }
}
