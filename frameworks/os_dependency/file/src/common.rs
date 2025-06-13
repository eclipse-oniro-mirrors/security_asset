/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

//! This file implements ce file operations.

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_log::logi;
use std::{fs, path::Path, slice, sync::Mutex};
use crate::de_operator::{read_record_time, write_record_time};
use lazy_static::lazy_static; 


const SUCCESS: i32 = 0;
const USER_ID_VEC_BUFFER: u32 = 5;
const MINIMUM_MAIN_USER_ID: i32 = 100;
/// Suffix for backup database files.
pub const BACKUP_SUFFIX: &str = ".backup";
/// Suffix for database files.
pub const DB_SUFFIX: &str = ".db";
/// Name for data base key ciphertext file.
pub const DB_KEY: &str = "db_key";
/// Root path to de user directories.
pub const DE_ROOT_PATH: &str = "data/service/el1/public/asset_service";
/// Root path to ce user directories.
pub const CE_ROOT_PATH: &str = "data/service/el2";
/// Root path to de clone directories.
pub const DE_CLONE_PATH: &str = "data/service/el1/public/asset_clone";

lazy_static! {
    static ref RECORD_UNIX_FILE_MUTEX: Mutex<()> = Mutex::new(());
}

/// Get all db name in user directory.
pub(crate) fn get_user_dbs(path_str: &str) -> Result<Vec<String>> {
    let mut dbs = vec![];
    for db_path in fs::read_dir(path_str)? {
        let db_path = db_path?;
        let db_file_name = db_path.file_name().to_string_lossy().to_string();
        if db_file_name.ends_with(DB_SUFFIX) {
            dbs.push(db_file_name.strip_suffix(DB_SUFFIX).unwrap_or(&db_file_name).to_string())
        }
    }
    Ok(dbs)
}

/// Check whether file exists.
pub fn is_file_exist(path_str: &str) -> Result<bool> {
    let path: &Path = Path::new(&path_str);
    match path.try_exists() {
        Ok(true) => Ok(true),
        Ok(false) => Ok(false),
        Err(e) => {
            log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][SA]]Checking existence of database key ciphertext file failed! error is [{}]",
                e
            )
        },
    }
}

extern "C" {
    fn GetUserIds(userIdsPtr: *mut i32, userIdsSize: *mut u32) -> i32;
    fn GetUsersSize(userIdsSize: *mut u32) -> i32;
}

/// get all asset user db
pub fn get_db_dirs() -> Result<Vec<String>> {
    let mut dirs = vec![];
    dirs.push(String::from(DE_ROOT_PATH));
    dirs.push(String::from(DE_CLONE_PATH));

    let mut user_ids_size: u32 = 0;
    let user_ids_size_ptr = &mut user_ids_size;
    let mut ret: i32 = unsafe{ GetUsersSize(user_ids_size_ptr) };
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::AccountError, "[FATAL] Get users size failed.");
    }

    let mut user_ids: Vec<i32> = vec![0i32; (*user_ids_size_ptr + USER_ID_VEC_BUFFER).try_into().unwrap()];
    let user_ids_ptr = user_ids.as_mut_ptr();
    ret = unsafe{ GetUserIds(user_ids_ptr, user_ids_size_ptr) };
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::AccountError, "[FATAL] Get users IDs failed.");
    }

    let user_ids_slice = unsafe { slice::from_raw_parts(user_ids_ptr, user_ids_size.try_into().unwrap()) };
    for user_id in user_ids_slice {
        if *user_id < MINIMUM_MAIN_USER_ID {
            continue;
        }
        let ce_path = format!("{}/{}/asset_service", CE_ROOT_PATH, user_id);
        dirs.push(ce_path);
    }
    Ok(dirs)
}

/// check time for uploading data size
pub fn should_upload_data_size(unix_time: u64) -> Result<bool> {
    let path_str = format!("{}/record_unix_time.txt", DE_ROOT_PATH);
    let _lock = RECORD_UNIX_FILE_MUTEX.lock().unwrap();

    match is_file_exist(&path_str) {
        Ok(true) => {
            let prev_time = read_record_time(&path_str)?;
            if unix_time - prev_time > 86400 {
                write_record_time(&path_str, unix_time)?;
                Ok(true)
            }
            else {
                Ok(false)
            }
        }
        Ok(false) => {
            write_record_time(&path_str, unix_time)?;
            Ok(true)
        }
        Err(e) => Err(e)
    }
}