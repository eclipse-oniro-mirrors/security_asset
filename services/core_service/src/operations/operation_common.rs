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

//! This crate implements the asset

mod crypto_adapter;
pub(crate) mod db_adapter;
// mod default_params;
// mod extra_params;
mod file_operator;

pub(crate) use crypto_adapter::{encrypt, decrypt, init_decrypt};
pub(crate) use file_operator::create_user_db_dir;

use std::time::{SystemTime, UNIX_EPOCH};

use asset_common::{definition::{Result, ErrCode, Value}, loge};
use db_operator::{types::DbMap, database_table_helper::{COLUMN_OWNER, COLUMN_OWNER_TYPE}};

use crate::calling_info::CallingInfo;

pub(crate) fn get_system_time() -> Result<Vec<u8>> {
    let sys_time = SystemTime::now().duration_since(UNIX_EPOCH).map_err(|e| {
        loge!("[FATAL]Get system time faield [{}].", e);
        ErrCode::SystemTimeError
    })?;
    Ok(sys_time.as_millis().to_string().as_bytes().to_vec())
}

pub(crate) fn add_owner_info(calling_info: &CallingInfo, db_data: &mut DbMap) {
    db_data.insert(COLUMN_OWNER, Value::Bytes(calling_info.owner_info().clone()));
    db_data.insert(COLUMN_OWNER_TYPE, Value::Number(calling_info.owner_type()));
}