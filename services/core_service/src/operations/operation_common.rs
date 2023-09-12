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

use asset_common::{definition::{AssetMap, Result, Value, ErrCode, Tag}, logi, loge};
use db_operator::{
    types::{Pair, DataValue},
    database_table_helper::{G_COLUMN_ACCESSTYPE, G_COLUMN_OWNERTYPE, G_COLUMN_DELETETYPE,
        G_COLUMN_VERSION, G_COLUMN_SECRET, G_COLUMN_CREATETIME, G_COLUMN_UPDATETIME}
};

use hilog_rust::hilog;
use std::ffi::{c_char, CString};

use crate::calling_process_info::CallingInfo;

pub(crate) trait FromValueToDataValue {
    /// xxx
    fn to_data_value(&self) -> Result<DataValue>;
}

impl FromValueToDataValue for Value {
    fn to_data_value(&self) -> Result<DataValue> {
        match self {
            Value::NUMBER(n) => {
                Ok(DataValue::Integer(*n)) // to do 类型确认
            },
            Value::Bytes(v) => {
                Ok(DataValue::Blob(v))
            },
            _ => {
                Err(ErrCode::InvalidArgument)
            }
        }
    }
}

/// xxx
pub(crate) fn get_set_attr<'a>(input: &'a AssetMap, column_name: &'a str, tag: Tag, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    if let Some(v) = input.get(&tag) {
        vec.push(
            Pair {
                column_name,
                value: v.to_data_value()?,
            }
        );
        logi!("get {} {} successfully", @public(column_name), @public(tag as u32));
        return Ok(());
    }
    loge!("{} missed", @public(tag as u32));
    Err(ErrCode::InvalidArgument)
}

/// xxx
pub(crate) fn get_set_owner_type(calling_info: &CallingInfo, vec: &mut Vec<Pair>) -> Result<()>  {
    vec.push(
        Pair {
            column_name: G_COLUMN_OWNERTYPE,
            value: DataValue::Integer(calling_info.get_owner_type()),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_access_type(vec: &mut Vec<Pair>) -> Result<()>  {
    let access_type = 1;
    vec.push(
        Pair {
            column_name: G_COLUMN_ACCESSTYPE,
            value: DataValue::Integer(access_type),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_delete_type(vec: &mut Vec<Pair>) -> Result<()>  {
    let delete_type = 1;
    vec.push(
        Pair {
            column_name: G_COLUMN_DELETETYPE,
            value: DataValue::Integer(delete_type),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_version(vec: &mut Vec<Pair>) -> Result<()>  {
    let version = 1;
    vec.push(
        Pair {
            column_name: G_COLUMN_VERSION,
            value: DataValue::Integer(version),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn set_ciphet_secret<'a>(cipher_secret: &'a [u8], vec: &mut Vec<Pair<'a>>) -> Result<()>  {
    vec.push(
        Pair {
            column_name: G_COLUMN_SECRET,
            value: DataValue::Blob(cipher_secret),
        }
    );
    Ok(())
}

/// xxx
pub(crate) fn get_set_current_time<'a>(create_time: &'a str, vec: &mut Vec<Pair<'a>>) -> Result<()>  {
    vec.push(
        Pair {
            column_name: G_COLUMN_CREATETIME,
            value: DataValue::Text(create_time.as_bytes())
        }
    );

    Ok(())
}

/// xxx
pub(crate) fn get_set_update_time<'a>(update_time: &'a str, vec: &mut Vec<Pair<'a>>) -> Result<()>  {
    vec.push(
        Pair {
            column_name: G_COLUMN_UPDATETIME,
            value: DataValue::Text(update_time.as_bytes())
        }
    );

    Ok(())
}