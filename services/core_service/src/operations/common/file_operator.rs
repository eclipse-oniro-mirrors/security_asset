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

use asset_common::{
    definition::{ErrCode, Result},
    loge,
};

const ROOT_PATH: &str = "data/service/el1/public/asset_service";

pub(crate) fn create_user_db_dir(user_id: i32) -> Result<()> {
    let path = format!("{}/{}", ROOT_PATH, user_id);
    let path = Path::new(&path);
    if !path.exists() {
        match fs::create_dir(path) {
            Err(e) if e.kind() != std::io::ErrorKind::AlreadyExists => {
                loge!("[FATAL]Create dir failed! error is [{}]", e);
                return Err(ErrCode::FileOperationError);
            },
            Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => {
                return Ok(());
            },
            _ => return Ok(()),
        }
    }
    Ok(())
}
