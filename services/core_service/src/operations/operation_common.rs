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

use std::time::{SystemTime, UNIX_EPOCH};

use asset_common::{definition::{AssetMap, Result, Value, ErrCode, Tag, Insert, SyncType, Accessibility, AuthType},
    loge, logi};
use asset_ipc_interface::IpcCode;
use db_operator::{
    types::{Pair, DataValue},
    database_table_helper::{G_COLUMN_ACCESSTYPE, G_COLUMN_OWNERTYPE, G_COLUMN_DELETETYPE,
        G_COLUMN_VERSION, G_COLUMN_SECRET, G_COLUMN_CREATETIME, G_COLUMN_UPDATETIME, G_COLUMN_ALIAS, G_COLUMN_AUTHTYPE,
        G_COLUMN_SYNCTYPE, G_COLUMN_CRITICAL1, G_COLUMN_CRITICAL2, G_COLUMN_CRITICAL3,
        G_COLUMN_CRITICAL4, G_COLUMN_NORMAL1, G_COLUMN_NORMAL2, G_COLUMN_NORMAL3, G_COLUMN_NORMAL4}
};

use crate::{calling_process_info::CallingInfo, definition_inner::{AssetInnerMap, DeleteType, InnerValue}};

pub(crate) fn convert_value_into_db_value(value: &Value) -> Result<DataValue>
{
    match value {
        Value::Number(n) => {
            Ok(DataValue::Integer(*n)) // to do 类型确认
        },
        Value::Bytes(v) => {
            Ok(DataValue::Blob(v))
        },
    }
}

pub(crate) fn convert_inner_value_into_db_value(value: &InnerValue) -> Result<DataValue>
{
    match value {
        InnerValue::Number(n) => {
            Ok(DataValue::Integer(*n)) // to do 类型确认
        },
        InnerValue::Blob(v) => {
            Ok(DataValue::Blob(v))
        },
        InnerValue::Text(v) => {
            Ok(DataValue::Text(v))
        },
    }
}

fn get_owner_type(calling_info: &CallingInfo, params: &mut AssetInnerMap) -> Result<()>
{
    params.insert(G_COLUMN_OWNERTYPE, InnerValue::Number(calling_info.get_owner_type()));
    Ok(())
}

fn get_delete_type(params: &mut AssetInnerMap) -> Result<()>
{
    params.insert(G_COLUMN_DELETETYPE,
        InnerValue::Number(DeleteType::WhenUninstallApp as u32 | DeleteType::WhenRemoveUser as u32));
    Ok(())
}

fn get_version(params: &mut AssetInnerMap) -> Result<()>
{
    params.insert(G_COLUMN_VERSION, InnerValue::Number(1)); // todo get real
    Ok(())
}

fn get_update_time(params: &mut AssetInnerMap) -> Result<()>
{
    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH);
    if sys_time_res.is_err() {
        loge!("get sys_time_res faield!");
        return Err(ErrCode::Failed);
    }
    let time_string = sys_time_res.unwrap().as_millis().to_string();
    params.insert(G_COLUMN_UPDATETIME, InnerValue::Text(time_string.into_bytes()));
    Ok(())
}

fn get_create_time(params: &mut AssetInnerMap) -> Result<()>
{
    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH);
    if sys_time_res.is_err() {
        loge!("get sys_time_res faield!");
        return Err(ErrCode::Failed);
    }
    let time_string = sys_time_res.unwrap().as_millis().to_string();
    params.insert(G_COLUMN_CREATETIME, InnerValue::Text(time_string.into_bytes()));
    Ok(())
}

pub(crate) fn construst_inner_params(calling_info: &CallingInfo, code: IpcCode) -> Result<AssetInnerMap>
{
    let mut params = AssetInnerMap::new();
    get_owner_type(calling_info, &mut params)?;
    get_delete_type(&mut params)?;
    get_version(&mut params)?;
    match code {
        IpcCode::Add => {
            get_update_time(&mut params)?;
            get_create_time(&mut params)?;
        },
        IpcCode::Update => {
            get_update_time(&mut params)?;
        },
        _ => {},
    }
    Ok(params)
}

fn check_or_default_sync_type(map: &mut AssetMap) -> Result<()>
{
    if !map.contains_key(&Tag::SyncType) {
        logi!("add default sync type");
        map.insert_attr(Tag::SyncType, SyncType::Never)?;
    }
    Ok(())
}

fn check_or_default_access_type(map: &mut AssetMap) -> Result<()>
{
    if !map.contains_key(&Tag::Accessibility) {
        logi!("add default access type");
        map.insert_attr(Tag::Accessibility, Accessibility::DevoceFirstUnlock)?;
    }
    Ok(())
}

fn check_or_default_auth_type(map: &mut AssetMap) -> Result<()>
{
    if !map.contains_key(&Tag::AuthType) {
        logi!("add default auth type");
        map.insert_attr(Tag::AuthType, AuthType::None)?;
    }
    Ok(())
}

pub(crate) fn construct_params_with_default(input: &AssetMap) -> Result<AssetMap>
{
    let mut map = (*input).clone();
    check_or_default_sync_type(&mut map)?;
    check_or_default_access_type(&mut map)?;
    check_or_default_auth_type(&mut map)?;
    Ok(map)
}

pub(crate) fn get_alias(input: &AssetMap) -> Result<String>
{
    let alias;
    if let Some(Value::Bytes(alias_vec)) = input.get(&Tag::Alias) {
        let alias_try = String::from_utf8(alias_vec.clone());
        if let Ok(alias_ok) = alias_try {
            alias = alias_ok;
        } else {
            loge!("parse alias from utf8 failed!");
            return Err(ErrCode::InvalidArgument);
        }
    } else {
        loge!("get alias failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(alias)
}

fn get_tag_column_name(tag: &Tag) -> Option<&str>
{
    match *tag {
        Tag::Accessibility => Some(G_COLUMN_ACCESSTYPE),
        Tag::Secret => Some(G_COLUMN_SECRET),
        Tag::Alias => Some(G_COLUMN_ALIAS),
        Tag::AuthType => Some(G_COLUMN_AUTHTYPE),
        Tag::SyncType => Some(G_COLUMN_SYNCTYPE),
        Tag::DataLabelCritical1 => Some(G_COLUMN_CRITICAL1),
        Tag::DataLabelCritical2 => Some(G_COLUMN_CRITICAL2),
        Tag::DataLabelCritical3 => Some(G_COLUMN_CRITICAL3),
        Tag::DataLabelCritical4 => Some(G_COLUMN_CRITICAL4),
        Tag::DataLabelNormal1 => Some(G_COLUMN_NORMAL1),
        Tag::DataLabelNormal2 => Some(G_COLUMN_NORMAL2),
        Tag::DataLabelNormal3 => Some(G_COLUMN_NORMAL3),
        Tag::DataLabelNormal4 => Some(G_COLUMN_NORMAL4),
        _ => None,
    }
}

pub(crate) fn set_input_attr<'a>(input: &'a AssetMap, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    for (tag, value) in input.iter() {
        if let Some(column) = get_tag_column_name(tag) {
            vec.push(
                Pair {
                    column_name: column,
                    value: convert_value_into_db_value(value)?,
                }
            );
        }
    }
    Ok(())
}

/// xxx
pub(crate) fn set_inner_attrs<'a>(input: &'a AssetInnerMap, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    for (tag, value) in input.iter() {
        vec.push(
            Pair {
                column_name: tag,
                value: convert_inner_value_into_db_value(value)?,
            }
        );
    }
    Ok(())
}