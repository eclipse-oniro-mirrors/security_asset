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

//! This module is used to get Asset file size.

use crate::sys_event::{upload_data_size, PARTITION};
use asset_common::SUCCESS;
use asset_definition::{log_throw_error, ErrCode, Result};
use asset_file_operator::common::{is_file_exist, CE_ROOT_PATH, DE_ROOT_PATH};
use asset_utils::time::system_time_in_seconds;
use lazy_static::lazy_static;
use std::{ffi::CString, fs, os::raw::c_char, path::Path, sync::Mutex};

/// The buffer for userId vec.
pub const USER_ID_VEC_BUFFER: u32 = 5;
/// Asset migration path.
pub const MIGRATION_PATH: &str = "data/service/el1/public/asset_migration";
/// One day secs.
pub const ONE_DAY_SECS: u64 = 86400;

extern "C" {
    fn GetUserIds(userIdsPtr: *mut i32, userIdsSize: *mut u32) -> i32;
    fn GetRemainPartitionSize(partition_name: *const c_char, partition_size: *mut f64) -> i32;
    fn GetDirSize(dir: *const c_char) -> u64;
}

lazy_static! {
    static ref RECORD_UNIX_FILE_MUTEX: Mutex<()> = Mutex::new(());
}

/// get remain partition size
fn get_remain_partition_size(partition: &str) -> Result<f64> {
    let partition_cstr = CString::new(partition).unwrap();
    let mut remain_size: f64 = 0.0;
    let ret_code: i32 = unsafe { GetRemainPartitionSize(partition_cstr.as_ptr(), &mut remain_size) };
    if ret_code != 0 {
        return log_throw_error!(ErrCode::try_from(ret_code as u32)?, "Get remain partition size failed");
    }
    Ok(remain_size)
}

/// get all asset folders size
fn get_folders_size(paths: &[String]) -> Result<Vec<u64>> {
    let mut folders_size = vec![];

    for folder_path in paths.iter() {
        let path_cstr = CString::new(folder_path.as_str()).unwrap();
        let folder_size: u64 = unsafe { GetDirSize(path_cstr.as_ptr()) };
        folders_size.push(folder_size);
    }
    Ok(folders_size)
}

/// read record unix time
fn read_record_time(path_str: &str) -> Result<u64> {
    let path: &Path = Path::new(&path_str);
    let time_str = fs::read_to_string(path)?;
    let trim_time = time_str.trim();
    match trim_time.parse::<u64>() {
        Ok(unix_time) => Ok(unix_time),
        Err(_) => {
            eprintln!("[WARNING] Failed to parse time from file. Return 0 as a default");
            Ok(0)
        },
    }
}

/// write record unix time
fn write_record_time(path_str: &str, unix_time: u64) -> Result<()> {
    let path: &Path = Path::new(&path_str);
    let time_str = unix_time.to_string();
    fs::write(path, time_str)?;
    Ok(())
}

/// get all asset user db
fn get_db_dirs() -> Result<Vec<String>> {
    let mut dirs = vec![];
    dirs.push(String::from(DE_ROOT_PATH));
    if is_file_exist(MIGRATION_PATH)? {
        dirs.push(String::from(MIGRATION_PATH));
    }

    let mut user_ids_size: u32 = USER_ID_VEC_BUFFER;
    let mut user_ids: Vec<i32> = vec![0i32; user_ids_size as usize];
    let user_ids_size_ptr = &mut user_ids_size;
    let user_ids_ptr = user_ids.as_mut_ptr();
    let ret: i32 = unsafe { GetUserIds(user_ids_ptr, user_ids_size_ptr) };
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::AccountError, "[FATAL] Get users IDs failed.");
    }

    if user_ids_size < USER_ID_VEC_BUFFER {
        user_ids.truncate(user_ids_size as usize);
    }
    for user_id in &user_ids {
        let ce_path = format!("{}/{}/asset_service", CE_ROOT_PATH, user_id);
        if is_file_exist(&ce_path)? {
            dirs.push(ce_path);
        }
    }
    Ok(dirs)
}

/// check time for uploading data size
fn should_upload_data_size(path_str: &str, unix_time: u64) -> bool {
    match is_file_exist(path_str) {
        Ok(true) => {
            let prev_time = match read_record_time(path_str) {
                Ok(prev_time) => prev_time,
                Err(_) => return true,
            };
            if unix_time >= prev_time && unix_time - prev_time < ONE_DAY_SECS {
                return false;
            }
            true
        },
        _ => true,
    }
}

/// handle data upload
pub(crate) fn handle_data_size_upload() -> Result<()> {
    let unix_time = system_time_in_seconds()?;
    let path_str = format!("{}/record_unix_time.txt", DE_ROOT_PATH);
    let _lock = RECORD_UNIX_FILE_MUTEX.lock().unwrap();
    if should_upload_data_size(&path_str, unix_time) {
        write_record_time(&path_str, unix_time)?;
        let folder_path = get_db_dirs()?;
        let folders_size = get_folders_size(&folder_path)?;
        let remain_size = get_remain_partition_size(PARTITION)?;
        upload_data_size(remain_size, folder_path, folders_size);
    }
    Ok(())
}
