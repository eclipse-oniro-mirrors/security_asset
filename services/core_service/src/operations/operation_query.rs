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

use asset_common::{definition::{AssetMap, Result, Tag, Value, ErrCode, ReturnType}, loge, logi};
use db_operator::{types::{DbMap, QueryOptions}, database_table_helper::DefaultDatabaseHelper};

use crate::{
    argument_check::value_check::check_value_validity,
    calling_info::CallingInfo,
    operations::operation_common::{
        decrypt, add_owner_info, into_db_map, TAG_COLUMN_TABLE
    },
};

fn convert_db_data_into_map(db_results: &Vec<DbMap>) -> Result<Vec<AssetMap>> {
    let mut map_set = Vec::new();
    for db_result in db_results {
        let mut map = AssetMap::new();
        for (column, data) in db_result.iter() {
            for (table_tag, table_column) in TAG_COLUMN_TABLE {
                if (*column).eq(table_column) {
                    map.insert(table_tag, data.clone());
                    break;
                }
            }
        }
        check_value_validity(&map)?;
        map_set.push(map);
    }
    Ok(map_set)
}

fn query_all(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<Vec<AssetMap>> {
    let mut results = DefaultDatabaseHelper::query_columns_default_once(calling_info.user_id(), &vec![], db_data, None)?;
    logi!("results len {}", results.len());
    match results.len() {
        0 => {
            loge!("[FATAL]The data to be queried does not exist.");
            Err(ErrCode::NotFound)
        },
        1 => {
            decrypt(calling_info, &mut results[0])?;
            convert_db_data_into_map(&results)
        }
        n => {
            loge!("[FATAL]The database contains {} records with the specified alias.", n);
            Err(ErrCode::SqliteError)
        },
    }
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
            Some(Value::Number(order_by)) => {
                let order_tag = Tag::try_from(*order_by).expect("Tag::ReturnOrderBy has been verified");
                let mut order_by = None;
                for (tag, column) in TAG_COLUMN_TABLE {
                    if order_tag == tag {
                        order_by = Some(vec![column]);
                        break;
                    }
                }
                order_by
            }
            _ => None,
        },
    }
}

pub(crate) fn query_attrs(calling_info: &CallingInfo, db_data: &DbMap, attrs: &AssetMap) -> Result<Vec<AssetMap>> {
    let results = DefaultDatabaseHelper::query_columns_default_once(calling_info.user_id(),
        &vec![], db_data, Some(&get_query_options(attrs)))?;
    logi!("query found {}", results.len());
    if results.is_empty() {
        loge!("[FATAL]The data to be queried does not exist.");
        return Err(ErrCode::NotFound)
    }

    let mut results = convert_db_data_into_map(&results)?;
    for data in &mut results {
        data.remove(&Tag::Secret);
    }
    Ok(results)
}

pub(crate) fn query(query: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<AssetMap>> {
    let mut db_data = into_db_map(query);
    add_owner_info(calling_info, &mut db_data);

    match query.get(&Tag::ReturnType) {
        Some(Value::Number(return_type)) if *return_type == (ReturnType::All as u32) => {
            if !query.contains_key(&Tag::Alias)  {
                loge!("[FATAL]Batch secret query is not supported.");
                Err(ErrCode::NotSupport)
            } else {
                query_all(calling_info, &mut db_data)
            }
        },
        _ => query_attrs(calling_info, &db_data, query)
    }
}
