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

//! This module is used to insert an Asset with a specified alias.

use std::sync::Mutex;

use asset_constants::CallingInfo;
use asset_crypto_manager::{crypto::Crypto, secret_key::SecretKey};
use asset_db_operator::{
    database::Database,
    types::{column, DbMap, DB_DATA_VERSION},
};
use asset_definition::{
    log_throw_error, Availability, AssetMap, AuthType, ConflictResolution, ErrCode, Extension, Result, SyncType, Tag,
    Value,
};
use asset_log::logi;
use asset_utils::time;

use crate::operations::common;

static GEN_KEY_MUTEX: Mutex<()> = Mutex::new(());

fn generate_key_if_needed(secret_key: &SecretKey) -> Result<()> {
    match secret_key.exists() {
        Ok(true) => Ok(()),
        Ok(false) => {
            let _lock = GEN_KEY_MUTEX.lock().unwrap();
            match secret_key.exists() {
                Ok(true) => Ok(()),
                Ok(false) => {
                    logi!("[INFO]The key does not exist, generate it.");
                    secret_key.generate()
                },
                Err(ret) => Err(ret),
            }
        },
        Err(ret) => Err(ret),
    }
}

fn encrypt(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    let secret_key = common::build_secret_key(calling_info, db_data)?;
    generate_key_if_needed(&secret_key)?;

    let secret = db_data.get_bytes_attr(&column::SECRET)?;
    let cipher = Crypto::encrypt(&secret_key, secret, &common::build_aad(db_data)?)?;
    db_data.insert(column::SECRET, Value::Bytes(cipher));
    Ok(())
}

fn resolve_conflict(
    calling: &CallingInfo,
    db: &mut Database,
    attrs: &AssetMap,
    query: &DbMap,
    db_data: &mut DbMap,
) -> Result<()> {
    match attrs.get(&Tag::ConflictResolution) {
        Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 => {
            encrypt(calling, db_data)?;
            db.replace_datas(query, db_data)
        },
        _ => {
            log_throw_error!(ErrCode::Duplicated, "[FATAL][SA]The specified alias already exists.")
        },
    }
}

fn get_query_condition(calling_info: &CallingInfo, attrs: &AssetMap) -> Result<DbMap> {
    let alias = attrs.get_bytes_attr(&Tag::Alias)?;
    let mut query = DbMap::new();
    query.insert(column::ALIAS, Value::Bytes(alias.clone()));
    query.insert(column::OWNER, Value::Bytes(calling_info.owner_info().clone()));
    query.insert(column::OWNER_TYPE, Value::Number(calling_info.owner_type()));
    Ok(query)
}

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    db_data.insert(column::VERSION, Value::Number(DB_DATA_VERSION));

    let time = time::system_time_in_millis()?;
    db_data.insert(column::CREATE_TIME, Value::Bytes(time.clone()));
    db_data.insert(column::UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn add_default_attrs(db_data: &mut DbMap) {
    db_data.entry(column::AVAILABILITY).or_insert(Value::Number(Availability::default() as u32));
    db_data.entry(column::AUTH_TYPE).or_insert(Value::Number(AuthType::default() as u32));
    db_data.entry(column::SYNC_TYPE).or_insert(Value::Number(SyncType::default() as u32));
    db_data.entry(column::REQUIRE_PASSWORD_SET).or_insert(Value::Bool(bool::default()));
    db_data.entry(column::IS_PERSISTENT).or_insert(Value::Bool(bool::default()));
}

const REQUIRED_ATTRS: [Tag; 2] = [Tag::Secret, Tag::Alias];
const OPTIONAL_ATTRS: [Tag; 3] = [Tag::Secret, Tag::ConflictResolution, Tag::IsPersistent];
const SYSTEM_USER_ID_MAX: i32 = 99;

fn check_accessibity_validity(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if calling_info.user_id() > SYSTEM_USER_ID_MAX {
        return Ok(());
    }
    let availability =
        attributes.get_enum_attr::<Availability>(&Tag::Availability).unwrap_or(Availability::DeviceFirstUnlocked);
    if availability == Availability::DevicePowerOn {
        return Ok(());
    }
    log_throw_error!(
        ErrCode::InvalidArgument,
        "[FATAL][SA]System user data cannot be protected by the lock screen password."
    )
}

extern "C" {
    fn CheckPermission() -> bool;
}

fn check_permission(attributes: &AssetMap) -> Result<()> {
    if attributes.get(&Tag::IsPersistent).is_some() && unsafe { !CheckPermission() } {
        return log_throw_error!(ErrCode::PermissionDenied, "[FATAL][SA]Permission check failed.");
    }
    Ok(())
}

fn check_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    common::check_required_tags(attributes, &REQUIRED_ATTRS)?;

    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)?;
    check_accessibity_validity(attributes, calling_info)?;
    check_permission(attributes)
}

pub(crate) fn add(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(attributes, calling_info)?;

    // Create database directory if not exists.
    asset_file_operator::create_user_db_dir(calling_info.user_id())?;

    // Fill all attributes to DbMap.
    let mut db_data = common::into_db_map(attributes);
    common::add_owner_info(calling_info, &mut db_data);
    add_system_attrs(&mut db_data)?;
    add_default_attrs(&mut db_data);

    let query = get_query_condition(calling_info, attributes)?;
    let mut db = Database::build(calling_info.user_id())?;
    if db.is_data_exists(&query)? {
        resolve_conflict(calling_info, &mut db, attributes, &query, &mut db_data)
    } else {
        encrypt(calling_info, &mut db_data)?;
        let _ = db.insert_datas(&db_data)?;
        Ok(())
    }
}
