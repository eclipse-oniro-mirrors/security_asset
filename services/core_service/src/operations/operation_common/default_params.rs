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

use asset_common::{
    definition::{Result, Tag, ReturnType, SyncType, Accessibility, AuthType, AssetMap, Insert, ConflictResolution},
    logi,
};
use asset_ipc_interface::IpcCode;

fn check_or_default_sync_type(map: &mut AssetMap) -> Result<()> {
    if !map.contains_key(&Tag::SyncType) {
        logi!("add default sync type");
        map.insert_attr(Tag::SyncType, SyncType::Never)?;
    }
    Ok(())
}

fn check_or_default_access_type(map: &mut AssetMap) -> Result<()> {
    if !map.contains_key(&Tag::Accessibility) {
        logi!("add default access type");
        map.insert_attr(Tag::Accessibility, Accessibility::DeviceFirstUnlock)?;
    }
    Ok(())
}

fn check_or_default_auth_type(map: &mut AssetMap) -> Result<()> {
    if !map.contains_key(&Tag::AuthType) {
        logi!("add default auth type");
        map.insert_attr(Tag::AuthType, AuthType::None)?;
    }
    Ok(())
}

fn check_or_default_return_type(map: &mut AssetMap) -> Result<()> {
    if !map.contains_key(&Tag::ReturnType) {
        logi!("add default return type");
        map.insert_attr(Tag::ReturnType, ReturnType::Attributes)?;
    }
    Ok(())
}

fn check_or_default_required_pwd_set(map: &mut AssetMap) -> Result<()> {
    if !map.contains_key(&Tag::RequirePasswordSet) {
        logi!("add default required password set");
        map.insert_attr(Tag::RequirePasswordSet, false)?;
    }
    Ok(())
}

fn check_or_default_conflict_resolution(map: &mut AssetMap) -> Result<()> {
    if !map.contains_key(&Tag::ConflictResolution) {
        logi!("add default conflict resolution set");
        map.insert_attr(Tag::ConflictResolution, ConflictResolution::ThrowError)?;
    }
    Ok(())
}

fn construct_add(input: &AssetMap) -> Result<AssetMap> {
    let mut map = (*input).clone();
    check_or_default_sync_type(&mut map)?;
    check_or_default_access_type(&mut map)?;
    check_or_default_auth_type(&mut map)?;
    check_or_default_required_pwd_set(&mut map)?;
    check_or_default_conflict_resolution(&mut map)?;

    Ok(map)
}

fn construct_query(input: &AssetMap) -> Result<AssetMap> {
    let mut map = (*input).clone();
    check_or_default_return_type(&mut map)?;
    Ok(map)
}

fn construct_pre_query(input: &AssetMap) -> Result<AssetMap> {
    let mut map = (*input).clone();
    check_or_default_return_type(&mut map)?;
    Ok(map)
}

pub(crate) fn construct_params_with_default(input: &AssetMap, code: &IpcCode) -> Result<AssetMap> {
    match *code {
        IpcCode::Add => {
            construct_add(input)
        },
        IpcCode::Query => {
            construct_query(input)
        },
        IpcCode::PreQuery => {
            construct_pre_query(input)
        },
        _ => todo!()
    }
}