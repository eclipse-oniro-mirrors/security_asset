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

use std::{fs, path::Path};

use asset_common::{CallingInfo, OwnerType};
use asset_definition::{log_throw_error, ErrCode, Extension, Result, Value};
use asset_file_operator::common::DB_SUFFIX;
use asset_log::logi;

use crate::{
    database::{
        fmt_backup_path, fmt_de_db_path_with_name, get_db, get_db_by_type, get_split_db_lock_by_user_id, Database,
        CE_ROOT_PATH, DE_ROOT_PATH, OLD_DB_NAME,
    },
    types::{column, DbMap, QueryOptions, DB_UPGRADE_VERSION_V3},
};

const MINIM_OWNER_INFO_LEN: usize = 3;
const REMOVE_INDEX: usize = 2;
static MAX_BATCH_NUM: u32 = 100;

#[inline(always)]
pub(crate) fn fmt_old_de_db_path(user_id: i32) -> String {
    format!("{}/{}/asset.db", DE_ROOT_PATH, user_id)
}

fn check_old_db_exist(user_id: i32) -> bool {
    let path_str = fmt_old_de_db_path(user_id);
    let path = Path::new(&path_str);
    path.exists()
}

/// Use owner_type and owner_info construct db name.
pub fn construct_splited_db_name(calling_info: &CallingInfo, is_ce: bool) -> Result<String> {
    let mut res: String = match calling_info.owner_type_enum() {
        OwnerType::Hap => {
            if let Some(group) = calling_info.group() {
                format!("Group_{}", String::from_utf8_lossy(group))
            } else {
                let owner_info_string = String::from_utf8_lossy(calling_info.owner_info()).to_string();
                let split_owner_info: Vec<&str> = owner_info_string.split('_').collect();
                if split_owner_info.len() < MINIM_OWNER_INFO_LEN || split_owner_info.last().is_none() {
                    return log_throw_error!(ErrCode::DatabaseError, "[FATAL]The queried owner info is not correct.");
                }
                let app_index = split_owner_info.last().unwrap();
                let mut split_owner_info_mut = split_owner_info.clone();
                for _ in 0..REMOVE_INDEX {
                    split_owner_info_mut.pop();
                }
                let owner_info = split_owner_info_mut.join("_").clone();
                format!("Hap_{}_{}", owner_info, app_index)
            }
        },
        OwnerType::Native => {
            format!("Native_{}", String::from_utf8_lossy(calling_info.owner_info()))
        },
    };
    if is_ce {
        res = format!("enc_{}", res)
    }
    Ok(res)
}

fn get_db_before_split(user_id: i32) -> Result<Database> {
    let db_path = fmt_de_db_path_with_name(user_id, OLD_DB_NAME);
    get_db_by_type(user_id, OLD_DB_NAME, db_path, DB_UPGRADE_VERSION_V3, None)
}

fn get_value_from_db_map(db_map: &DbMap, key: &str) -> Result<Value> {
    match db_map.get(key) {
        Some(value) => Ok(value.clone()),
        _ => log_throw_error!(ErrCode::DatabaseError, "[FATAL]Get value from {} failed.", key),
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
    get_db_by_type(user_id, &new_db_name, db_path, DB_UPGRADE_VERSION_V3, None)
}

/// Trigger upgrade of database version and renaming secret key alias.
pub fn trigger_db_upgrade(user_id: i32, is_ce: bool) -> Result<()> {
    let path = if is_ce {
        format!("{}/{}/asset_service", CE_ROOT_PATH, user_id)
    } else {
        format!("{}/{}", DE_ROOT_PATH, user_id)
    };
    for entry in fs::read_dir(path)? {
        let entry = entry?;
        if entry.file_name().to_string_lossy().ends_with(DB_SUFFIX) {
            if let Some(file_name_stem) = entry.file_name().to_string_lossy().strip_suffix(DB_SUFFIX) {
                let _ = get_db(user_id, file_name_stem, is_ce)?;
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
    let query_options = QueryOptions { offset: None, limit: Some(MAX_BATCH_NUM), order_by: None, order: None };

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
    if check_old_db_exist(user_id) {
        let _lock = get_split_db_lock_by_user_id(user_id).mtx.lock().unwrap();
        if check_old_db_exist(user_id) {
            logi!("[INFO]Start splitting db.");
            split_db(user_id)?;
        }
    }
    trigger_db_upgrade(user_id, false)?;
    Ok(())
}
