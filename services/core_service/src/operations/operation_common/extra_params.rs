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

//! This crate implements the obtain of extra params

use std::time::{SystemTime, UNIX_EPOCH};

use asset_common::{definition::{Result, ErrCode, Value}, loge};
use db_operator::{database_table_helper::{G_COLUMN_OWNER_TYPE, G_COLUMN_DELETE_TYPE,
        G_COLUMN_VERSION, G_COLUMN_CREATE_TIME, G_COLUMN_UPDATE_TIME, G_COLUMN_OWNER}, types::Pair};

use crate::{calling_info::CallingInfo, definition_inner::{DeleteType, OperationCode}};

fn get_owner_type(calling_info: &CallingInfo, db_data: &mut Vec<Pair<>>) -> Result<()> {
    db_data.push(
        Pair {
            column_name: G_COLUMN_OWNER_TYPE,
            value: Value::Number(calling_info.owner_type()),
        }
    );
    Ok(())
}

fn get_delete_type(db_data: &mut Vec<Pair<>>) -> Result<()> {
    db_data.push(
        Pair {
            column_name: G_COLUMN_DELETE_TYPE,
            value: Value::Number(DeleteType::WhenUninstallApp as u32 | DeleteType::WhenRemoveUser as u32),
        }
    );
    Ok(())
}

fn get_version(db_data: &mut Vec<Pair<>>) -> Result<()> {
    db_data.push(
        Pair {
            column_name: G_COLUMN_VERSION,
            value: Value::Number(1), // todo zwz get real
        }
    );
    Ok(())
}

fn get_update_time(db_data: &mut Vec<Pair<>>) -> Result<()> {
    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH).map_err(|e| {
        loge!("Get sys_time_res faield [{}].", e);
        ErrCode::SystemTimeError
    })?;
    let time_string = sys_time_res.as_millis().to_string().as_bytes().to_vec();

    db_data.push(
        Pair {
            column_name: G_COLUMN_UPDATE_TIME,
            value: Value::Bytes(time_string),
        }
    );
    Ok(())
}

fn get_create_time(db_data: &mut Vec<Pair<>>) -> Result<()> {
    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH).map_err(|e| {
        loge!("Get sys_time_res faield [{}].", e);
        ErrCode::SystemTimeError
    })?;
    let time_string = sys_time_res.as_millis().to_string().as_bytes().to_vec();

    db_data.push(
        Pair {
            column_name: G_COLUMN_CREATE_TIME,
            value: Value::Bytes(time_string),
        }
    );
    Ok(())
}

fn get_owner(calling_info: &CallingInfo, db_data: &mut Vec<Pair<>>) -> Result<()> {
    db_data.push(
        Pair {
            column_name: G_COLUMN_OWNER,
            value: Value::Bytes(calling_info.owner_info().clone()),
        }
    );
    Ok(())
}

pub(crate) fn add_extra_db_data(calling_info: &CallingInfo, code: &OperationCode, db_data: &mut Vec<Pair<>>) -> Result<()> {
    get_owner_type(calling_info, db_data)?;
    get_owner(calling_info, db_data)?;
    match *code {
        OperationCode::Add => {
            get_update_time(db_data)?;
            get_create_time(db_data)?;
            get_delete_type(db_data)?;
            get_version(db_data)?;
        },
        OperationCode::Update => get_update_time(db_data)?,
        _ => {},
    }
    Ok(())
}