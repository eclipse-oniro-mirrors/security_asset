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

use std::collections::HashSet;

use asset_common::CallingInfo;
use asset_definition::{
    AssetMap, Tag, Value, Result, macros_lib, ErrCode, WrapType, LocalStatus,
    SyncType, SyncStatus,
};

use crate::{
    types::{DbMap, column},
    common::{
        TAG_COLUMN_TABLE, REQUIRED_ATTRS, CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS, NORMAL_LOCAL_LABEL_ATTRS,
        ACCESS_CONTROL_ATTRS, ASSET_SYNC_ATTRS, OPTIONAL_ATTRS, check_accessibility_validity, check_value_validity,
        check_sync_permission, check_wrap_permission, check_persistent_permission,
        check_required_tags, check_group_validity,
    }
};

const INVALID_TAGS: [Tag; 5] = [Tag::AuthType, Tag::Accessibility, Tag::RequireAttrEncrypted, Tag::GroupId, Tag::UserId];

fn check_tag_validity_with_invalid_tags(attrs: &AssetMap, valid_tags: &[Tag], invalid_tags: &[Tag]) -> Result<()> {
    for tag in attrs.keys() {
        if !valid_tags.contains(tag) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The tag [{}] is illegal.", tag);
        }
        if invalid_tags.contains(tag) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The tag [{}] is illegal.", tag);
        }
    }
    Ok(())
}

fn check_array_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_required_tags(attributes, &REQUIRED_ATTRS)?;
    let mut valid_tags = CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&NORMAL_LOCAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&ASSET_SYNC_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    check_tag_validity_with_invalid_tags(attributes, &valid_tags, &INVALID_TAGS)?;
    check_group_validity(attributes, calling_info)?;
    check_value_validity(attributes)?;
    check_accessibility_validity(attributes, calling_info)?;
    check_sync_permission(attributes, calling_info)?;
    check_wrap_permission(attributes, calling_info)?;
    check_persistent_permission(attributes)
}

fn into_db_map_with_column_names(attrs: &AssetMap, column_names: &mut HashSet<String>) -> DbMap {
    let mut db_data = DbMap::new();
    for (attr_tag, attr_value) in attrs.iter() {
        for (table_tag, table_column) in TAG_COLUMN_TABLE {
            if *attr_tag == table_tag {
                column_names.insert(table_column.to_string());
                db_data.insert(table_column, attr_value.clone());
                break;
            }
        }
    }
    db_data
}

fn add_default_batch_attrs(db_data: &mut DbMap) {
    db_data.entry(column::SYNC_TYPE).or_insert(Value::Number(SyncType::default() as u32));
    db_data.entry(column::REQUIRE_PASSWORD_SET).or_insert(Value::Bool(bool::default()));
    db_data.entry(column::IS_PERSISTENT).or_insert(Value::Bool(bool::default()));
    db_data.entry(column::LOCAL_STATUS).or_insert(Value::Number(LocalStatus::Local as u32));
    db_data.entry(column::SYNC_STATUS).or_insert(Value::Number(SyncStatus::SyncAdd as u32));
    db_data.entry(column::WRAP_TYPE).or_insert(Value::Number(WrapType::default() as u32));
}

pub(crate) fn add_not_null_column(column_names: &mut HashSet<String>) {
    column_names.insert(column::ID.to_string());
    column_names.insert(column::SECRET.to_string());
    column_names.insert(column::ALIAS.to_string());
    column_names.insert(column::OWNER.to_string());
    column_names.insert(column::OWNER_TYPE.to_string());
    column_names.insert(column::SYNC_TYPE.to_string());
    column_names.insert(column::ACCESSIBILITY.to_string());
    column_names.insert(column::AUTH_TYPE.to_string());
    column_names.insert(column::CREATE_TIME.to_string());
    column_names.insert(column::UPDATE_TIME.to_string());
    column_names.insert(column::IS_PERSISTENT.to_string());
    column_names.insert(column::VERSION.to_string());
    column_names.insert(column::REQUIRE_PASSWORD_SET.to_string());
    column_names.insert(column::LOCAL_STATUS.to_string());
    column_names.insert(column::SYNC_STATUS.to_string());
    column_names.insert(column::WRAP_TYPE.to_string());
}

pub(crate) fn parse_attr_in_array(
    attributes: &AssetMap,
    calling_info: &CallingInfo,
    column_names: &mut HashSet<String>
) -> Result<DbMap> {
    check_array_arguments(attributes, calling_info)?;
    let mut db_data = into_db_map_with_column_names(attributes, column_names);
    if let Some(group) = calling_info.group() {
        db_data.insert(column::GROUP_ID, Value::Bytes(group));
    };
    add_default_batch_attrs(&mut db_data);
    Ok(db_data)
}
