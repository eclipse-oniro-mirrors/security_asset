/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

use asset_definition::{ErrCode, Result};
use asset_log::loge;

const ROOT_PATH: &str = "data/service/el1/public/asset_service";

/// Create user database directory.
pub fn create_user_db_dir(user_id: i32) -> Result<()> {
    let path = format!("{}/{}", ROOT_PATH, user_id);
    let path = Path::new(&path);
    if path.exists() {
        return Ok(());
    }

    match fs::create_dir(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => Ok(()),
        Err(e) => {
            loge!("[FATAL][SA]Create dir failed! error is [{}]", e);
            Err(ErrCode::FileOperationError)
        },
    }
}

/// Delete user databse directory.
pub fn delete_user_db_dir(user_id: i32) -> Result<()> {
    let path_str = format!("{}/{}", ROOT_PATH, user_id);
    let path = Path::new(&path_str);
    if !path.exists() {
        return Ok(());
    }

    match fs::remove_dir_all(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() != std::io::ErrorKind::NotFound => Ok(()),
        Err(e) => {
            loge!("[FATAL][SA]Delete dir failed! error is [{}]", e);
            Err(ErrCode::FileOperationError)
        },
    }
}
