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
    types::{Pair, DataValue, AdvancedResultSet, ResultDataValue},
    database::Database,
    database_table_helper::{
        do_transaction,
        DefaultDatabaseHelper,
        G_COLUMN_ACCESS_TYPE, G_COLUMN_SECRET, G_COLUMN_ALIAS, G_COLUMN_AUTH_TYPE,
        G_COLUMN_SYNC_TYPE, G_COLUMN_CRITICAL1, G_COLUMN_CRITICAL2, G_COLUMN_CRITICAL3,
        G_COLUMN_CRITICAL4, G_COLUMN_NORMAL1, G_COLUMN_NORMAL2, G_COLUMN_NORMAL3, G_COLUMN_NORMAL4, G_COLUMN_REQUIRE_PASSWORD_SET
    }
};
use crate::{
    calling_process_info::CallingInfo,
    definition_inner::{AssetInnerMap, InnerValue},
    operations::param_check::value_validity_check::check_value_validity,
};

fn convert_value_into_db_value(value: &Value) -> Result<DataValue> {
    match value {
        Value::Bool(b) => Ok(DataValue::Integer(*b as u32)),
        Value::Number(n) => Ok(DataValue::Integer(*n)), // to do 类型确认
        Value::Bytes(v) => Ok(DataValue::Blob(v))
    }
}

fn convert_extra_value_into_db_value(value: &InnerValue) -> Result<DataValue> {
    match value {
        InnerValue::Number(n) => Ok(DataValue::Integer(*n)), // to do 类型确认
        InnerValue::Blob(v) => Ok(DataValue::Blob(v)),
        InnerValue::Text(v) => Ok(DataValue::Text(v)),
    }
}

fn get_tag_column_name(tag: &Tag) -> Option<&str> {
    match *tag {
        Tag::Accessibility => Some(G_COLUMN_ACCESS_TYPE),
        Tag::Secret => Some(G_COLUMN_SECRET),
        Tag::Alias => Some(G_COLUMN_ALIAS),
        Tag::AuthType => Some(G_COLUMN_AUTH_TYPE),
        Tag::SyncType => Some(G_COLUMN_SYNC_TYPE),
        Tag::DataLabelCritical1 => Some(G_COLUMN_CRITICAL1),
        Tag::DataLabelCritical2 => Some(G_COLUMN_CRITICAL2),
        Tag::DataLabelCritical3 => Some(G_COLUMN_CRITICAL3),
        Tag::DataLabelCritical4 => Some(G_COLUMN_CRITICAL4),
        Tag::DataLabelNormal1 => Some(G_COLUMN_NORMAL1),
        Tag::DataLabelNormal2 => Some(G_COLUMN_NORMAL2),
        Tag::DataLabelNormal3 => Some(G_COLUMN_NORMAL3),
        Tag::DataLabelNormal4 => Some(G_COLUMN_NORMAL4),
        Tag::RequirePasswordSet => Some(G_COLUMN_REQUIRE_PASSWORD_SET),
        _ => None,
    }
}

pub(crate) fn set_input_attr<'a>(input: &'a AssetMap, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    for (tag, value) in input.iter() {
        // skip secret param input, for it should be cipher instead of plain
        if tag == &Tag::Secret || tag == &Tag::Alias {
            continue;
        }
        if let Some(column) = get_tag_column_name(tag) {
            vec.push(
                Pair {
                    column_name: column,
                    value: convert_value_into_db_value(value)?,
                }
            );
        }
    }
    Ok(())
}

/// xxx
pub(crate) fn set_extra_attrs<'a>(input: &'a AssetInnerMap, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    for (tag, value) in input.iter() {
        vec.push(
            Pair {
                column_name: tag,
                value: convert_extra_value_into_db_value(value)?,
            }
        );
    }
    Ok(())
}

pub(crate) fn insert_data_once(alias: &str, calling_info: &CallingInfo, db_data: Vec<Pair>) -> Result<i32> {
    // get owner str
    let owner_str = calling_info.owner_text()?;

    // call sql to add
    let insert_num =
        DefaultDatabaseHelper::insert_datas_default_once(calling_info.user_id(), &owner_str, alias, &db_data)?;

    logi!("insert params calling_info.user_id() = [{}], owner_str = [{}], alias = [{}]", calling_info.user_id(), owner_str, alias); // todo delete

    logi!("insert {} data", insert_num);

    Ok(insert_num)
}

pub(crate) fn replace_data_once(alias: &str, calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<()> {
    // get owner str
    let owner_str = calling_info.owner_text()?;

    let replace_call = |db: &Database| -> bool {
        if db.delete_datas_default(&owner_str, alias, &Vec::new()).is_err() {
            loge!("remove asset in replace operation failed!");
            return false;
        }
        if db.insert_datas_default(&owner_str, alias, db_data).is_err() {
            loge!("insert asset in replace operation failed!");
            return false;
        }
        true
    };

    if !do_transaction(calling_info.user_id(), replace_call)? {
        loge!("do_transaction in replace_data_once failed!");
        return Err(ErrCode::SqliteError);
    }
    Ok(())
}

pub(crate) fn data_exist_once(alias: &str, calling_info: &CallingInfo) -> Result<bool> {
    // get owner str
    let owner_str = calling_info.owner_text()?;
    DefaultDatabaseHelper::is_data_exists_default_once(calling_info.user_id(), &owner_str, alias)
}

pub(crate) fn query_data_once(alias: &str, calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<Vec<AssetMap>> {
    // get owner str
    let owner_str = calling_info.owner_text()?;

    // call sql to add
    let query_res =
        DefaultDatabaseHelper::query_columns_default_once(calling_info.user_id(), &Vec::new(), &owner_str, alias, db_data)?;

    logi!("query params calling_info.user_id() = [{}], owner_str = [{}], alias = [{}], db_data len is [{}]", calling_info.user_id(), owner_str, alias, db_data.len()); // todo delete
    for pair in db_data {
        logi!("db data is [{}]", pair.column_name);
    }
    logi!("query found {}", query_res.len());

    let res_vec = convert_db_data_into_map(&query_res)?;

    Ok(res_vec)
}

pub(crate) fn update_data_once(alias: &str, calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<i32> {
    // get owner str
    let owner_str = calling_info.owner_text()?;

    // call sql to update
    let update_num =
        DefaultDatabaseHelper::update_datas_default_once(calling_info.user_id(), &owner_str, alias, db_data)?;

    logi!("update {} data", update_num);

    Ok(update_num)
}

pub(crate) fn remove_data_once(alias: &str, calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<i32> {
    // get owner str
    let owner_str = calling_info.owner_text()?;

    // call sql to remove
    let remove_num =
        DefaultDatabaseHelper::delete_datas_default_once(calling_info.user_id(), &owner_str, alias, db_data)?;

    logi!("remove {} data", remove_num);

    Ok(remove_num)
}

fn convert_db_data_into_asset(tag: &Tag, data: &ResultDataValue) -> Option<Value> {
    match data {
        ResultDataValue::Integer(i) => {
            match *tag {
                Tag::RequirePasswordSet => Some(Value::Bool(*i != 0)),
                _ => Some(Value::Number(*i)),
            }
        },
        ResultDataValue::Text(t) | ResultDataValue::Blob(t) =>
            t.as_ref().map(|v| Value::Bytes(*v.clone())),
        _ => None
    }
}

fn convert_db_column_into_tag(column: &str) -> Option<Tag> {
    match column {
        G_COLUMN_ACCESS_TYPE => Some(Tag::Accessibility),
        G_COLUMN_SECRET => Some(Tag::Secret),
        G_COLUMN_ALIAS => Some(Tag::Alias),
        G_COLUMN_AUTH_TYPE => Some(Tag::AuthType),
        G_COLUMN_SYNC_TYPE => Some(Tag::SyncType),
        G_COLUMN_CRITICAL1 => Some(Tag::DataLabelCritical1),
        G_COLUMN_CRITICAL2 => Some(Tag::DataLabelCritical2),
        G_COLUMN_CRITICAL3 => Some(Tag::DataLabelCritical3),
        G_COLUMN_CRITICAL4 => Some(Tag::DataLabelCritical4),
        G_COLUMN_NORMAL1 => Some(Tag::DataLabelNormal1),
        G_COLUMN_NORMAL2 => Some(Tag::DataLabelNormal2),
        G_COLUMN_NORMAL3 => Some(Tag::DataLabelNormal3),
        G_COLUMN_NORMAL4 => Some(Tag::DataLabelNormal4),
        G_COLUMN_REQUIRE_PASSWORD_SET => Some(Tag::RequirePasswordSet),
        _ => None,
    }
}

fn insert_db_data_into_asset_map(column: &String, data: &ResultDataValue, map: &mut AssetMap) -> Result<()> {
    if let Some(tag) = convert_db_column_into_tag(column) {
        match convert_db_data_into_asset(&tag, data) {
            Some(value) => map.insert(tag, value),
            None => {
                logi!("convert [{}] is empty", column);
                None
            }
        };
    }
    Ok(())
}

pub(crate) fn convert_db_data_into_map(db_results: &AdvancedResultSet) -> Result<Vec<AssetMap>> {
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
