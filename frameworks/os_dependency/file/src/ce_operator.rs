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

use crate::common::{get_user_dbs, is_file_exist, DB_KEY};

fn construct_ce_db_dir(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service", user_id)
}

fn construct_db_key_cipher_path(user_id: i32) -> String {
    format!("data/service/el2/{}/asset_service/{}", user_id, DB_KEY)
}

/// Check db key cipher file exists.
pub fn is_db_key_cipher_file_exist(user_id: i32) -> Result<bool> {
    let path_str = construct_db_key_cipher_path(user_id);
    is_file_exist(&path_str)
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

/// Remove all CE file in a specific user space.
pub fn remove_ce_files(user_id: i32) -> Result<()> {
    let path_str = construct_ce_db_dir(user_id);
    for file in fs::read_dir(path_str)? {
        let file = &file?;
        match fs::remove_file(file.path().to_string_lossy().to_string()) {
            Ok(_) => (),
            Err(e) => {
                return log_throw_error!(
                    ErrCode::FileOperationError,
                    "[FATAL]Remove [{}] failed, error code:[{}]",
                    file.path().to_string_lossy().to_string(),
                    e
                )
            },
        }
    }
    Ok(())
}

/// Obtain ce user dbs
pub fn get_ce_user_dbs(user_id: i32) -> Result<Vec<String>> {
    get_user_dbs(&construct_ce_db_dir(user_id))
}
