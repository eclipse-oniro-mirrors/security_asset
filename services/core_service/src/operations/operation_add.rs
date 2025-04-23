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

use std::{ffi::CString, os::raw::c_char};

use asset_common::{CallingInfo, OwnerType};
use asset_crypto_manager::crypto::Crypto;
use asset_db_key_operator::generate_secret_key_if_needed;
use asset_db_operator::{
    database::{build_db, Database},
    types::{column, DbMap, DB_DATA_VERSION},
};
use asset_definition::{
    log_throw_error, Accessibility, AssetMap, AuthType, ConflictResolution, ErrCode, Extension, LocalStatus, Result,
    SyncStatus, SyncType, Tag, Value,
};
use asset_log::logw;
use asset_sdk::WrapType;
use asset_utils::time;

use crate::operations::common;

fn encrypt_secret(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    let secret_key = common::build_secret_key(calling_info, db_data)?;
    generate_secret_key_if_needed(&secret_key)?;

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
            encrypt_secret(calling, db_data)?;
            db.replace_datas(query, false, db_data)
        },
        _ => {
            let mut condition = query.clone();
            condition.insert(column::SYNC_TYPE, Value::Number(SyncType::TrustedAccount as u32));
            condition.insert(column::SYNC_STATUS, Value::Number(SyncStatus::SyncDel as u32));
            if db.is_data_exists(&condition, false)? {
                encrypt_secret(calling, db_data)?;
                db.replace_datas(&condition, false, db_data)
            } else {
                log_throw_error!(ErrCode::Duplicated, "[FATAL][SA]The specified alias already exists.")
            }
        },
    }
}

fn get_query_condition(attrs: &AssetMap) -> Result<DbMap> {
    let alias = attrs.get_bytes_attr(&Tag::Alias)?;
    let mut query = DbMap::new();
    query.insert(column::ALIAS, Value::Bytes(alias.clone()));
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
    db_data.entry(column::ACCESSIBILITY).or_insert(Value::Number(Accessibility::default() as u32));
    db_data.entry(column::AUTH_TYPE).or_insert(Value::Number(AuthType::default() as u32));
    db_data.entry(column::SYNC_TYPE).or_insert(Value::Number(SyncType::default() as u32));
    db_data.entry(column::REQUIRE_PASSWORD_SET).or_insert(Value::Bool(bool::default()));
    db_data.entry(column::IS_PERSISTENT).or_insert(Value::Bool(bool::default()));
    db_data.entry(column::LOCAL_STATUS).or_insert(Value::Number(LocalStatus::Local as u32));
    db_data.entry(column::SYNC_STATUS).or_insert(Value::Number(SyncStatus::SyncAdd as u32));
    db_data.entry(column::WRAP_TYPE).or_insert(Value::Number(WrapType::default() as u32));
}

const REQUIRED_ATTRS: [Tag; 2] = [Tag::Secret, Tag::Alias];
const OPTIONAL_ATTRS: [Tag; 3] = [Tag::Secret, Tag::ConflictResolution, Tag::WrapType];
const SYSTEM_USER_ID_MAX: i32 = 99;

fn check_accessibity_validity(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if calling_info.user_id() > SYSTEM_USER_ID_MAX {
        return Ok(());
    }
    let accessibility =
        attributes.get_enum_attr::<Accessibility>(&Tag::Accessibility).unwrap_or(Accessibility::DeviceFirstUnlocked);
    if accessibility == Accessibility::DevicePowerOn {
        return Ok(());
    }
    log_throw_error!(
        ErrCode::InvalidArgument,
        "[FATAL][SA]System user data cannot be protected by the lock screen password."
    )
}

extern "C" {
    fn CheckPermission(permission: *const c_char) -> bool;
    fn CheckSystemHapPermission() -> bool;
}

fn check_persistent_permission(attributes: &AssetMap) -> Result<()> {
    if attributes.get(&Tag::IsPersistent).is_some() {
        let permission = CString::new("ohos.permission.STORE_PERSISTENT_DATA").unwrap();
        if unsafe { !CheckPermission(permission.as_ptr()) } {
            return log_throw_error!(ErrCode::PermissionDenied, "[FATAL][SA]Permission check failed.");
        }
    }
    Ok(())
}

fn check_sync_permission(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if attributes.get(&Tag::SyncType).is_none()
        || (attributes.get_num_attr(&Tag::SyncType)? & SyncType::TrustedAccount as u32) == 0
    {
        return Ok(());
    }
    match calling_info.owner_type_enum() {
        OwnerType::Hap => {
            if calling_info.app_index() > 0 {
                return log_throw_error!(ErrCode::Unsupported, "[FATAL]The caller does not support storing sync data.");
            }
        },
        OwnerType::HapGroup => {
            return log_throw_error!(ErrCode::Unsupported, "[FATAL]The caller does not support storing sync data.");
        },
        OwnerType::Native => (),
    }
    Ok(())
}

fn check_wrap_permission(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if attributes.get(&Tag::WrapType).is_none()
        || attributes.get_enum_attr::<WrapType>(&Tag::WrapType)? == WrapType::Never
    {
        return Ok(());
    }
    match calling_info.owner_type_enum() {
        OwnerType::Hap | OwnerType::HapGroup => {
            if calling_info.app_index() > 0 {
                return log_throw_error!(ErrCode::Unsupported, "[FATAL]The caller does not support storing wrap data.");
            }
        },
        OwnerType::Native => (),
    }

    if attributes.get(&Tag::SyncType).is_none()
        || (attributes.get_num_attr(&Tag::SyncType)? & SyncType::TrustedAccount as u32) == 0
    {
        Ok(())
    } else {
        log_throw_error!(ErrCode::Unsupported, "[FATAL]trusted account data can not be set need wrap data.")
    }
}

fn check_arguments(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    common::check_required_tags(attributes, &REQUIRED_ATTRS)?;

    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::NORMAL_LOCAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&common::ASSET_SYNC_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_group_validity(attributes, calling_info)?;
    common::check_value_validity(attributes)?;
    check_accessibity_validity(attributes, calling_info)?;
    check_sync_permission(attributes, calling_info)?;
    check_wrap_permission(attributes, calling_info)?;
    common::check_system_permission(attributes)?;
    check_persistent_permission(attributes)
}

fn modify_sync_type(db: &mut DbMap) -> Result<()> {
    if db.get(&column::SYNC_TYPE).is_none()
        || (db.get_num_attr(&column::SYNC_TYPE)? & SyncType::TrustedAccount as u32) == 0
    {
        return Ok(());
    }
    if unsafe { !CheckSystemHapPermission() } {
        logw!("[FATAL]The caller is not system application. Modify store sync type!");
        db.insert(&column::SYNC_TYPE,
            Value::Number(db.get_num_attr(&column::SYNC_TYPE)? - SyncType::TrustedAccount as u32))
    }
    Ok(())
}

fn local_add(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(attributes, calling_info)?;

    // Fill all attributes to DbMap.
    let mut db_data = common::into_db_map(attributes);
    modify_sync_type(&mut db_data);
    common::add_calling_info(calling_info, &mut db_data);
    add_system_attrs(&mut db_data)?;
    add_default_attrs(&mut db_data);
    let query = get_query_condition(attributes)?;

    let mut db = build_db(attributes, calling_info)?;

    if db.is_data_exists(&query, false)? {
        resolve_conflict(calling_info, &mut db, attributes, &query, &mut db_data)?;
    } else {
        encrypt_secret(calling_info, &mut db_data)?;
        let _ = db.insert_datas(&db_data)?;
    }

    Ok(())
}

pub(crate) fn add(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<()> {
    let local_res = local_add(attributes, calling_info);

    common::inform_asset_ext(calling_info, attributes);

    local_res
}
