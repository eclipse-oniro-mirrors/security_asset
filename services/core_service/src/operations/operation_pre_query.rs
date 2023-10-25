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
    crypto::{Crypto, CryptoManager, SecretKey},
    huks_ffi::{CHALLENGE_LEN, HKS_KEY_PURPOSE_DECRYPT},
};
use asset_db_operator::{
    database_table_helper::{DatabaseHelper, COLUMN_ACCESSIBILITY, COLUMN_AUTH_TYPE},
    types::DbMap,
};
use asset_definition::{Accessibility, AssetMap, AuthType, ErrCode, Result, Tag, Value};
use asset_hasher::sha256;
use asset_log::{loge, logi};

use crate::{calling_info::CallingInfo, operations::common};

const OPTIONAL_ATTRS: [Tag; 1] = [Tag::AuthValidityPeriod];

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)?;

    let auth_type = AuthType::Any as u32;
    match attributes.get(&Tag::AuthType) {
        Some(Value::Number(val)) if *val == auth_type => Ok(()),
        None => Ok(()),
        _ => Err(ErrCode::InvalidArgument),
    }
}

fn query_access_types(calling_info: &CallingInfo, db_data: &DbMap) -> Result<Vec<Accessibility>> {
    let results = DatabaseHelper::query_columns(calling_info.user_id(), &vec![COLUMN_ACCESSIBILITY], db_data, None)?;
    logi!("query found {}", results.len());
    if results.is_empty() {
        loge!("[FATAL]The data to be queried does not exist.");
        return Err(ErrCode::NotFound);
    }

    // into list
    let mut access_types = Vec::new();
    for db_result in results {
        match db_result.get(&COLUMN_ACCESSIBILITY) {
            Some(Value::Number(access_type)) => access_types.push(Accessibility::try_from(*access_type)?),
            _ => return Err(ErrCode::InvalidArgument),
        }
    }
    Ok(access_types)
}

pub(crate) fn pre_query(query: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<u8>> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);
    db_data.entry(COLUMN_AUTH_TYPE).or_insert(Value::Number(AuthType::Any as u32));

    let access_types = query_access_types(calling_info, &db_data)?;

    if access_types.is_empty() {
        return Err(ErrCode::NotFound);
    }

    let mut challenge = vec![0; CHALLENGE_LEN as usize];
    let mut cryptos = Vec::with_capacity(4);
    for (idx, access_type) in access_types.iter().enumerate() {
        // get_or_default
        let Value::Number(exp_time) = query.get(&Tag::AuthValidityPeriod).unwrap_or(&Value::Number(60)) else {
            return Err(ErrCode::InvalidArgument);
        };

        let secret_key =
            SecretKey::new(calling_info.user_id(), &sha256(calling_info.owner_info()), AuthType::Any, *access_type);
        let mut crypto = Crypto::new(HKS_KEY_PURPOSE_DECRYPT, secret_key, idx as u32, *exp_time);

        match crypto.init_crypto() {
            Ok(the_challenge) => {
                challenge[(idx * 8)..((idx + 1) * 8)].copy_from_slice(&the_challenge[(idx * 8)..((idx + 1) * 8)]);
            },
            Err(e) => return Err(e),
        }
        cryptos.push(crypto);
    }

    // todo crypto manager的获取需要改用单例模式
    let mut crypto_manager = CryptoManager::new();
    for crypto in cryptos {
        crypto_manager.add(crypto)?;
    }
    logi!("get challenge successful!"); // todo delete
    Ok(challenge)
}
