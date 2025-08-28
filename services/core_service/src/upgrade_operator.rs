/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

//! This module provides interfaces for upgrade clone apps.
//! Databases are isolated based on users and protected by locks.

use std::ffi::{CString, collections::HashSet};
use std::os::raw::c_char;

use asset_common::{CallingInfo, OwnerType, SUCCESS};
use asset_crypto_manager::secret_key::SecretKey;
use asset_definition::{log_throw_error, Accessibility, AuthType, ErrCode, Extension, Result, Value};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_sdk::plugin_interface::{
    EventType, ExtDbMap, PARAM_NAME_AAD, PARAM_NAME_ACCESSIBILITY, PARAM_NAME_APP_INDEX, PARAM_NAME_CIPHER,
    PARAM_NAME_DECRYPT_KEY_ALIAS, PARAM_NAME_ENCRYPT_KEY_ALIAS, PARAM_NAME_USER_ID,
};
use asset_db_operator::{
    database::Database, database_file_upgrade::{
        get_file_content, get_upgrade_list, get_upgrade_version, update_upgrade_list,
        OriginVersion, UpgradeData, is_hap_special
    },
    types::{column, DbMap},
};
use asset_db_key_operator::generate_secret_key_if_needed;

use crate::operations::common;

extern "C" {
    fn GetCloneAppIndexes(userId: i32, appIndexes: *mut i32, indexSize: *mut u32, appName: *const c_char) -> i32;
    fn IsHapInAllowList(userId: i32, appName: *const c_char, isHapInList: &mut bool) -> i32;
}

const DEFAULT_VALUE: i32 = 0;
const DEFAULT_SIZE: usize = 5;
const INIT_INDEX: usize = 1;

struct UnwrapInfo<'a> {
    data: &'a mut DbMap,
    calling_info: &'a CallingInfo,
    calling_info_new: &'a CallingInfo,
    suffix: &'a [u8],
    new_owner: &'a Vec<u8>,
}

/// Upgrade the data of clone apps.
pub fn upgrade_clone_app_data(user_id: i32) -> Result<()> {
    let upgrade_data = get_file_content(user_id)?;
    upgrade(user_id, upgrade_data)
}

/// To upgrade a clone app.
pub fn upgrade_single_clone_app_data(user_id: i32, hap_info: String) -> Result<()> {
    if user_id == 0 {
        return Ok(());
    }
    let parts: Vec<_> = hap_info.split('_').collect();
    if parts.len() < INIT_INDEX + 1 {
        return log_throw_error!(ErrCode::InvalidArgument, "Hap info too short.");
    }
    match parts.last().unwrap().parse::<i32>() {
        Ok(num) => {
            if num == 0 {
                return Ok(());
            }
        },
        Err(_) => return log_throw_error!(ErrCode::InvalidArgument, "Upgrade clone app failed."),
    }
    let version = get_upgrade_version(user_id)?;
    if version == OriginVersion::V2 {
        return Ok(());
    }
    upgrade_single(user_id, version, parts[INIT_INDEX..parts.len() - 1].join("_"));
    Ok(())
}

fn upgrade(user_id: i32, upgrade_data: UpgradeData) -> Result<()> {
    for info in upgrade_data.upgrade_list {
        let version = match upgrade_data.version {
            version if version == OriginVersion::V1 as u32 => OriginVersion::V1,
            version if version == OriginVersion::V2 as u32 => OriginVersion::V2,
            version if version == OriginVersion::V3 as u32 => OriginVersion::V3,
            _ => OriginVersion::V2,
        };
        upgrade_single(user_id, version, info.to_owned());
    }
    Ok(())
}

fn is_hap_in_upgrade_list(user_id: i32, info: &str) -> bool {
    let list = match get_upgrade_list(user_id) {
        Ok(list) => list,
        Err(_) => return false,
    };
    list.contains(&info.to_owned())
}

fn upgrade_single(user_id: i32, version: OriginVersion, info: String) {
    if !is_hap_in_upgrade_list(user_id, &info) {
        return;
    }
    let _ = upgrade_execute(user_id, version.clone(), &info);
}

fn upgrade_execute(user_id: i32, version: OriginVersion, info: &str) -> Result<()> {
    if is_hap_special(info) || (version == OriginVersion::V3 && !(is_hap_in_allowlist(user_id, info)?)) {
        return update_upgrade_list(user_id, &info.to_owned());
    }
    let indexes = get_clone_app_indexes(user_id, info)?;
    if indexes.is_empty() {
        return update_upgrade_list(user_id, &info.to_owned());
    }
    clone_data_from_app_to_clone_app(user_id, info, &indexes)?;
    update_upgrade_list(user_id, &info.to_owned())
}

fn get_clone_app_indexes(user_id: i32, app_name: &str) -> Result<Vec<i32>> {
    let mut indexes: Vec<i32> = vec![DEFAULT_VALUE; DEFAULT_SIZE];
    let app_name_cstr = match CString::new(app_name) {
        Ok(app_name_cstr) => app_name_cstr,
        Err(_) => return log_throw_error!(ErrCode::OutOfMemory, "Create CString failed."),
    };
    let mut index_size = DEFAULT_SIZE as u32;
    let ret = unsafe { GetCloneAppIndexes(user_id, indexes.as_mut_ptr(), &mut index_size, app_name_cstr.as_ptr())};
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::try_from(ret as u32)?, "Get clone app indexes failed.");
    }
    indexes.truncate(index_size as usize);
    Ok(indexes)
}

fn fmt_de_db_name(app_name: &str, app_index: i32) -> String {
    format!("Hap_{}_{}", app_name, app_index)
}

fn clone_data_from_app_to_clone_app(user_id: i32, app_name: &str, app_indexes: &[i32]) -> Result<()> {
    let main_name = fmt_de_db_name(app_name, 0);
    let mut db_main = Database::build_with_file_name(user_id, &main_name, false)?;
    let mut datas: Vec<DbMap> = db_main.query_datas(&vec![], &DbMap::new(), None, false)?;
    if datas.is_empty() {
        return Ok(());
    }
    for index in app_indexes {
        clone_single_app(user_id, app_name, *index, &mut datas)?;
    }
    Ok(())
}

fn is_hap_in_allowlist(user_id: i32, info: &str) -> Result<bool> {
    let app_name = CString::new(info).unwrap();
    let mut is_in_list: bool = false;
    let ret = unsafe { IsHapInAllowList(user_id, app_name.as_ptr(), &mut is_in_list) };
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::try_from(ret as u32)?, "Check hap in allowlist failed.");
    }
    Ok(is_in_list)
}

fn clone_single_app(user_id: i32, app_name: &str, app_index: i32, datas: &mut Vec<DbMap>) -> Result<()> {
    let clone_name = fmt_de_db_name(app_name, app_index);
    let mut db_clone = Database::build_with_file_name(user_id, &clone_name, false)?;
    let db_map = db_clone.query_data_without_lock(&vec![], &DbMap::new(), None, true)?;
    let mut alias_set = HashSet::new();
    for data in db_map {
        alias_set.insert(data.get_bytes_attr(&column::ALIAS)?.clone());
    }
    let mut need_rollback = false;
    let owner_info = datas.first().unwrap().get_bytes_attr(&column::OWNER)?;
    let owner_type = datas.first().unwrap().get_enum_attr::<OwnerType>(&column::OWNER_TYPE)?; 
    let calling_info = CallingInfo::new(user_id, owner_type, owner_info.clone(), None);
    let index = match owner_info.iter().rev().position(|&x| x == b'_') {
        Some(index) => index,
        _ => return log_throw_error!(ErrCode::InvalidArgument, "Owner info is too short."),
    };
    if index >= owner_info.len() - 1 {
        return log_throw_error!(ErrCode::InvalidArgument, "Owner info is too short.");
    }
    let mut new_owner = owner_info[..(owner_info.len() - index)].to_vec();
    let app_index_str = app_index.to_string();
    let suffix = app_index_str.as_bytes();
    new_owner.extend_from_slice(suffix.clone());
    let calling_info_new = CallingInfo::new(user_id, owner_type, new_owner.clone(), None);
    db_clone.exec("begin transaction")?;
    for data in datas {
        if alias_set.contains(data.get_bytes_attr(&column::ALIAS)?) {
            continue;
        }
        let unwrap_info = UnwrapInfo{
            data, calling_info: &calling_info, calling_info_new: &calling_info_new, suffix, new_owner: &new_owner
        };
        if unwrap_and_insert(user_id, unwrap_info, &mut db_clone).is_err() {
            need_rollback = true;
            break;
        }
    }
    if need_rollback {
        db_clone.exec("rollback")?;
        return log_throw_error!(ErrCode::DatabaseError, "Upgrade clone app data failed.");
    }
    db_clone.exec("commit")
}

fn unwrap_and_insert(user_id: i32, unwrap_info: UnwrapInfo, db_clone: &mut Database) -> Result<()> {
    let auth_type = data.get_enum_attr::<AuthType>(&column::AUTH_TYPE)?;
    let accessibility = data.get_enum_attr::<Accessibility>(&column::ACCESSIBILITY)?;
    let required_password_set = data.get_bool_attr(&column::REQUIRE_PASSWORD_SET)?;
    let secret_key = SecretKey::new_without_alias(&calling_info, auth_type, accessibility, required_password_set)?;
    let new_secret_key =
        SecretKey::new_without_alias(&calling_info, auth_type, accessibility, required_password_set)?;
    let _ = generate_secret_key_if_needed(&new_secret_key);
    let _ = generate_secret_key_if_needed(&secret_key);
    let secret = data.get_bytes_attr(&column::SECRET)?;
    let mut params = ExtDbMap::new();
    params.insert(PARAM_NAME_DECRYPT_KEY_ALIAS, Value::Bytes(secret_key.alias().to_vec()));
    params.insert(PARAM_NAME_ENCRYPT_KEY_ALIAS, Value::Bytes(new_secret_key.alias().to_vec()));
    params.insert(PARAM_NAME_ACCESSIBILITY, Value::Number(accessibility as u32));
    params.insert(PARAM_NAME_CIPHER, Value::Bytes(secret.clone()));
    params.insert(PARAM_NAME_AAD, Value::Bytes(common::build_aad(data)?));
    params.insert(PARAM_NAME_APP_INDEX, Value::Bytes(suffix.to_vec()));
    params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
    
    let load = AssetPlugin::get_instance().load_plugin()?;
    match load.process_event(EventType::WrapData, &mut params) {
        Ok(()) => {
            let cipher = params.get_bytes_attr(&PARAM_NAME_CIPHER)?;
            data.insert(column::SECRET, Value::Bytes(cipher.to_vec()));
            data.insert(column::OWNER, Value::Bytes(new_owner));
            if db_clone.insert_datas(data).is_err() {
                return log_throw_error!(ErrCode::CryptoError, "Unwrap the clone data app failed.");
            }
        },
        Err(_) => {
            return log_throw_error!(ErrCode::CryptoError, "Unwrap the clone data app failed.");
        },
    };
    Ok(())
}