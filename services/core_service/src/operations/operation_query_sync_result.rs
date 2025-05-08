/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

//! This module is used to query the result of synchronization.

use asset_common::CallingInfo;
use asset_definition::{AssetError, AssetMap, ErrCode, Extension, Result, SyncResult, Tag, Value};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_sdk::plugin_interface::{ EventType, ExtDbMap, PARAM_NAME_USER_ID, PARAM_NAME_OWNER_INFO,
    PARAM_NAME_OWNER_TYPE, PARAM_NAME_REQUIRE_ATTR_ENCRYPTED, PARAM_NAME_GROUP_ID, PARAM_NAME_RESULT_CODE,
    PARAM_NAME_TOTAL_COUNT, PARAM_NAME_FAILED_COUNT };

use crate::operations::common;

const OPTIONAL_ATTRS: [Tag; 2] = [Tag::GroupId, Tag::RequireAttrEncrypted];

fn check_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    common::check_tag_validity(attributes, &OPTIONAL_ATTRS)?;
    common::check_group_validity(attributes, calling_info)?;
    common::check_value_validity(attributes)?;
    Ok(())
}

fn map_err(code: u32) -> AssetError {
    match ErrCode::try_from(code) {
        Ok(code) => AssetError{ code, msg: "get sync result failed".to_string() },
        Err(err) => err
    }
}

pub(crate) fn query_sync_result(calling_info: &CallingInfo, query: &AssetMap) -> Result<SyncResult> {
    check_arguments(query, calling_info)?;

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
        params.insert(PARAM_NAME_OWNER_INFO, Value::Bytes(calling_info.owner_info().clone()));
        params.insert(PARAM_NAME_OWNER_TYPE, Value::Number(calling_info.owner_type() as u32));
        params.insert(PARAM_NAME_REQUIRE_ATTR_ENCRYPTED, Value::Bool(query.get_bool_attr(&Tag::RequireAttrEncrypted)?));
        if let Some(group) = calling_info.group() {
            params.insert(PARAM_NAME_GROUP_ID, Value::Bytes(group.clone()));
        }
        match load.process_event(EventType::QuerySyncResult, &mut params) {
            Ok(()) => return Ok(SyncResult {
                result_code: params.get_num_attr(&PARAM_NAME_RESULT_CODE)? as i32,
                total_count: params.get_num_attr(&PARAM_NAME_TOTAL_COUNT)?,
                failed_count: params.get_num_attr(&PARAM_NAME_FAILED_COUNT)?,
            }),
            Err(code) => return Err(map_err(code))
        }
    }
    Ok(SyncResult::default())
}
