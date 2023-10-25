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

//! This module is used to query the Asset, including single and batch query.

use asset_db_operator::{
    database_table_helper::{DatabaseHelper, COLUMN_AUTH_TYPE, COLUMN_SECRET},
    types::{DbMap, QueryOptions},
};
use asset_definition::{AssetMap, AuthType, ErrCode, Result, ReturnType, Tag, Value};
use asset_log::{loge, logi};

use crate::{calling_info::CallingInfo, operations::common};

fn into_asset_maps(db_results: &Vec<DbMap>) -> Result<Vec<AssetMap>> {
    let mut map_set = Vec::new();
    for db_result in db_results {
        let map = common::into_asset_map(db_result);
        common::check_value_validity(&map)?;
        map_set.push(map);
    }
    Ok(map_set)
}

fn query_all(calling_info: &CallingInfo, db_data: &mut DbMap, query: &AssetMap) -> Result<Vec<AssetMap>> {
    let mut results = DatabaseHelper::query_columns(calling_info.user_id(), &vec![], db_data, None)?;
    logi!("results len {}", results.len());
    match results.len() {
        0 => {
            loge!("[FATAL]The data to be queried does not exist.");
            Err(ErrCode::NotFound)
        },
        1 => {
            match results[0].get(COLUMN_AUTH_TYPE) {
                Some(Value::Number(auth_type)) if *auth_type == AuthType::Any as u32 => {
                    common::check_required_tags(query, &SEC_QUERY_OPTIONAL_ATTRS)?;
                    let Some(Value::Bytes(ref challenge)) = query.get(&Tag::AuthChallenge) else {
                        return Err(ErrCode::InvalidArgument);
                    };
                    let Some(Value::Bytes(ref auth_token)) = query.get(&Tag::AuthToken) else {
                        return Err(ErrCode::InvalidArgument);
                    };
                    common::exec_crypto(calling_info, &mut results[0], challenge, auth_token)?;
                    loge!("enter secdond query 4"); // todo delete
                    into_asset_maps(&results)
                },
                _ => {
                    common::decrypt(calling_info, &mut results[0])?;
                    into_asset_maps(&results)
                },
            }
        },
        n => {
            loge!("[FATAL]The database contains {} records with the specified alias.", n);
            Err(ErrCode::SqliteError)
        },
    }
}

fn get_query_options(attrs: &AssetMap) -> QueryOptions {
    QueryOptions {
        offset: match attrs.get(&Tag::ReturnOffset) {
            Some(Value::Number(offset)) => Some(*offset),
            _ => None,
        },
        limit: match attrs.get(&Tag::ReturnLimit) {
            Some(Value::Number(limit)) => Some(*limit),
            _ => None,
        },
        order: None,
        order_by: match attrs.get(&Tag::ReturnOrderBy) {
            Some(Value::Number(order_by)) => {
                let tag = Tag::try_from(*order_by).expect("Tag::ReturnOrderBy has been verified");
                common::get_cloumn_name(tag).map(|order_by| vec![order_by])
            },
            _ => None,
        },
    }
}

pub(crate) fn query_attrs(calling_info: &CallingInfo, db_data: &DbMap, attrs: &AssetMap) -> Result<Vec<AssetMap>> {
    let mut results =
        DatabaseHelper::query_columns(calling_info.user_id(), &vec![], db_data, Some(&get_query_options(attrs)))?;
    logi!("query found {}", results.len());
    if results.is_empty() {
        loge!("[FATAL]The data to be queried does not exist.");
        return Err(ErrCode::NotFound);
    }

    for data in &mut results {
        data.remove(&COLUMN_SECRET);
    }

    into_asset_maps(&results)
}

const OPTIONAL_ATTRS: [Tag; 6] =
    [Tag::ReturnLimit, Tag::ReturnOffset, Tag::ReturnOrderBy, Tag::ReturnType, Tag::AuthToken, Tag::AuthChallenge];
const SEC_QUERY_OPTIONAL_ATTRS: [Tag; 2] = [Tag::AuthChallenge, Tag::AuthToken];

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)
}

pub(crate) fn query(query: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<AssetMap>> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);

    match query.get(&Tag::ReturnType) {
        Some(Value::Number(return_type)) if *return_type == (ReturnType::All as u32) => {
            if !query.contains_key(&Tag::Alias) {
                loge!("[FATAL]Batch secret query is not supported.");
                Err(ErrCode::NotSupport)
            } else {
                query_all(calling_info, &mut db_data, query)
            }
        },
        _ => query_attrs(calling_info, &db_data, query),
    }
}
