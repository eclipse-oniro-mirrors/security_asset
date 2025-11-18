/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

//! This module is used to provide common capabilities for the Asset operations.

use asset_common::{CallingInfo, OWNER_INFO_SEPARATOR};
use asset_definition::{AssetMap, OperationType, Tag, Value};
use asset_log::{loge, logi};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_sdk::plugin_interface::{EventType, ExtDbMap, PARAM_NAME_BUNDLE_NAME, PARAM_NAME_USER_ID};

pub(crate) fn inform_asset_ext(calling_info: &CallingInfo, input: &AssetMap) {
    if let Some(Value::Number(operation_type)) = input.get(&Tag::OperationType) {
        match operation_type {
            x if *x == OperationType::NeedSync as u32 => {
                if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
                    let owner_info_str = String::from_utf8_lossy(calling_info.owner_info()).to_string();
                    let owner_info_vec: Vec<_> = owner_info_str.split(OWNER_INFO_SEPARATOR).collect();
                    let caller_name = owner_info_vec[0];
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
                    params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(caller_name.as_bytes().to_vec()));
                    match load.process_event(EventType::Sync, &mut params) {
                        Ok(()) => logi!("process sync ext event success."),
                        Err(code) => loge!("process sync ext event failed, code: {}", code),
                    }
                }
            },
            x if *x == OperationType::NeedLogout as u32 => {
                if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
                    let owner_info_str = String::from_utf8_lossy(calling_info.owner_info()).to_string();
                    let owner_info_vec: Vec<_> = owner_info_str.split(OWNER_INFO_SEPARATOR).collect();
                    let caller_name = owner_info_vec[0];
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
                    params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(caller_name.as_bytes().to_vec()));
                    match load.process_event(EventType::CleanCloudFlag, &mut params) {
                        Ok(()) => logi!("process clean cloud flag ext event success."),
                        Err(code) => loge!("process clean cloud flag ext event failed, code: {}", code),
                    }
                }
            },
            x if *x == OperationType::NeedDeleteCloudData as u32 => {
                if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
                    params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(calling_info.owner_info().clone()));
                    match load.process_event(EventType::DeleteCloudData, &mut params) {
                        Ok(()) => logi!("process delete cloud data ext event success."),
                        Err(code) => loge!("process delete cloud data ext event failed, code: {}", code),
                    }
                }
            },
            _ => {},
        }
    }
}
