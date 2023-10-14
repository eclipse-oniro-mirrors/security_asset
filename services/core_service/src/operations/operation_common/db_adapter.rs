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
    types::{Pair, DataValue, AdvancedResultSet, ResultDataValue, QueryOptions},
    database::Database,
    database_table_helper::{
        do_transaction,
        DefaultDatabaseHelper,
        G_COLUMN_ACCESSIBILITY, G_COLUMN_SECRET, G_COLUMN_ALIAS, G_COLUMN_AUTH_TYPE,
        G_COLUMN_SYNC_TYPE, G_COLUMN_CRITICAL1, G_COLUMN_CRITICAL2, G_COLUMN_CRITICAL3,
        G_COLUMN_CRITICAL4, G_COLUMN_NORMAL1, G_COLUMN_NORMAL2, G_COLUMN_NORMAL3, G_COLUMN_NORMAL4, G_COLUMN_REQUIRE_PASSWORD_SET
    }
};
use crate::{
    calling_info::CallingInfo,
    operations::operation_common::extra_params::add_extra_db_data,
    argument_check::value_check::check_value_validity,
    definition_inner::OperationCode,
};

fn convert_value_into_db_value(value: &Value) -> Result<DataValue> {
    match value {
        Value::Bool(b) => Ok(DataValue::Integer(*b as u32)),
        Value::Number(n) => Ok(DataValue::Integer(*n)), // to do 类型确认
        Value::Bytes(v) => Ok(DataValue::Blob(v.to_vec()))
    }
}

// todo zwz 尝试与下文映射归一
fn get_tag_column_name(tag: &Tag) -> Option<&'static str> {
    match *tag {
        Tag::Accessibility => Some(G_COLUMN_ACCESSIBILITY),
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

// todo zwz 尝试与上文映射归一
fn order_by_into_str(order_by: &u32) -> Result<&'static str> {
    match Tag::try_from(*order_by)? {
        Tag::DataLabelNormal1 => Ok(G_COLUMN_NORMAL1),
        Tag::DataLabelNormal2 => Ok(G_COLUMN_NORMAL2),
        Tag::DataLabelNormal3 => Ok(G_COLUMN_NORMAL3),
        Tag::DataLabelNormal4 => Ok(G_COLUMN_NORMAL4),
        Tag::DataLabelCritical1 => Ok(G_COLUMN_CRITICAL1),
        Tag::DataLabelCritical2 => Ok(G_COLUMN_CRITICAL2),
        Tag::DataLabelCritical3 => Ok(G_COLUMN_CRITICAL3),
        Tag::DataLabelCritical4 => Ok(G_COLUMN_CRITICAL4),
        _ => {
            loge!("Invalid tag for order by [{}].", order_by);
            Err(ErrCode::InvalidArgument)
        }
    }
}

pub(crate) fn set_input_attr(input: &AssetMap, vec: &mut Vec<Pair<>>) -> Result<()> {
    for (tag, value) in input.iter() {
        // skip secret param input, for it should be cipher instead of plain
        if tag == &Tag::Secret {
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

pub(crate) fn construct_db_data(input: &AssetMap, calling_info: &CallingInfo, code: &OperationCode)
    -> Result<Vec<Pair<>>> {
    let mut db_data = Vec::new();
    set_input_attr(input, &mut db_data)?;
    add_extra_db_data(calling_info, code, &mut db_data)?;
    Ok(db_data)
}

pub(crate) fn insert_data_once(calling_info: &CallingInfo, db_data: Vec<Pair>) -> Result<i32> {
    DefaultDatabaseHelper::insert_datas_default_once(calling_info.user_id(), &db_data)
}

pub(crate) fn replace_data_once(calling_info: &CallingInfo, query_db_data: &Vec<Pair<>>,
    replace_db_data: &Vec<Pair>) -> Result<()> {
    let replace_call = |db: &Database| -> bool {
        if db.delete_datas_default(query_db_data).is_err() {
            loge!("remove asset in replace operation failed!");
            return false;
        }
        if db.insert_datas_default(replace_db_data).is_err() {
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

pub(crate) fn data_exist_once(calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<bool> {
    DefaultDatabaseHelper::is_data_exists_default_once(calling_info.user_id(), db_data)
}

fn get_query_options(input: &AssetMap) -> QueryOptions {
    QueryOptions {
        offset: match input.get(&Tag::ReturnOffset) {
            Some(Value::Number(offset)) => Some(*offset),
            _ => None,
        },
        limit: match input.get(&Tag::ReturnLimit) {
            Some(Value::Number(limit)) => Some(*limit),
            _ => None,
        },
        order: None,
        order_by: match input.get(&Tag::ReturnOrderBy) {
            Some(Value::Number(limit)) => {
                match order_by_into_str(limit) {
                    Ok(res) => Some(vec![res]),
                    Err(_) => None,
                }
            }
            _ => None,
        },
    }
}

pub(crate) fn query_data_once(calling_info: &CallingInfo, db_data: &Vec<Pair>, input: &AssetMap)
    -> Result<Vec<AssetMap>> {
    // call sql to add
    let query_res = DefaultDatabaseHelper::query_columns_default_once(calling_info.user_id(),
        &Vec::new(), db_data, Some(&get_query_options(input)))?;

    logi!("query found {}", query_res.len());

    convert_db_data_into_map(&query_res)
}

pub(crate) fn update_data_once(calling_info: &CallingInfo, query_db_data: &Vec<Pair>, update_db_data: &Vec<Pair>) -> Result<i32> {
    // call sql to update
    DefaultDatabaseHelper::update_datas_default_once(calling_info.user_id(), query_db_data, update_db_data)
}

pub(crate) fn remove_data_once(calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<i32> {
    DefaultDatabaseHelper::delete_datas_default_once(calling_info.user_id(), db_data)
}

fn convert_db_data_into_asset(tag: &Tag, data: &ResultDataValue) -> Option<Value> {
    match data {
        ResultDataValue::Integer(i) => {
            match *tag {
                Tag::RequirePasswordSet => Some(Value::Bool(*i != 0)),
                _ => Some(Value::Number(*i)),
            }
        },
        ResultDataValue::Blob(t) =>
            Some(Value::Bytes(*t.clone())),
        _ => None
    }
}

fn convert_db_column_into_tag(column: &str) -> Option<Tag> {
    match column {
        G_COLUMN_ACCESSIBILITY => Some(Tag::Accessibility),
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
                logi!("convert [{}] is empty", column); //todo: delete
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
