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

//! This module is used to provide common capabilities for the Asset operations.

mod argument_check;

pub(crate) use argument_check::{check_required_tags, check_tag_validity, check_value_validity};

use asset_crypto_manager::crypto::SecretKey;
use asset_db_operator::types::{column, DbMap};
use asset_definition::{Accessibility, AssetMap, AuthType, Extension, Result, Tag, Value};

use crate::calling_info::CallingInfo;

const TAG_COLUMN_TABLE: [(Tag, &str); 15] = [
    (Tag::Secret, column::SECRET),
    (Tag::Alias, column::ALIAS),
    (Tag::Accessibility, column::ACCESSIBILITY),
    (Tag::AuthType, column::AUTH_TYPE),
    (Tag::SyncType, column::SYNC_TYPE),
    (Tag::DeleteType, column::DELETE_TYPE),
    (Tag::RequirePasswordSet, column::REQUIRE_PASSWORD_SET),
    (Tag::DataLabelCritical1, column::CRITICAL1),
    (Tag::DataLabelCritical2, column::CRITICAL2),
    (Tag::DataLabelCritical3, column::CRITICAL3),
    (Tag::DataLabelCritical4, column::CRITICAL4),
    (Tag::DataLabelNormal1, column::NORMAL1),
    (Tag::DataLabelNormal2, column::NORMAL2),
    (Tag::DataLabelNormal3, column::NORMAL3),
    (Tag::DataLabelNormal4, column::NORMAL4),
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
    column::DELETE_TYPE,
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

pub(crate) const ACCESS_CONTROL_ATTRS: [Tag; 6] =
    [Tag::Alias, Tag::Accessibility, Tag::AuthType, Tag::DeleteType, Tag::SyncType, Tag::RequirePasswordSet];

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
    Ok(SecretKey::new(calling.user_id(), calling.owner_info(), auth_type, access_type, require_password_set))
}

pub(crate) fn build_aad(attrs: &DbMap) -> Vec<u8> {
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
