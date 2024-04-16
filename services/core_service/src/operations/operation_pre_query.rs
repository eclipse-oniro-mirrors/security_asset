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

//! This module prepares for querying Asset that required secondary identity authentication.

use asset_constants::CallingInfo;
use asset_crypto_manager::{crypto::Crypto, crypto_manager::CryptoManager, secret_key::SecretKey};
use asset_db_operator::{
    database::Database,
    types::{column, DbMap},
};
use asset_definition::{log_throw_error, Accessibility, AssetMap, AuthType, ErrCode, Extension, Result, Tag, Value};

use crate::operations::common;

const OPTIONAL_ATTRS: [Tag; 2] = [Tag::AuthValidityPeriod, Tag::SpecificUserId];
const DEFAULT_AUTH_VALIDITY_IN_SECS: u32 = 60;

fn check_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);

    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)?;
    common::check_system_permission_if_needed(calling_info.has_specific_user_id())?;

    match attributes.get(&Tag::AuthType) {
        Some(Value::Number(val)) if *val == (AuthType::None as u32) => {
            log_throw_error!(ErrCode::InvalidArgument, "[FATAL][SA]Pre Query AuthType invalid.")
        },
        _ => Ok(()),
    }
}

fn query_key_attrs(calling_info: &CallingInfo, db_data: &DbMap) -> Result<(Accessibility, bool)> {
    let results = Database::build(calling_info.stored_user_id())?.query_datas(
        &vec![column::ACCESSIBILITY, column::REQUIRE_PASSWORD_SET],
        db_data,
        None,
    )?;
    match results.len() {
        0 => log_throw_error!(ErrCode::NotFound, "[FATAL][SA]No data that meets the query conditions is found."),
        1 => {
            let access_type = results[0].get_enum_attr::<Accessibility>(&column::ACCESSIBILITY)?;
            let require_password_set = results[0].get_bool_attr(&column::REQUIRE_PASSWORD_SET)?;
            Ok((access_type, require_password_set))
        },
        _ => log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][SA]Data of multiple access control types cannot be accessed at the same time."
        ),
    }
}

pub(crate) fn pre_query(query: &AssetMap, calling_info: &mut CallingInfo) -> Result<Vec<u8>> {
    if let Some(Value::Number(num)) = query.get(&Tag::SpecificUserId) {
        calling_info.set_specific_user_id(*num as i32)?;
    }
    check_arguments(query, calling_info)?;

    // Check database directory exist.
    if !asset_file_operator::is_user_db_dir_exist(calling_info.stored_user_id()) {
        return log_throw_error!(ErrCode::NotFound, "[FATAL][SA]No data that meets the query conditions is found.");
    }

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);
    db_data.entry(column::AUTH_TYPE).or_insert(Value::Number(AuthType::Any as u32));

    let (access_type, require_password_set) = query_key_attrs(calling_info, &db_data)?;
    let valid_time = match query.get(&Tag::AuthValidityPeriod) {
        Some(Value::Number(num)) => *num,
        _ => DEFAULT_AUTH_VALIDITY_IN_SECS,
    };
    let secret_key = SecretKey::new(calling_info, AuthType::Any, access_type, require_password_set);
    let mut crypto = Crypto::build(secret_key, valid_time)?;
    let challenge = crypto.init_key()?.to_vec();
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().add(crypto)?;
    Ok(challenge)
}
