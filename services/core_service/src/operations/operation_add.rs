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

use asset_common::{
    definition::{AssetMap, Result, Tag, ErrCode, Value},
    loge,
    logi,
};
use asset_ipc_interface::IpcCode;
use db_operator::{database_table_helper::DefaultDatabaseHelper, types::Pair};

// use crypto_manager::hukkey::Crypto;
use crate::{
    operations::operation_common::{get_alias, construst_extra_params, set_extra_attrs, set_input_attr,
        create_user_db_dir, construct_params_with_default},
    calling_process_info::CallingInfo,
    definition_inner::AssetInnerMap
};

fn encrypt_secret(input: &AssetMap) -> Result<Vec<u8>> {
    if let Some(Value::Bytes(secret)) = input.get(&Tag::Secret) {
        // Crypto::encrypt(secret)
        Ok(secret.clone()) // to do 使用加解密适配层的接口进行加密
    } else {
        loge!("get secret from input failed!");
        Err(ErrCode::InvalidArgument)
    }
}

fn construct_data<'a>(input: &'a AssetMap, inner_params: &'a AssetInnerMap) -> Result<Vec<Pair<'a>>> {
    let mut data_vec = Vec::new();
    set_input_attr(input, &mut data_vec)?;
    set_extra_attrs(inner_params, &mut data_vec)?;
    Ok(data_vec)
}

pub(crate) fn add(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    // create user dir
    create_user_db_dir(calling_info.get_user_id())?;

    // get param map contains input params and default params
    let input_new = construct_params_with_default(input, &IpcCode::Add)?;

    // a map collecting inner params
    let inner_params = construst_extra_params(calling_info, &IpcCode::Add)?;

    // construct db data from input map and inner params
    let db_data = construct_data(&input_new, &inner_params)?;

    // get owner str
    let owner_str = String::from_utf8(calling_info.get_owner_text().clone()).map_err(|_| {
        loge!("get owner str faield!");
        ErrCode::Failed
    })?;

    let alias = get_alias(&input_new)?;

    // call sql to add
    let insert_num =
        DefaultDatabaseHelper::insert_datas_default_once(calling_info.get_user_id(), &owner_str, &alias, db_data)?;

    logi!("insert {} data", insert_num);
    Ok(())
}
