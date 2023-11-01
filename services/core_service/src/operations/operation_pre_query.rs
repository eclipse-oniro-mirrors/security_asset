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

//! This module prepares for querying Asset that required secondary identity authentication.

use asset_crypto_manager::{
    crypto::{get_valiad_challenge, set_valiad_challenge, Crypto, CryptoManager, SecretKey},
    huks_ffi::{CHALLENGE_LEN, HKS_KEY_PURPOSE_DECRYPT},
};
use asset_db_operator::{
    database_table_helper::DatabaseHelper,
    types::{column, DbMap},
};
use asset_definition::{asset_error_err, Accessibility, AssetMap, AuthType, ErrCode, Extension, Result, Tag, Value};
use asset_hasher::sha256;

use crate::{calling_info::CallingInfo, operations::common};

const OPTIONAL_ATTRS: [Tag; 1] = [Tag::AuthValidityPeriod];
const DEFAULT_AUTH_VALIDITY: u32 = 60;

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)?;

    match attributes.get(&Tag::AuthType) {
        Some(Value::Number(val)) if *val == (AuthType::None as u32) => {
            asset_error_err!(ErrCode::InvalidArgument, "[FATAL][SA]Pre Query AuthType invalid.")
        },
        _ => Ok(()),
    }
}

fn query_access_types(calling_info: &CallingInfo, db_data: &DbMap) -> Result<Vec<Accessibility>> {
    let results = DatabaseHelper::query_columns(calling_info.user_id(), &vec![column::ACCESSIBILITY], db_data, None)?;
    if results.is_empty() {
        return asset_error_err!(ErrCode::NotFound, "[FATAL][SA]The data to be queried does not exist.");
    }

    let mut access_types = Vec::new();
    for db_result in results {
        match db_result.get(&column::ACCESSIBILITY) {
            Some(Value::Number(access_type)) => access_types.push(Accessibility::try_from(*access_type)?),
            _ => {
                return asset_error_err!(ErrCode::InvalidArgument, "[FATAL][SA]Pre Query Accessibility invalid.");
            },
        }
    }
    Ok(access_types)
}

pub(crate) fn pre_query(query: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<u8>> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);
    db_data.entry(column::AUTH_TYPE).or_insert(Value::Number(AuthType::Any as u32));

    let access_types = query_access_types(calling_info, &db_data)?;
    if access_types.is_empty() {
        return asset_error_err!(ErrCode::NotFound, "[FATAL][SA]Pre Query result not found.");
    }

    let valid_time = query.get_num_attr(&Tag::AuthValidityPeriod).unwrap_or(DEFAULT_AUTH_VALIDITY);
    let owner_hash = sha256(calling_info.owner_info());
    let mut challenge = vec![0; CHALLENGE_LEN as usize];
    let mut cryptos = Vec::with_capacity(4);
    for (idx, access_type) in access_types.iter().enumerate() {
        let secret_key = SecretKey::new(calling_info.user_id(), &owner_hash, AuthType::Any, *access_type);
        let mut crypto = Crypto::new(HKS_KEY_PURPOSE_DECRYPT, secret_key, idx as u32, valid_time);

        let tmp_challenge = crypto.init_crypto()?;
        set_valiad_challenge(get_valiad_challenge(&tmp_challenge, idx), idx, &mut challenge);
        cryptos.push(crypto);
    }

    let arc_crypto_manager = CryptoManager::get_instance();
    let mut crypto_manager = arc_crypto_manager.lock().unwrap();
    for crypto in cryptos {
        crypto_manager.add(crypto)?;
    }
    Ok(challenge)
}
