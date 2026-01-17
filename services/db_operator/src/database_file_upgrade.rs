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

//! This module provides interfaces for database management.
//! Databases are isolated based on users and protected by locks.

use std::fs::File;
use std::fs::OpenOptions;
use std::{fs, path::Path, sync::Mutex, os::unix::fs::{OpenOptionsExt, PermissionsExt}};

use asset_common::{CallingInfo, OwnerType};
use asset_definition::{macros_lib, AssetError, ErrCode, Extension, Result, Value};
use asset_file_operator::common::DB_SUFFIX;
use asset_log::logi;
use asset_utils::hasher;
use lazy_static::lazy_static;
use serde::{Deserialize, Serialize};
use ylong_json::{to_writer, from_reader};

use crate::database::get_specific_db_version;
use crate::{
    database::{
        fmt_backup_path, fmt_de_db_path_with_name, get_db, get_db_by_type, get_split_db_lock_by_user_id, Database,
        CE_ROOT_PATH, DE_ROOT_PATH, OLD_DB_NAME,
    },
    database_util::construct_hap_owner_info,
    types::{column, DB_UPGRADE_VERSION, DB_UPGRADE_VERSION_V3, DbMap, QueryOptions},
};

const TRUNCATE_LEN: usize = 4;
static MAX_BATCH_NUM: u32 = 100;

/// Code used to identify the origin version.
#[derive(Clone, Debug, PartialEq)]
pub enum OriginVersion {
    /// Code for version 5.0.1.
    V1 = 1,
    /// Code for version 5.0.2.
    V2 = 2,
    /// Code for other versions.
    V3 = 3,
}

/// Struct used to identify the original version and the list of haps to be upgraded.
#[derive(Debug, Serialize, Deserialize, PartialEq, Default)]
pub struct UpgradeData {
    /// The original version.
    pub version: u32,
    /// The list of haps to be upgraded.
    pub upgrade_list: Vec<String>,
    /// ce to be upgraded.
    pub ce_upgrade: Option<String>,
}

lazy_static! {
    static ref GLOBAL_FILE_LOCK: Mutex<()> = Mutex::new(());
}

#[inline(always)]
pub(crate) fn fmt_old_de_db_path(user_id: i32) -> String {
    format!("{}/{}/asset.db", DE_ROOT_PATH, user_id)
}

fn check_old_db_exist(user_id: i32) -> bool {
    let path_str = fmt_old_de_db_path(user_id);
    let path = Path::new(&path_str);
    path.exists()
}

fn to_hex(bytes: &Vec<u8>) -> Vec<u8> {
    let bytes_len = bytes.len();
    let scale_capacity = 2;
    let mut hex_vec = Vec::with_capacity(bytes_len * scale_capacity);
    for byte in bytes.iter() {
        hex_vec.extend(format!("{:02x}", byte).as_bytes());
    }
    hex_vec
}

/// Use owner_type and owner_info construct db name.
pub fn construct_splited_db_name(calling_info: &CallingInfo, is_ce: bool) -> Result<String> {
    let mut res: String = match calling_info.owner_type_enum() {
        OwnerType::HapGroup => match (calling_info.developer_id(), calling_info.group_id()) {
            (Some(developer_id), Some(group_id)) => format!(
                "Group_{}_{}",
                String::from_utf8_lossy(developer_id).trim_end_matches('\0'),
                String::from_utf8_lossy(&to_hex(&hasher::sha256(true, group_id)))
            ),
            _ => return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
                "[FATAL]Wrong queried owner group info."),
        },
        OwnerType::Hap => {
            construct_hap_owner_info(calling_info.owner_info())?
        },
        OwnerType::Native => format!("Native_{}", String::from_utf8_lossy(calling_info.owner_info())),
    };
    if is_ce {
        res = format!("enc_{}", res)
    }
    Ok(res)
}

fn get_db_before_split(user_id: i32) -> Result<Database> {
    let db_path = fmt_de_db_path_with_name(user_id, OLD_DB_NAME);
    get_db_by_type(user_id, OLD_DB_NAME, db_path, None)
}

fn get_value_from_db_map(db_map: &DbMap, key: &str) -> Result<Value> {
    match db_map.get(key) {
        Some(value) => Ok(value.clone()),
        _ => macros_lib::log_throw_error!(ErrCode::DatabaseError, "[FATAL]Get value from {} failed.", key),
    }
}

fn remove_old_db(user_id: i32) -> Result<()> {
    let mut remove_db_files = vec![];
    let path = fmt_de_db_path_with_name(user_id, OLD_DB_NAME);
    remove_db_files.push(path.clone());
    remove_db_files.push(fmt_backup_path(path.as_str()));
    for file_path in &remove_db_files {
        fs::remove_file(file_path)?;
    }
    Ok(())
}

fn get_new_db(user_id: i32, info_map: &DbMap) -> Result<Database> {
    // 1.1 construct db name
    let owner_type = OwnerType::try_from(info_map.get_num_attr(&column::OWNER_TYPE)?.to_owned())?;
    let owner_info = info_map.get_bytes_attr(&column::OWNER)?;
    let calling_info = CallingInfo::new(user_id, owner_type, owner_info.to_vec(), None);
    let new_db_name = construct_splited_db_name(&calling_info, false)?;
    // 1.2 construct new db
    let db_path = fmt_de_db_path_with_name(user_id, &new_db_name);
    get_db_by_type(user_id, &new_db_name, db_path, None)
}

/// Trigger upgrade of database version and renaming secret key alias.
pub fn trigger_db_upgrade(user_id: i32, db_key: Option<Vec<u8>>) -> Result<()> {
    let path = if db_key.is_none() {
        format!("{}/{}", DE_ROOT_PATH, user_id)
    } else {
        format!("{}/{}/asset_service", CE_ROOT_PATH, user_id)
    };
    for entry in fs::read_dir(path)? {
        let entry = entry?;
        if entry.file_name().to_string_lossy().ends_with(DB_SUFFIX) {
            if let Some(file_name_stem) = entry.file_name().to_string_lossy().strip_suffix(DB_SUFFIX) {
                let _ = get_db(user_id, file_name_stem, &db_key)?;
            }
        }
    }
    Ok(())
}

fn construct_old_query_condition(info_map: &DbMap) -> Result<DbMap> {
    let mut old_data_query_condition = DbMap::new();
    let owner_info = info_map.get_bytes_attr(&column::OWNER)?;
    old_data_query_condition.insert(column::OWNER, Value::Bytes(owner_info.clone()));
    Ok(old_data_query_condition)
}

fn calculate_batch_split_times(old_data_query_condition: &DbMap, old_db: &mut Database) -> Result<u32> {
    let query_times = (old_db.query_data_count(old_data_query_condition)? + MAX_BATCH_NUM - 1) / MAX_BATCH_NUM;
    Ok(query_times)
}

fn migrate_data(
    old_db: &mut Database,
    new_db: &mut Database,
    split_time: u32,
    old_data_query_condition: &DbMap,
) -> Result<()> {
    // 3.1 query data in old db
    let query_options =
        QueryOptions { offset: None, limit: Some(MAX_BATCH_NUM), order_by: None, order: None, amend: None };

    let old_data_vec = old_db.query_datas(&vec![], old_data_query_condition, Some(&query_options), false)?;
    // 3.2 insert data in new db
    for data in &old_data_vec {
        let mut condition = DbMap::new();
        condition.insert(column::ALIAS, get_value_from_db_map(data, column::ALIAS)?);
        condition.insert(column::OWNER, get_value_from_db_map(data, column::OWNER)?);
        condition.insert(column::OWNER_TYPE, get_value_from_db_map(data, column::OWNER_TYPE)?);
        let mut data_clone = data.clone();
        data_clone.remove(column::ID);
        new_db.replace_datas(&condition, false, &data_clone)?;
        // 3.3 remove data in old db
        old_db.delete_datas(&condition, None, false)?;
    }
    logi!("[INFO]Upgrade [{}] [{}]times", new_db.get_db_name(), split_time);
    Ok(())
}

fn split_db(user_id: i32) -> Result<()> {
    // 1. open old db
    let mut old_db = get_db_before_split(user_id)?;

    // 2. get split db info
    let empty_condition = DbMap::new();
    let owner_info_db_list =
        old_db.query_datas(&vec![column::OWNER_TYPE, column::OWNER], &empty_condition, None, false)?;
    for info_map in &owner_info_db_list {
        // 1. get new db
        let mut new_db = get_new_db(user_id, info_map)?;
        // 2. batch insert data from old db to new db.
        let old_data_query_condition = construct_old_query_condition(info_map)?;
        for split_time in 0..calculate_batch_split_times(&old_data_query_condition, &mut old_db)? {
            migrate_data(&mut old_db, &mut new_db, split_time, &old_data_query_condition)?;
        }
        logi!("[INFO]Upgrade [{}] success!", new_db.get_db_name());
    }
    logi!("[INFO]Upgrade all db success!");
    remove_old_db(user_id)?;
    Ok(())
}

/// check db need split or not. If needed, split it by owner.
pub fn check_and_split_db(user_id: i32) -> Result<()> {
    let mut ret: bool = false;
    if check_old_db_exist(user_id) {
        let _lock = get_split_db_lock_by_user_id(user_id).mtx.lock().unwrap();
        if check_old_db_exist(user_id) {
            logi!("[INFO]Start splitting db.");
            split_db(user_id)?;
            ret = create_upgrade_file(user_id, OriginVersion::V1).is_ok();
        }
    }
    let folder_name = fmt_file_dir(user_id);
    let folder = Path::new(&folder_name);
    let mut db_version = DB_UPGRADE_VERSION;
    let mut is_clone_app_exist: bool = false;
    if !is_upgrade_file_exist(user_id) {
        if let Ok(entries) = fs::read_dir(folder) {
            let mut entry_version = String::new();
            for path in entries {
                let entry = match path {
                    Ok(e) => e.path().to_string_lossy().into_owned(),
                    Err(_) => continue,
                };
                if entry.contains("_1.db") && !is_hap_special(&entry) {
                    is_clone_app_exist = true;
                }
                if entry.contains("_0.db") {
                    entry_version = entry.clone();
                }
            }

            if !entry_version.is_empty() && entry_version.contains(DB_SUFFIX) {
                let index = entry_version.rfind('/').unwrap();
                let index_last = entry_version.rfind(DB_SUFFIX).unwrap();
                if index_last > index + 1 {
                    let db_name = &entry_version[index + 1..index_last];
                    db_version = get_specific_db_version(user_id, db_name, fmt_de_db_path_with_name(user_id, db_name))?;
                }
            }
        }
    }

    if !ret && db_version == DB_UPGRADE_VERSION_V3 && !is_upgrade_file_exist(user_id) {
        if !is_clone_app_exist {
            ret = create_upgrade_file(user_id, OriginVersion::V1).is_ok();
        } else {
            ret = create_upgrade_file(user_id, OriginVersion::V2).is_ok();
        }
    }
    if !ret && !is_upgrade_file_exist(user_id) {
        let _ = create_upgrade_file(user_id, OriginVersion::V3);
    }

    Ok(())
}

#[inline(always)]
fn fmt_file_path(user_id: i32) -> String {
    format!("{}/{}/upgrade.cache", DE_ROOT_PATH, user_id)
}

#[inline(always)]
fn fmt_file_dir(user_id: i32) -> String {
    format!("{}/{}", DE_ROOT_PATH, user_id)
}

/// Check if hap is a special hap.
pub fn is_hap_special(info: &str) -> bool {
    info.contains("com.alipay.mobile.client")
}

/// Function to create an upgrade file.
pub fn create_upgrade_file(user_id: i32, origin_version: OriginVersion) -> Result<()> {
    let _lock = GLOBAL_FILE_LOCK.lock().unwrap();
    let path_str = fmt_file_path(user_id);
    let file_path = Path::new(&path_str);
    let mut file = match OpenOptions::new().write(true).create(true).open(file_path) {
        Ok(file) => file,
        Err(_) => {
            return macros_lib::log_throw_error!(ErrCode::FileOperationError,
                "Create file failed in create_upgrade_file.");
        },
    };
    let _ = fs::set_permissions(file_path, fs::Permissions::from_mode(0o640));
    let upgrade_list = create_upgrade_list_inner(user_id, &origin_version);
    let content = UpgradeData { version: origin_version as u32, upgrade_list, ce_upgrade: None };
    to_writer(&content, &mut file).map_err(|e| macros_lib::log_and_into_asset_error!(
        ErrCode::FileOperationError, "Write file failed in create_upgrade_file. error: {}", e))
}

/// Check if upgrade file exists.
pub fn is_upgrade_file_exist(user_id: i32) -> bool {
    let _lock = GLOBAL_FILE_LOCK.lock().unwrap();
    let file_name = fmt_file_path(user_id);
    let file_path = Path::new(&file_name);
    file_path.exists()
}

/// To get original version and the list of haps to be upgraded.
pub fn get_file_content(user_id: i32) -> Result<UpgradeData> {
    let _lock = GLOBAL_FILE_LOCK.lock().unwrap();
    let path = fmt_file_path(user_id);
    let file_path = Path::new(&path);
    let _ = fs::set_permissions(file_path, fs::Permissions::from_mode(0o640));
    let file = File::open(path)?;
    match from_reader(file) {
        Ok(content) => Ok(content),
        Err(_) => macros_lib::log_throw_error!(ErrCode::FileOperationError, "Get content from upgrade file failed."),
    }
}

/// To get original version.
pub fn get_upgrade_version(user_id: i32) -> Result<OriginVersion> {
    match get_file_content(user_id)?.version {
        version if version == OriginVersion::V1 as u32 => Ok(OriginVersion::V1),
        version if version == OriginVersion::V2 as u32 => Ok(OriginVersion::V2),
        version if version == OriginVersion::V3 as u32 => Ok(OriginVersion::V3),
        _ => Err(AssetError { code: ErrCode::FileOperationError, msg: "Get upgrade version failed.".to_owned() }),
    }
}

/// To get the list of haps to be upgraded.
pub fn get_upgrade_list(user_id: i32) -> Result<Vec<String>> {
    Ok(get_file_content(user_id)?.upgrade_list)
}

fn is_file_hap(path: &Path) -> bool {
    path.file_name()
        .and_then(|os_str| os_str.to_str())
        .map_or(false, |name| name.starts_with("Hap_") && name.ends_with("_0.db"))
}

fn create_upgrade_list_inner(user_id: i32, version: &OriginVersion) -> Vec<String> {
    if user_id == 0 || version == &OriginVersion::V2 {
        return Vec::new();
    }
    let folder_name = fmt_file_dir(user_id);
    let folder = Path::new(&folder_name);

    let entries = match fs::read_dir(folder) {
        Ok(entries) => entries,
        Err(_) => return Vec::new(),
    };

    let mut upgrade_list = Vec::new();
    for entry in entries {
        let path = match entry {
            Ok(e) => e.path(),
            Err(_) => continue,
        };
        if !is_file_hap(&path) {
            continue;
        }
        let file_name = path.to_string_lossy().into_owned();
        let index_first = match file_name.find("Hap_") {
            Some(index) => index,
            None => continue,
        };

        let index_last = match file_name.rfind('_') {
            Some(index) => index,
            None => continue,
        };
        if index_first >= index_last {
            continue;
        }
        let owner = file_name[(index_first + TRUNCATE_LEN)..index_last].to_owned();
        upgrade_list.push(owner);
    }
    upgrade_list
}

/// save UpgradeData to file
pub fn save_to_writer(user_id: i32, content: &UpgradeData) -> Result<()> {
    let _lock = GLOBAL_FILE_LOCK.lock().unwrap();
    let path_str = fmt_file_path(user_id);
    let file_path = Path::new(&path_str);
    let _ = fs::set_permissions(file_path, fs::Permissions::from_mode(0o640));
    let mut file = match OpenOptions::new().write(true).create(true).truncate(true).mode(0o640).open(file_path) {
        Ok(file) => file,
        Err(_) => {
            return macros_lib::log_throw_error!(ErrCode::FileOperationError, "Create file failed.");
        },
    };
    to_writer(&content, &mut file).map_err(|e| macros_lib::log_and_into_asset_error!(
        ErrCode::FileOperationError, "Write file failed in update_upgrade_list. error: {}", e))
}

/// Update the list of haps to be upgraded.
pub fn update_upgrade_list(user_id: i32, remove_file: &String) -> Result<()> {
    let content = get_file_content(user_id)?;
    let mut upgrade_list = content.upgrade_list;
    upgrade_list.retain(|x| x != remove_file);
    
    let content = UpgradeData { version: content.version as u32, upgrade_list, ce_upgrade: content.ce_upgrade };
    save_to_writer(user_id, &content)
}
