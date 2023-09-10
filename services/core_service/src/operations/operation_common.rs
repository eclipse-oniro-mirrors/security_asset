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

//! This create implement the asset

use asset_common_lib::{asset_type::{AssetMap, AssetResult, Value, AssetStatusCode, Tag}, asset_log_info, asset_log_error};
use db_operator::types::{Pair, DataValue};

use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

pub(crate) trait FromValueToDataValue {
    /// xxx
    fn to_data_value(&self) -> AssetResult<DataValue>;
}

impl FromValueToDataValue for Value {
    fn to_data_value(&self) -> AssetResult<DataValue> {
        match self {
            Value::NUMBER(n) => {
                Ok(DataValue::Integer(*n as i32)) // to do 类型确认
            },
            Value::Bytes(v) => {
                Ok(DataValue::Blob(v))
            },
            _ => {
                Err(AssetStatusCode::InvalidArgument)
            }
        }
    }
}

/// xxx
pub(crate) fn get_set_attr<'a>(input: &'a AssetMap, column_name: &'a str, tag: Tag, vec: &mut Vec<Pair<'a>>) -> AssetResult<()> {
    if let Some(v) = input.get(&tag) {
        vec.push(
            Pair {
                column_name,
                value: v.to_data_value()?,
            }
        );
        asset_log_info!("get {} {} successfully", @public(column_name), @public(tag as u32));
        return Ok(());
    }
    asset_log_error!("{} missed", @public(tag as u32));
    Err(AssetStatusCode::InvalidArgument)
}

pub(crate) const G_CREATE_TIME: &str = "CreateTime";
pub(crate) const G_OWNER_TYPE: &str = "OwnerType";
pub(crate) const G_ACCESS_TYPE: &str = "AccessType";
pub(crate) const G_DELETE_TYPE: &str = "DeleteType";
pub(crate) const G_VERSION: &str = "Version";
pub(crate) const G_UPDATE_TIME: &str = "UpdateTime";
pub(crate) const G_SECRET: &str = "Secret";
pub(crate) const G_AUTH_TYPE: &str = "AuthType";
pub(crate) const G_SYNC_TYPE: &str = "SyncType";

/// xxx
pub(crate) fn get_set_current_time(vec: &mut Vec<Pair>) -> AssetResult<()>  {
    let current_time = 5555;
    vec.push(
        Pair {
            column_name: G_CREATE_TIME,
            value: DataValue::Integer(current_time),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_owner_type(vec: &mut Vec<Pair>) -> AssetResult<()>  {
    let owner_type = 1;
    vec.push(
        Pair {
            column_name: G_OWNER_TYPE,
            value: DataValue::Integer(owner_type),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_access_type(vec: &mut Vec<Pair>) -> AssetResult<()>  {
    let access_type = 1;
    vec.push(
        Pair {
            column_name: G_ACCESS_TYPE,
            value: DataValue::Integer(access_type),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_delete_type(vec: &mut Vec<Pair>) -> AssetResult<()>  {
    let delete_type = 1;
    vec.push(
        Pair {
            column_name: G_DELETE_TYPE,
            value: DataValue::Integer(delete_type),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_version(vec: &mut Vec<Pair>) -> AssetResult<()>  {
    let version = 1;
    vec.push(
        Pair {
            column_name: G_VERSION,
            value: DataValue::Integer(version),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_update_time(vec: &mut Vec<Pair>) -> AssetResult<()>  {
    let update_time = 1;
    vec.push(
        Pair {
            column_name: G_UPDATE_TIME,
            value: DataValue::Integer(update_time),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn set_ciphet_secret<'a>(cipher_secret: &'a [u8], vec: &mut Vec<Pair<'a>>) -> AssetResult<()>  {
    vec.push(
        Pair {
            column_name: G_SECRET,
            value: DataValue::Blob(cipher_secret),
        }
    );
    Ok(())
}