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

//! This module is used to update the specified alias of Asset.

use asset_crypto_manager::crypto::Crypto;
use asset_db_operator::{
    database_table_helper::DatabaseHelper,
    types::{column, DbMap},
};
use asset_definition::{asset_error_err, AssetMap, ErrCode, Extension, Result, Tag, Value};
use asset_log::logi;

use crate::{calling_info::CallingInfo, operations::common};

fn encrypt(calling_info: &CallingInfo, db_data: &DbMap) -> Result<Vec<u8>> {
    let secret_key = common::build_secret_key(calling_info, db_data)?;
    let secret = db_data.get_bytes_attr(&column::SECRET)?;
    let cipher = Crypto::encrypt(&secret_key, secret, &common::build_aad(db_data))?;
    Ok(cipher)
}

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    let time = common::get_system_time()?;
    db_data.insert(column::UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

const QUERY_REQUIRED_ATTRS: [Tag; 1] = [Tag::Alias];
const UPDATE_OPTIONAL_ATTRS: [Tag; 1] = [Tag::Secret];

fn check_arguments(query: &AssetMap, attrs_to_update: &AssetMap) -> Result<()> {
    // Check attributes used to query.
    common::check_required_tags(query, &QUERY_REQUIRED_ATTRS)?;
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    common::check_tag_validity(query, &valid_tags)?;
    common::check_value_validity(query)?;

    if attrs_to_update.is_empty() {
        return asset_error_err!(ErrCode::InvalidArgument, "[FATAL]The attributes to update is empty.");
    }
    // Check attributes to update.
    valid_tags = common::NORMAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&UPDATE_OPTIONAL_ATTRS);
    common::check_tag_validity(attrs_to_update, &valid_tags)?;
    common::check_value_validity(attrs_to_update)
}

pub(crate) fn update(query: &AssetMap, update: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(query, update)?;

    let mut query_db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut query_db_data);

    let mut update_db_data = common::into_db_map(update);
    add_system_attrs(&mut update_db_data)?;

    if update.contains_key(&Tag::Secret) {
        let mut results = DatabaseHelper::query_columns(calling_info.user_id(), &vec![], &query_db_data, None)?;
        if results.len() != 1 {
            return asset_error_err!(
                ErrCode::NotFound,
                "query to-be-updated asset failed, found [{}] assets",
                results.len()
            );
        }

        let result = results.get_mut(0).unwrap();
        result.insert(column::SECRET, update[&Tag::Secret].clone());
        let cipher = encrypt(calling_info, result)?;
        update_db_data.insert(column::SECRET, Value::Bytes(cipher));
    }

    // call sql to update
    let update_num = DatabaseHelper::update_datas(calling_info.user_id(), &query_db_data, &update_db_data)?;
    if update_num == 0 {
        return asset_error_err!(ErrCode::NotFound, "[FATAL]Update asset failed, update 0 asset.");
    }
    logi!("update {} data", update_num);
    Ok(())
}
