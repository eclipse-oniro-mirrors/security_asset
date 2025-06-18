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

//! This file implements de file operations.

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_log::logi;
use std::{fs, path::Path};

use crate::common::{get_user_dbs, is_file_exist};

fn construct_user_de_path(user_id: i32) -> String {
    format!("data/service/el1/public/asset_service/{}", user_id)
}

fn is_user_de_dir_exist(user_id: i32) -> Result<bool> {
    let path_str = construct_user_de_path(user_id);
    is_file_exist(&path_str)
}

/// Create user de directory.
pub fn create_user_de_dir(user_id: i32) -> Result<()> {
    if is_user_de_dir_exist(user_id)? {
        return Ok(());
    }

    logi!("[INFO]User DE directory does not exist, create it...");
    let path_str = construct_user_de_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::create_dir(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => Ok(()),
        Err(e) => {
            log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][SA]Create user DE directory failed! error is [{}]",
                e
            )
        },
    }
}

/// Delete user de directory.
pub fn delete_user_de_dir(user_id: i32) -> Result<()> {
    if !is_user_de_dir_exist(user_id)? {
        return Ok(());
    }

    let path_str = construct_user_de_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::remove_dir_all(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() != std::io::ErrorKind::NotFound => Ok(()),
        Err(e) => {
            log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][SA]Delete user DE directory failed! error is [{}]",
                e
            )
        },
    }
}

/// Obtain de user dbs
pub fn get_de_user_dbs(user_id: i32) -> Result<Vec<String>> {
    get_user_dbs(&construct_user_de_path(user_id))
}

/// read record unix time 
pub fn read_record_time(path_str: &str) -> Result<u64> {
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
pub fn write_record_time(path_str: &str, unix_time: u64) -> Result<()> {
    let path: &Path = Path::new(&path_str);
    let time_str = unix_time.to_string();
    fs::write(path, time_str)?;
    Ok(())
}