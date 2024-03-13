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

const ROOT_PATH: &str = "data/service/el1/public/asset_service";

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

/// visit root path and execute func
pub fn visit_root_dir(cb: &dyn Fn(&DirEntry) -> Result<()>) -> Result<()> {
    let root_path = Path::new(ROOT_PATH);
    visit_dirs(root_path, cb)
}

fn visit_dirs(dir: &Path, cb: &dyn Fn(&DirEntry) -> Result<()>) -> Result<()> {
    if dir.is_dir() {
        for entry in fs::read_dir(dir)? {
            let entry = entry?;
            let path = entry.path();
            if path.is_dir() {
                visit_dirs(&path, cb)?;
            } else {
                cb(&entry)?;
            }
        }
    }
    Ok(())
}
