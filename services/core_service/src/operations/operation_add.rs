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

use asset_common::{definition::{AssetMap, Result, Tag, ErrCode, Value}, logi, loge};
use db_operator::{database_table_helper::DefaultDatabaseHelper, types::Pair,
    database_table_helper::{G_COLUMN_SYNCTYPE, G_COLUMN_AUTHTYPE}};

// use crypto_manager::hukkey::Crypto;
use crate::{operations::operation_common::*, calling_process_info::CallingInfo};

use std::time::{SystemTime, UNIX_EPOCH};

fn encrypt_secret(input: &AssetMap) -> Result<Vec<u8>> {
    if let Some(Value::Bytes(secret)) = input.get(&Tag::Secret) {
        // Crypto::encrypt(secret)
        Ok(secret.clone()) // to do 使用加解密适配层的接口进行加密
    } else {
        loge!("get secret from input failed!");
        Err(ErrCode::InvalidArgument)
    }
}

fn construct_data<'a>(input: &'a AssetMap, calling_info: &'a CallingInfo) -> Result<Vec<Pair<'a>>> {
    let mut data_vec = Vec::new();

    get_set_attr(input, G_COLUMN_SYNCTYPE, Tag::SyncType, &mut data_vec)?;
    get_set_attr(input, G_COLUMN_AUTHTYPE, Tag::AuthType, &mut data_vec)?;

    get_set_owner_type(calling_info, &mut data_vec)?;

    get_set_delete_type(&mut data_vec)?;
    get_set_access_type(&mut data_vec)?;
    get_set_version(&mut data_vec)?;

    Ok(data_vec)
}

pub(crate) fn add(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    // arrange the table value
    let mut db_data = construct_data(input, calling_info)?;

    let cipher_secret = encrypt_secret(input)?;
    set_ciphet_secret(&cipher_secret, &mut db_data)?;

    let sys_time_res = SystemTime::now().duration_since(UNIX_EPOCH);
    if sys_time_res.is_err() {
        loge!("get sys_time_res faield!");
        return Err(ErrCode::Failed);
    }
    let time_string = sys_time_res.unwrap().as_millis().to_string();
    get_set_current_time(&time_string, &mut db_data)?;
    get_set_update_time(&time_string, &mut db_data)?;

    // to do 创建用户目录

    let owner_str = String::from_utf8(calling_info.get_owner_text().clone());
    if owner_str.is_err() {
        loge!("get owner str faield!");
        return Err(ErrCode::Failed);
    }

    let alias;
    if let Some(Value::Bytes(alias_vec)) = input.get(&Tag::Alias) {
        let alias_try = String::from_utf8(alias_vec.clone());
        if let Ok(alias_ok) = alias_try {
            alias = alias_ok;
        } else {
            loge!("parse alias from utf8 faield!");
            return Err(ErrCode::InvalidArgument);
        }
    } else {
        loge!("get alias faield!");
        return Err(ErrCode::InvalidArgument);
    }

    // call sql to add
    let insert_num =
        DefaultDatabaseHelper::insert_datas_default_once(calling_info.get_user_id(), &owner_str.unwrap(), &alias, db_data)?;

    logi!("insert {} data", insert_num);
    Ok(())
}
