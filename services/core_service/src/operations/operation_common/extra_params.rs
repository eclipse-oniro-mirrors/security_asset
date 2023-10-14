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

use asset_common::{definition::{Result, ErrCode}, loge};
use asset_ipc_interface::IpcCode;
use db_operator::database_table_helper::{G_COLUMN_OWNER_TYPE, G_COLUMN_DELETE_TYPE,
        G_COLUMN_VERSION, G_COLUMN_CREATE_TIME, G_COLUMN_UPDATE_TIME, G_COLUMN_OWNER};

use crate::{calling_info::CallingInfo, definition_inner::{AssetInnerMap, DeleteType, InnerValue}};

fn get_owner_type(calling_info: &CallingInfo, params: &mut AssetInnerMap) -> Result<()> {
    params.insert(G_COLUMN_OWNER_TYPE, InnerValue::Number(calling_info.owner_type()));
    Ok(())
}

fn get_delete_type(params: &mut AssetInnerMap) -> Result<()> {
    params.insert(G_COLUMN_DELETE_TYPE,
        InnerValue::Number(DeleteType::WhenUninstallApp as u32 | DeleteType::WhenRemoveUser as u32));
    Ok(())
}

fn get_version(params: &mut AssetInnerMap) -> Result<()> {
    params.insert(G_COLUMN_VERSION, InnerValue::Number(1)); // todo zwz get real
    Ok(())
}

fn get_update_time(params: &mut AssetInnerMap) -> Result<()> {
    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH).map_err(|e| {
        loge!("Get sys_time_res faield [{}].", e);
        ErrCode::SystemTimeError
    })?;
    let time_string = sys_time_res.as_millis().to_string();
    params.insert(G_COLUMN_UPDATE_TIME, InnerValue::Blob(time_string.into_bytes()));
    Ok(())
}

fn get_create_time(params: &mut AssetInnerMap) -> Result<()> {
    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH).map_err(|e| {
        loge!("Get sys_time_res faield [{}].", e);
        ErrCode::SystemTimeError
    })?;
    let time_string = sys_time_res.as_millis().to_string();
    params.insert(G_COLUMN_CREATE_TIME, InnerValue::Blob(time_string.into_bytes()));
    Ok(())
}

fn get_owner(calling_info: &CallingInfo, params: &mut AssetInnerMap) -> Result<()> {
    params.insert(G_COLUMN_OWNER, InnerValue::Blob(calling_info.owner_info().clone()));
    Ok(())
}

pub(crate) fn construst_extra_params(calling_info: &CallingInfo, code: &IpcCode) -> Result<AssetInnerMap> {
    let mut params = AssetInnerMap::new();
    get_owner_type(calling_info, &mut params)?;
    get_owner(calling_info, &mut params)?;
    match *code {
        IpcCode::Add => {
            get_update_time(&mut params)?;
            get_create_time(&mut params)?;
            get_delete_type(&mut params)?;
            get_version(&mut params)?;
        },
        IpcCode::Update => get_update_time(&mut params)?,
        _ => {},
    }
    Ok(params)
}
