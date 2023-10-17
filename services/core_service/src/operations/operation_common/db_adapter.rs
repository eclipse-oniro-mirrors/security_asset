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

use asset_common::{definition::{AssetMap, ErrCode, Result, Value, Tag}, loge, logi};
use db_operator::{
    types::DbMap,
    database_table_helper::{
        COLUMN_ACCESSIBILITY, COLUMN_SECRET, COLUMN_ALIAS, COLUMN_AUTH_TYPE,
        COLUMN_SYNC_TYPE, COLUMN_CRITICAL1, COLUMN_CRITICAL2, COLUMN_CRITICAL3,
        COLUMN_CRITICAL4, COLUMN_NORMAL1, COLUMN_NORMAL2, COLUMN_NORMAL3, COLUMN_NORMAL4, COLUMN_REQUIRE_PASSWORD_SET
    }
};
use crate::argument_check::value_check::check_value_validity;

fn convert_value_into_db_value(value: &Value) -> Value {
    match value {
        Value::Bool(b) => Value::Number(*b as u32),
        Value::Number(n) => Value::Number(*n), // todo zwz 类型确认
        Value::Bytes(v) => Value::Bytes(v.to_vec())
    }
}

// todo zwz 尝试与下文映射归一
fn get_tag_column_name(tag: &Tag) -> Option<&'static str> {
    match *tag {
        Tag::Accessibility => Some(COLUMN_ACCESSIBILITY),
        Tag::Secret => Some(COLUMN_SECRET),
        Tag::Alias => Some(COLUMN_ALIAS),
        Tag::AuthType => Some(COLUMN_AUTH_TYPE),
        Tag::SyncType => Some(COLUMN_SYNC_TYPE),
        Tag::DataLabelCritical1 => Some(COLUMN_CRITICAL1),
        Tag::DataLabelCritical2 => Some(COLUMN_CRITICAL2),
        Tag::DataLabelCritical3 => Some(COLUMN_CRITICAL3),
        Tag::DataLabelCritical4 => Some(COLUMN_CRITICAL4),
        Tag::DataLabelNormal1 => Some(COLUMN_NORMAL1),
        Tag::DataLabelNormal2 => Some(COLUMN_NORMAL2),
        Tag::DataLabelNormal3 => Some(COLUMN_NORMAL3),
        Tag::DataLabelNormal4 => Some(COLUMN_NORMAL4),
        Tag::RequirePasswordSet => Some(COLUMN_REQUIRE_PASSWORD_SET),
        _ => None,
    }
}

// todo zwz 尝试与上文映射归一
pub(crate) fn order_by_into_str(order_by: &u32) -> Result<&'static str> {
    match Tag::try_from(*order_by)? {
        Tag::DataLabelNormal1 => Ok(COLUMN_NORMAL1),
        Tag::DataLabelNormal2 => Ok(COLUMN_NORMAL2),
        Tag::DataLabelNormal3 => Ok(COLUMN_NORMAL3),
        Tag::DataLabelNormal4 => Ok(COLUMN_NORMAL4),
        Tag::DataLabelCritical1 => Ok(COLUMN_CRITICAL1),
        Tag::DataLabelCritical2 => Ok(COLUMN_CRITICAL2),
        Tag::DataLabelCritical3 => Ok(COLUMN_CRITICAL3),
        Tag::DataLabelCritical4 => Ok(COLUMN_CRITICAL4),
        _ => {
            loge!("Invalid tag for order by [{}].", order_by);
            Err(ErrCode::InvalidArgument)
        }
    }
}

pub(crate) fn into_db_map(attrs: &AssetMap) -> DbMap {
    let mut db_data = DbMap::new();
    for (tag, value) in attrs.iter() {
        if let Some(column_name) = get_tag_column_name(tag) {
            db_data.insert(column_name, convert_value_into_db_value(value));
        }
    }

    db_data
}

fn convert_db_data_into_asset(tag: &Tag, data: &Value) -> Option<Value> {
    match data {
        Value::Number(i) => {
            match *tag {
                Tag::RequirePasswordSet => Some(Value::Bool(*i != 0)),
                _ => Some(Value::Number(*i)),
            }
        },
        Value::Bytes(t) =>
            Some(Value::Bytes(t.to_vec())),
        _ => None
    }
}

fn convert_db_column_into_tag(column: &str) -> Option<Tag> {
    match column {
        COLUMN_ACCESSIBILITY => Some(Tag::Accessibility),
        COLUMN_SECRET => Some(Tag::Secret),
        COLUMN_ALIAS => Some(Tag::Alias),
        COLUMN_AUTH_TYPE => Some(Tag::AuthType),
        COLUMN_SYNC_TYPE => Some(Tag::SyncType),
        COLUMN_CRITICAL1 => Some(Tag::DataLabelCritical1),
        COLUMN_CRITICAL2 => Some(Tag::DataLabelCritical2),
        COLUMN_CRITICAL3 => Some(Tag::DataLabelCritical3),
        COLUMN_CRITICAL4 => Some(Tag::DataLabelCritical4),
        COLUMN_NORMAL1 => Some(Tag::DataLabelNormal1),
        COLUMN_NORMAL2 => Some(Tag::DataLabelNormal2),
        COLUMN_NORMAL3 => Some(Tag::DataLabelNormal3),
        COLUMN_NORMAL4 => Some(Tag::DataLabelNormal4),
        COLUMN_REQUIRE_PASSWORD_SET => Some(Tag::RequirePasswordSet),
        _ => None,
    }
}

fn insert_db_data_into_asset_map(column: &str, data: &Value, map: &mut AssetMap) -> Result<()> {
    if let Some(tag) = convert_db_column_into_tag(column) {
        match convert_db_data_into_asset(&tag, data) {
            Some(value) => map.insert(tag, value),
            None => {
                logi!("convert [{}] is empty", column); //todo: delete
                None
            }
        };
    }
    Ok(())
}

// todo: yzt 修改到operation_query文件中
pub(crate) fn convert_db_data_into_map(db_results: &Vec<DbMap>) -> Result<Vec<AssetMap>> {
    let mut res_vec = Vec::new();
    for result in db_results {
        let mut map = AssetMap::new();
        for (column, data) in result.iter() {
            insert_db_data_into_asset_map(column, data, &mut map)?;
        }
        check_value_validity(&map)?;
        res_vec.push(map);
    }
    Ok(res_vec)
}
