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

//! This crate implements the asset

mod argument_check;
mod crypto_adapter;
mod file_operator;

pub(crate) use argument_check::{check_required_tags, check_optional_tags, check_value_validity};
pub(crate) use crypto_adapter::{encrypt, decrypt, init_decrypt};
pub(crate) use file_operator::create_user_db_dir;

use std::time::{SystemTime, UNIX_EPOCH};

use asset_common::{definition::{Result, ErrCode, Value, Tag, AssetMap}, loge};
use asset_db_operator::{types::DbMap, database_table_helper::{
    COLUMN_OWNER, COLUMN_OWNER_TYPE, COLUMN_ACCESSIBILITY, COLUMN_SECRET, COLUMN_ALIAS, COLUMN_AUTH_TYPE,
    COLUMN_SYNC_TYPE, COLUMN_CRITICAL1, COLUMN_CRITICAL2, COLUMN_CRITICAL3,
    COLUMN_CRITICAL4, COLUMN_NORMAL1, COLUMN_NORMAL2, COLUMN_NORMAL3, COLUMN_NORMAL4, COLUMN_REQUIRE_PASSWORD_SET
}};

use crate::calling_info::CallingInfo;

pub(crate) const TAG_COLUMN_TABLE: [(Tag, &str); 14] = [
    (Tag::Secret, COLUMN_SECRET),
    (Tag::Alias, COLUMN_ALIAS),
    (Tag::Accessibility, COLUMN_ACCESSIBILITY),
    (Tag::AuthType, COLUMN_AUTH_TYPE),
    (Tag::SyncType, COLUMN_SYNC_TYPE),
    (Tag::RequirePasswordSet, COLUMN_REQUIRE_PASSWORD_SET),
    (Tag::DataLabelCritical1, COLUMN_CRITICAL1),
    (Tag::DataLabelCritical2, COLUMN_CRITICAL2),
    (Tag::DataLabelCritical3, COLUMN_CRITICAL3),
    (Tag::DataLabelCritical4, COLUMN_CRITICAL4),
    (Tag::DataLabelNormal1, COLUMN_NORMAL1),
    (Tag::DataLabelNormal2, COLUMN_NORMAL2),
    (Tag::DataLabelNormal3, COLUMN_NORMAL3),
    (Tag::DataLabelNormal4, COLUMN_NORMAL4),
];

pub(crate) const CRITICAL_LABEL_ATTRS: [Tag; 4] = [
    Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3, Tag::DataLabelCritical4,
];

pub(crate) const NORMAL_LABEL_ATTRS: [Tag; 4] = [
    Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4
];

pub(crate) const ACCESS_CONTROL_ATTRS: [Tag; 5] = [
    Tag::Alias, Tag::Accessibility, Tag::AuthType, Tag::SyncType, Tag::RequirePasswordSet,
];

pub(crate) fn get_cloumn_name(tag: Tag) -> Option<&'static str> {
    for (table_tag, table_column) in TAG_COLUMN_TABLE {
        if table_tag == tag {
            return Some(table_column)
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

pub(crate) fn get_system_time() -> Result<Vec<u8>> {
    let sys_time = SystemTime::now().duration_since(UNIX_EPOCH).map_err(|e| {
        loge!("[FATAL]Get system time faield [{}].", e);
        ErrCode::SystemTimeError
    })?;
    Ok(sys_time.as_millis().to_string().as_bytes().to_vec())
}

pub(crate) fn add_owner_info(calling_info: &CallingInfo, db_data: &mut DbMap) {
    db_data.insert(COLUMN_OWNER, Value::Bytes(calling_info.owner_info().clone()));
    db_data.insert(COLUMN_OWNER_TYPE, Value::Number(calling_info.owner_type()));
}
