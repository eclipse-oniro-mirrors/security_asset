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

use std::{fs, path::Path};

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_log::logi;

fn construct_user_de_path(user_id: i32) -> String {
    format!("data/service/el1/public/asset_service/{}", user_id)
}

fn is_user_de_dir_exist(user_id: i32) -> Result<()> {
    let path_str = construct_user_de_path(user_id);
    let path: &Path = Path::new(&path_str);
    match path.try_exists() {
        Ok(_) => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]User DE directory does not exist! error is [{}]", e)
        },
    }
}

/// Create user de directory.
pub fn create_user_de_dir(user_id: i32) -> Result<()> {
    if is_user_de_dir_exist(user_id).is_ok() {
        return Ok(());
    }

    logi!("[INFO]User DE directory does not exist, create it...");
    let path_str = construct_user_de_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::create_dir(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Create user DE directory failed! error is [{}]", e)
        },
    }
}

/// Delete user de directory.
pub fn delete_user_de_dir(user_id: i32) -> Result<()> {
    if is_user_de_dir_exist(user_id).is_err() {
        return Ok(());
    }

    let path_str = construct_user_de_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::remove_dir_all(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() != std::io::ErrorKind::NotFound => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Delete user DE directory failed! error is [{}]", e)
        },
    }
}

fn construct_ce_db_path(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service/asset.db", user_id)
}

/// Check ce db file exists.
pub fn is_ce_db_file_exist(user_id: i32) -> Result<()> {
    let path_str = construct_ce_db_path(user_id);
    let path: &Path = Path::new(&path_str);
    match path.try_exists() {
        Ok(_) => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]CE database file does not exist! error is [{}]", e)
        },
    }
}

fn construct_db_key_cipher_path(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service/db_key", user_id)
}

/// Check db key cipher file exists.
pub fn is_db_key_cipher_file_exist(user_id: i32) -> Result<()> {
    let path_str = construct_db_key_cipher_path(user_id);
    let path: &Path = Path::new(&path_str);
    match path.try_exists() {
        Ok(_) => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Database key ciphertext file does not exist! error is [{}]", e)
        },
    }
}

/// Read db key cipher.
pub fn read_db_key_cipher(user_id: i32) -> Result<Vec<u8>> {
    let path_str = construct_db_key_cipher_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::read(path) {
        Ok(db_key_cipher) => Ok(db_key_cipher),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Read database key ciphertext failed! error is [{}]", e)
        },
    }
}

/// Write db key cipher. If path does not exist, create it automatically.
pub fn write_db_key_cipher(user_id: i32, db_key_cipher: &Vec<u8>) -> Result<()> {
    let path_str = construct_db_key_cipher_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::write(path, db_key_cipher) {
        Ok(_) => Ok(()),
        Err(e) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]Write database key ciphertext failed! error is [{}]", e)
        },
    }
}