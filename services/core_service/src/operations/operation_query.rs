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

//! This module is used to query the Asset, including single and batch query.

use std::{cmp::Ordering, collections::HashSet};

use asset_common::CallingInfo;
use asset_crypto_manager::{crypto::Crypto, crypto_manager::CryptoManager};
use asset_db_operator::{
    database::{build_db, Database},
    types::{column, DbMap, QueryOptions, DB_DATA_VERSION},
};
use asset_definition::{
    log_throw_error, throw_error, AssetMap, AuthType, ErrCode, Extension, Result, ReturnType, Tag, Value,
};

use crate::operations::common;

fn into_asset_maps(db_results: &Vec<DbMap>) -> Result<Vec<AssetMap>> {
    let mut map_set = Vec::new();
    for db_result in db_results {
        let map = common::into_asset_map(db_result);
        common::check_value_validity(&map)?;
        map_set.push(map);
    }
    Ok(map_set)
}

fn upgrade_version(db: &mut Database, calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    db_data.insert_attr(column::VERSION, DB_DATA_VERSION);
    let secret = db_data.get_bytes_attr(&column::SECRET)?;
    let secret_key = common::build_secret_key(calling_info, db_data)?;
    let cipher = Crypto::encrypt(&secret_key, secret, &common::build_aad(db_data)?)?;

    let mut update_data = DbMap::new();
    update_data.insert(column::SECRET, Value::Bytes(cipher));
    update_data.insert_attr(column::VERSION, DB_DATA_VERSION);

    let mut query_data = DbMap::new();
    query_data.insert_attr(column::ID, db_data.get_num_attr(&column::ID)?);

    let update_num = db.update_datas(&query_data, true, &update_data)?;
    if update_num == 0 {
        return log_throw_error!(ErrCode::NotFound, "[FATAL]Upgrade asset failed.");
    }
    Ok(())
}

fn decrypt_secret(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    let secret = db_data.get_bytes_attr(&column::SECRET)?;
    let secret_key = common::build_secret_key(calling_info, db_data)?;
    let secret = Crypto::decrypt(&secret_key, secret, &common::build_aad(db_data)?)?;
    db_data.insert(column::SECRET, Value::Bytes(secret));
    Ok(())
}

fn exec_crypto(calling_info: &CallingInfo, query: &AssetMap, db_data: &mut DbMap) -> Result<()> {
    common::check_required_tags(query, &AUTH_QUERY_ATTRS)?;
    let challenge = query.get_bytes_attr(&Tag::AuthChallenge)?;
    let auth_token = query.get_bytes_attr(&Tag::AuthToken)?;

    let secret = db_data.get_bytes_attr(&column::SECRET)?;
    let arc_crypto_manager = CryptoManager::get_instance();
    let mut manager = arc_crypto_manager.lock().unwrap();
    match manager.find(calling_info, challenge) {
        Ok(crypto) => {
            let secret = crypto.exec_crypt(secret, &common::build_aad(db_data)?, auth_token)?;
            db_data.insert(column::SECRET, Value::Bytes(secret));
            Ok(())
        },
        Err(e) => Err(e),
    }
}

fn query_all(calling_info: &CallingInfo, db_data: &mut DbMap, query: &AssetMap) -> Result<Vec<AssetMap>> {
    let mut db = build_db(query, calling_info)?;
    let mut results = db.query_datas(&vec![], db_data, None, true)?;
    match results.len() {
        0 => throw_error!(ErrCode::NotFound, "[FATAL]The data to be queried does not exist."),
        1 => {
            match results[0].get(column::AUTH_TYPE) {
                Some(Value::Number(auth_type)) if *auth_type == AuthType::Any as u32 => {
                    exec_crypto(calling_info, query, &mut results[0])?;
                },
                _ => decrypt_secret(calling_info, &mut results[0])?,
            };
            if common::need_upgrade(&results[0])? {
                upgrade_version(&mut db, calling_info, &mut results[0])?;
            }
            into_asset_maps(&results)
        },
        n => {
            log_throw_error!(
                ErrCode::DatabaseError,
                "[FATAL]The database contains {} records with the specified alias.",
                n
            )
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
        order_by: match attrs.get(&Tag::ReturnOrderedBy) {
            Some(Value::Number(order_by)) => {
                let tag = Tag::try_from(*order_by).expect("Tag::ReturnOrderBy has been verified");
                common::get_cloumn_name(tag).map(|order_by| vec![order_by])
            },
            _ => None,
        },
        order: {
            if attrs.contains_key(&Tag::ReturnOrderedBy) {
                Some(Ordering::Greater)
            } else {
                None
            }
        },
        amend: None,
    }
}

pub(crate) fn query_attrs(calling_info: &CallingInfo, db_data: &DbMap, attrs: &AssetMap) -> Result<Vec<AssetMap>> {
    let mut db = build_db(attrs, calling_info)?;
    let mut results = db.query_datas(&vec![], db_data, Some(&get_query_options(attrs)), true)?;
    if results.is_empty() {
        return throw_error!(ErrCode::NotFound, "[FATAL]The data to be queried does not exist.");
    }

    for data in &mut results {
        data.remove(&column::SECRET);
    }

    into_asset_maps(&results)
}

const OPTIONAL_ATTRS: [Tag; 6] =
    [Tag::ReturnLimit, Tag::ReturnOffset, Tag::ReturnOrderedBy, Tag::ReturnType, Tag::AuthToken, Tag::AuthChallenge];
const AUTH_QUERY_ATTRS: [Tag; 2] = [Tag::AuthChallenge, Tag::AuthToken];

fn check_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    let mut valid_tags = HashSet::new();
    valid_tags.extend(common::CRITICAL_LABEL_ATTRS.iter());
    valid_tags.extend(common::NORMAL_LABEL_ATTRS.iter());
    valid_tags.extend(common::NORMAL_LOCAL_LABEL_ATTRS.iter());
    valid_tags.extend(common::ACCESS_CONTROL_ATTRS.iter());
    valid_tags.extend(common::ASSET_SYNC_ATTRS.iter());
    valid_tags.extend(OPTIONAL_ATTRS.iter());

    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_group_validity(attributes, calling_info)?;
    common::check_value_validity(attributes)?;
    common::check_system_permission(attributes)
}

pub(crate) fn query(calling_info: &CallingInfo, query: &AssetMap) -> Result<Vec<AssetMap>> {
    check_arguments(query, calling_info)?;

    common::inform_asset_ext(calling_info, query);

    let mut db_data = common::into_db_map(query);
    if query.get(&Tag::GroupId).is_some() {
        common::add_group(calling_info, &mut db_data);
    } else {
        common::add_owner_info(calling_info, &mut db_data);
    }

    match query.get(&Tag::ReturnType) {
        Some(Value::Number(return_type)) if *return_type == (ReturnType::All as u32) => {
            if !query.contains_key(&Tag::Alias) {
                log_throw_error!(ErrCode::Unsupported, "[FATAL]Batch secret query is not supported.")
            } else {
                query_all(calling_info, &mut db_data, query)
            }
        },
        _ => query_attrs(calling_info, &db_data, query),
    }
}
