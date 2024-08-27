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
use std::{fs, path::Path};

use crate::common::get_user_dbs;

fn construct_ce_db_dir(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service", user_id)
}

fn construct_ce_db_path(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service/enc_asset.db", user_id)
}

/// Check ce db file exists.
pub fn is_ce_db_file_exist(user_id: i32) -> Result<()> {
    let path_str = construct_ce_db_path(user_id);
    let path: &Path = Path::new(&path_str);
    match path.try_exists() {
        Ok(true) => Ok(()),
        Ok(false) => {
            log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]CE database file does not exist!")
        },
        Err(e) => {
            log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][SA]Checking existence of CE database file failed! error is [{}]",
                e
            )
        },
    }
}

fn construct_db_key_cipher_path(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service/db_key", user_id)
}

/// Check db key cipher file exists.
pub fn is_db_key_cipher_file_exist(user_id: i32) -> Result<bool> {
    let path_str = construct_db_key_cipher_path(user_id);
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

/// Read db key cipher.
pub fn read_db_key_cipher(user_id: i32) -> Result<Vec<u8>> {
    let path_str = construct_db_key_cipher_path(user_id);
    let path: &Path = Path::new(&path_str);
    match fs::read(path) {
        Ok(db_key_cipher) => Ok(db_key_cipher),
        Err(e) => {
            log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][SA]Read database key ciphertext failed! error is [{}]",
                e
            )
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
            log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][SA]Write database key ciphertext failed! error is [{}]",
                e
            )
        },
    }
}

pub fn get_ce_user_dbs(user_id: i32) -> Result<Vec<String>> {
    get_user_dbs(user_id, &construct_ce_db_dir(user_id))
}
