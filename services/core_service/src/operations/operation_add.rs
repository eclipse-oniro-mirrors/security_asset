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

//! This module is used to insert an Asset with a specified alias.

use asset_constants::OwnerType;
use asset_crypto_manager::crypto::{Crypto, SecretKey};
use asset_db_operator::{
    database::Database,
    database_table_helper::{do_transaction, DatabaseHelper},
    types::{column, DbMap, DB_DATA_VERSION},
};
use asset_definition::{
    asset_error_err, Accessibility, AssetMap, AuthType, ConflictResolution, DeleteType, ErrCode, Extension, Result,
    SyncType, Tag, Value,
};
use asset_log::{loge, logi};

use crate::{calling_info::CallingInfo, operations::common};

fn generate_key_if_needed(secret_key: &SecretKey) -> Result<()> {
    match secret_key.exists() {
        Ok(true) => Ok(()),
        Ok(false) => {
            logi!("[INFO]The key does not exist, generate it.");
            secret_key.generate()
        },
        _ => {
            asset_error_err!(ErrCode::CryptoError, "[FATAL]HUKS failed to check whether the key exists.")
        },
    }
}

fn encrypt(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    let secret_key = common::build_secret_key(calling_info, db_data)?;
    generate_key_if_needed(&secret_key)?;

    let secret = db_data.get_bytes_attr(&column::SECRET)?;
    let cipher = Crypto::encrypt(&secret_key, secret, &common::build_aad(db_data))?;
    db_data.insert(column::SECRET, Value::Bytes(cipher));
    Ok(())
}

fn replace_db_record(calling_info: &CallingInfo, query_db_data: &DbMap, replace_db_data: &DbMap) -> Result<()> {
    let replace_callback = |db: &Database| -> bool {
        if db.delete_datas(query_db_data).is_err() {
            loge!("remove asset in replace operation failed!");
            return false;
        }
        if db.insert_datas(replace_db_data).is_err() {
            loge!("insert asset in replace operation failed!");
            return false;
        }
        true
    };

    if !do_transaction(calling_info.user_id(), replace_callback)? {
        return asset_error_err!(ErrCode::DatabaseError, "[FATAL]Execute do_transaction in replace_db_record failed!");
    }
    Ok(())
}

fn resolve_conflict(calling_info: &CallingInfo, attrs: &AssetMap, query: &DbMap, db_data: &mut DbMap) -> Result<()> {
    match attrs.get(&Tag::ConflictResolution) {
        Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 => {
            encrypt(calling_info, db_data)?;
            replace_db_record(calling_info, query, db_data)
        },
        _ => {
            asset_error_err!(ErrCode::Duplicated, "[FATAL][SA]The specified alias already exists.")
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

    let time = common::get_system_time()?;
    db_data.insert(column::CREATE_TIME, Value::Bytes(time.clone()));
    db_data.insert(column::UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn add_default_attrs(db_data: &mut DbMap) {
    db_data.entry(column::ACCESSIBILITY).or_insert(Value::Number(Accessibility::DeviceFirstUnlocked as u32));
    db_data.entry(column::AUTH_TYPE).or_insert(Value::Number(AuthType::None as u32));
    db_data.entry(column::SYNC_TYPE).or_insert(Value::Number(SyncType::Never as u32));
    db_data.entry(column::REQUIRE_PASSWORD_SET).or_insert(Value::Bool(false));
    let delete_type = DeleteType::WhenUserRemoved as u32 | DeleteType::WhenPackageRemoved as u32;
    db_data.entry(column::DELETE_TYPE).or_insert(Value::Number(delete_type));
}

const REQUIRED_ATTRS: [Tag; 2] = [Tag::Secret, Tag::Alias];

const OPTIONAL_ATTRS: [Tag; 3] = [Tag::Secret, Tag::ConflictResolution, Tag::DeleteType];

fn check_accessibity_validity(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if calling_info.owner_type() != OwnerType::Native as u32 {
        return Ok(())
    }
    let access = attributes.get_enum_attr::<Accessibility>(&Tag::Accessibility)
        .unwrap_or(Accessibility::DeviceFirstUnlocked);
    if access == Accessibility::DevicePowerOn {
        return Ok(());
    }
    asset_error_err!(ErrCode::InvalidArgument, "[FATAL][SA]Native can not use asset with accessibility as {}.", access)
}

fn check_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    common::check_required_tags(attributes, &REQUIRED_ATTRS)?;

    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)?;
    check_accessibity_validity(attributes, calling_info)
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
    if DatabaseHelper::is_data_exists(calling_info.user_id(), &query)? {
        resolve_conflict(calling_info, attributes, &query, &mut db_data)
    } else {
        encrypt(calling_info, &mut db_data)?;
        let insert_num = DatabaseHelper::insert_datas(calling_info.user_id(), &db_data)?;
        logi!("insert {} data", insert_num);
        Ok(())
    }
}
