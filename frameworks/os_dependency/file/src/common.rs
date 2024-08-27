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

use asset_definition::Result;
use std::fs;

const BACKUP_SUFFIX: &str = ".backup";
const DB_SUFFIX: &str = ".db";
pub(crate) const DB_KEY: &str = "db_key";

/// Get all db name in user directory.
pub(crate) fn get_user_dbs(path_str: &str) -> Result<Vec<String>> {
    let mut dbs = vec![];
    for db_path in fs::read_dir(path_str)? {
        let db_path = db_path?;
        let db_file_name = db_path.file_name().to_string_lossy().to_string();
        if !db_file_name.ends_with(BACKUP_SUFFIX) && db_file_name.ends_with(DB_SUFFIX) {
            dbs.push(db_file_name.strip_suffix(DB_SUFFIX).unwrap_or(&db_file_name).to_string())
        }
    }
    Ok(dbs)
}