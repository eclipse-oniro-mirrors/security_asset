/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

mod argument_check;
mod permission_check;

pub(crate) use argument_check::{check_required_tags, check_tag_validity, check_value_validity, MAX_LABEL_SIZE};
pub(crate) use permission_check::check_system_permission;

use asset_constants::CallingInfo;
use asset_crypto_manager::secret_key::SecretKey;
use asset_db_operator::types::{column, DbMap, DB_DATA_VERSION, DB_DATA_VERSION_V1};
use asset_definition::{
    log_throw_error, Accessibility, AssetMap, AuthType, ErrCode, Extension, Result, Tag, Value, OperationType
};
use asset_log::{logi, loge};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_sdk::plugin_interface::{EventType, ExtDbMap, PARAM_NAME_USER_ID};

const TAG_COLUMN_TABLE: [(Tag, &str); 20] = [
    (Tag::Secret, column::SECRET),
    (Tag::Alias, column::ALIAS),
    (Tag::Accessibility, column::ACCESSIBILITY),
    (Tag::AuthType, column::AUTH_TYPE),
    (Tag::SyncType, column::SYNC_TYPE),
    (Tag::UpdateTime, column::UPDATE_TIME),
    (Tag::IsPersistent, column::IS_PERSISTENT),
    (Tag::RequirePasswordSet, column::REQUIRE_PASSWORD_SET),
    (Tag::DataLabelCritical1, column::CRITICAL1),
    (Tag::DataLabelCritical2, column::CRITICAL2),
    (Tag::DataLabelCritical3, column::CRITICAL3),
    (Tag::DataLabelCritical4, column::CRITICAL4),
    (Tag::DataLabelNormal1, column::NORMAL1),
    (Tag::DataLabelNormal2, column::NORMAL2),
    (Tag::DataLabelNormal3, column::NORMAL3),
    (Tag::DataLabelNormal4, column::NORMAL4),
    (Tag::DataLabelNormalLocal1, column::NORMAL_LOCAL1),
    (Tag::DataLabelNormalLocal2, column::NORMAL_LOCAL2),
    (Tag::DataLabelNormalLocal3, column::NORMAL_LOCAL3),
    (Tag::DataLabelNormalLocal4, column::NORMAL_LOCAL4),
];

const AAD_ATTR: [&str; 14] = [
    column::ALIAS,
    column::OWNER,
    column::OWNER_TYPE,
    column::GROUP_ID,
    column::SYNC_TYPE,
    column::ACCESSIBILITY,
    column::REQUIRE_PASSWORD_SET,
    column::AUTH_TYPE,
    column::IS_PERSISTENT,
    column::VERSION,
    column::CRITICAL1,
    column::CRITICAL2,
    column::CRITICAL3,
    column::CRITICAL4,
];

pub(crate) const CRITICAL_LABEL_ATTRS: [Tag; 4] =
    [Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3, Tag::DataLabelCritical4];

pub(crate) const NORMAL_LABEL_ATTRS: [Tag; 4] =
    [Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4];

pub(crate) const NORMAL_LOCAL_LABEL_ATTRS: [Tag; 4] =
    [Tag::DataLabelNormalLocal1, Tag::DataLabelNormalLocal2, Tag::DataLabelNormalLocal3, Tag::DataLabelNormalLocal4];

pub(crate) const ACCESS_CONTROL_ATTRS: [Tag; 6] =
    [Tag::Alias, Tag::Accessibility, Tag::AuthType, Tag::IsPersistent, Tag::SyncType, Tag::RequirePasswordSet];

pub(crate) const ASSET_SYNC_ATTRS: [Tag; 1] = [Tag::OperationType];

pub(crate) fn get_cloumn_name(tag: Tag) -> Option<&'static str> {
    for (table_tag, table_column) in TAG_COLUMN_TABLE {
        if table_tag == tag {
            return Some(table_column);
        }
    }
    None
}

pub(crate) fn into_db_map(attrs: &AssetMap) -> DbMap {
    let mut db_data = DbMap::new();
    for (attr_tag, attr_value) in attrs.iter() {
        for (table_tag, table_column) in TAG_COLUMN_TABLE {
            if *attr_tag == table_tag {
                db_data.insert(table_column, attr_value.clone());
                break;
            }
        }
    }
    db_data
}

pub(crate) fn into_asset_map(db_data: &DbMap) -> AssetMap {
    let mut map = AssetMap::new();
    for (column, data) in db_data.iter() {
        for (table_tag, table_column) in TAG_COLUMN_TABLE {
            if (*column).eq(table_column) {
                map.insert(table_tag, data.clone());
                break;
            }
        }
    }
    map
}

pub(crate) fn add_owner_info(calling_info: &CallingInfo, db_data: &mut DbMap) {
    db_data.insert(column::OWNER, Value::Bytes(calling_info.owner_info().clone()));
    db_data.insert(column::OWNER_TYPE, Value::Number(calling_info.owner_type()));
}

pub(crate) fn build_secret_key(calling: &CallingInfo, attrs: &DbMap) -> Result<SecretKey> {
    let auth_type = attrs.get_enum_attr::<AuthType>(&column::AUTH_TYPE)?;
    let access_type = attrs.get_enum_attr::<Accessibility>(&column::ACCESSIBILITY)?;
    let require_password_set = attrs.get_bool_attr(&column::REQUIRE_PASSWORD_SET)?;
    Ok(SecretKey::new(calling, auth_type, access_type, require_password_set))
}

fn build_aad_v1(attrs: &DbMap) -> Vec<u8> {
    let mut aad = Vec::new();
    for column in &AAD_ATTR {
        match attrs.get(column) {
            Some(Value::Bytes(bytes)) => aad.extend(bytes),
            Some(Value::Number(num)) => aad.extend(num.to_le_bytes()),
            Some(Value::Bool(num)) => aad.push(*num as u8),
            None => continue,
        }
    }
    aad
}

fn to_hex(bytes: &Vec<u8>) -> Result<Vec<u8>> {
    let bytes_len = bytes.len();
    if bytes_len > MAX_LABEL_SIZE {
        return log_throw_error!(ErrCode::DataCorrupted, "The data in DB has been tampered with.");
    }

    let scale_capacity = 2;
    let mut hex_vec = Vec::with_capacity(bytes_len * scale_capacity);
    for byte in bytes.iter() {
        hex_vec.extend(format!("{:02x}", byte).as_bytes());
    }
    Ok(hex_vec)
}

fn build_aad_v2(attrs: &DbMap) -> Result<Vec<u8>> {
    let mut aad = Vec::new();
    for column in &AAD_ATTR {
        aad.extend(format!("{}:", column).as_bytes());
        match attrs.get(column) {
            Some(Value::Bytes(bytes)) => aad.extend(to_hex(bytes)?),
            Some(Value::Number(num)) => aad.extend(num.to_le_bytes()),
            Some(Value::Bool(num)) => aad.push(*num as u8),
            None => (),
        }
        aad.push(b'_');
    }
    Ok(aad)
}

pub(crate) fn build_aad(attrs: &DbMap) -> Result<Vec<u8>> {
    let version = attrs.get_num_attr(&column::VERSION)?;
    if version == DB_DATA_VERSION_V1 {
        Ok(build_aad_v1(attrs))
    } else {
        build_aad_v2(attrs)
    }
}

pub(crate) fn need_upgrade(db_date: &DbMap) -> Result<bool> {
    let version = db_date.get_num_attr(&column::VERSION)?;
    Ok(version != DB_DATA_VERSION)
}

pub(crate) fn inform_asset_ext(input: &AssetMap, user_id: i32) {
    if let Some(Value::Number(operation_type)) = input.get(&Tag::OperationType) {
        match operation_type {
            x if *x == OperationType::NeedSync as u32 => {
                let arc_asset_plugin = AssetPlugin::get_instance();
                let mut asset_plugin = arc_asset_plugin.lock().unwrap();
                if let Ok(load) = asset_plugin.load_plugin() {
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
                    match load.process_event(EventType::Sync, &params) {
                        Ok(()) => logi!("process sync ext event success."),
                        Err(code) => loge!("process sync ext event failed, code: {}", code),
                    }
                }
            },
            x if *x == OperationType::NeedLogout as u32 => {
                let arc_asset_plugin = AssetPlugin::get_instance();
                let mut asset_plugin = arc_asset_plugin.lock().unwrap();
                if let Ok(load) = asset_plugin.load_plugin() {
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
                    match load.process_event(EventType::Logout, &params) {
                        Ok(()) => logi!("process logout ext event success."),
                        Err(code) => loge!("process logout ext event failed, code: {}", code),
                    }
                }
            },
            _ => {},
        }
    }
}
