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
#![allow(dead_code)]

use asset_common_lib::{asset_type::{AssetMap, AssetResult, Tag, AssetStatusCode, Value}, asset_log_info, asset_log_error};
use db_operator::{database_table_helper::DefaultDatabaseHelper, types::Pair};
// use crypto_manager::hukkey::Crypto;
use crate::{operations::operation_common::*, calling_process_info::CallingInfo};

use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

fn encrypt_secret(input: &AssetMap) -> AssetResult<Vec<u8>> {
    if let Some(Value::Bytes(secret)) = input.get(&Tag::Secret) {
        // Crypto::encrypt(secret)
        Ok(secret.clone()) // to do 使用加解密适配层的接口进行加密
    } else {
        asset_log_error!("get secret from input failed!");
        Err(AssetStatusCode::InvalidArgument)
    }
}

fn construct_data(input: &AssetMap) -> AssetResult<Vec<Pair>> {
    let mut data_vec = Vec::new();

    get_set_attr(input, G_SYNC_TYPE, Tag::SyncType, &mut data_vec)?;
    get_set_attr(input, G_AUTH_TYPE, Tag::AuthType, &mut data_vec)?;

    get_set_delete_type(&mut data_vec)?;
    get_set_access_type(&mut data_vec)?;
    get_set_owner_type(&mut data_vec)?;
    get_set_version(&mut data_vec)?;
    get_set_current_time(&mut data_vec)?;
    get_set_update_time(&mut data_vec)?;
    Ok(data_vec)
}

pub(crate) fn add(input: &AssetMap) -> AssetResult<AssetMap> {
    // arrange the table value
    let mut db_data = construct_data(input)?;

    let cipher_secret = encrypt_secret(input)?;
    set_ciphet_secret(&cipher_secret, &mut db_data)?;

    // to do 创建用户目录

    let calling_info = CallingInfo::new()?;
    let owner_str = String::from_utf8(calling_info.get_owner_text().clone());
    if owner_str.is_err() {
        return Err(AssetStatusCode::Failed);
    }
    // call sql to add
    let insert_num =
        DefaultDatabaseHelper::insert_datas_default_once(calling_info.get_user_id(), &owner_str.unwrap(), "Alias1", db_data)?;

    asset_log_info!("inser {} data", @public(insert_num));
    Ok(AssetMap::new())
}
