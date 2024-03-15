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

//! This file implement the file operations.

use std::{fs::{self, DirEntry}, path::Path};

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_log::logi;

use asset_db_operator::database::Database;

const ROOT_PATH: &str = "data/service/el1/public/asset_service";
const ASSET_DB: &str = "asset.db";
const BACKUP_SUFFIX: &str = ".backup";

fn construct_user_path(user_id: i32) -> String {
    format!("{}/{}", ROOT_PATH, user_id)
}

/// Check user db dir exist.
pub fn is_user_db_dir_exist(user_id: i32) -> bool {
    let path_str = construct_user_path(user_id);
    let path: &Path = Path::new(&path_str);
    path.exists()
}

/// Create user database directory.
pub fn create_user_db_dir(user_id: i32) -> Result<()> {
    if is_user_db_dir_exist(user_id) {
        return Ok(());
    }

    logi!("[INFO]Directory is not exist, create it...");
    let path_str = construct_user_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::create_dir(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Create dir failed! error is [{}]", e)
        },
    }
}

/// Delete user database directory.
pub fn delete_user_db_dir(user_id: i32) -> Result<()> {
    if !is_user_db_dir_exist(user_id) {
        return Ok(());
    }

    let path_str = construct_user_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::remove_dir_all(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() != std::io::ErrorKind::NotFound => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Delete dir failed! error is [{}]", e)
        },
    }
}

/// Backup database if the database is available
pub fn backup_db_if_available(entry: &DirEntry, user_id: i32) -> Result<()> {
    let from_path = entry.path().with_file_name(format!("{}/{}", user_id, ASSET_DB)).to_string_lossy().to_string();
    Database::check_db_available(from_path.clone(), user_id)?;
    let backup_path = format!("{}{}", from_path, BACKUP_SUFFIX);
    fs::copy(from_path, backup_path)?;
    Ok(())
}