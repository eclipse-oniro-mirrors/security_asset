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
    definition::{AssetMap, Result, ErrCode, Tag, Value},
    logi, loge,
};
use asset_ipc_interface::IpcCode;
use db_operator::{database_table_helper::G_COLUMN_SECRET, types::{DataValue, Pair}};

// use crypto_manager::hukkey::Crypto;
use crate::{
    operations::operation_common::{
        get_alias, construst_extra_params, encrypt,
        db_adapter::{set_extra_attrs, set_input_attr, data_exist_once, update_data_once}
    },
    calling_process_info::CallingInfo,
    definition_inner::AssetInnerMap
};

fn construct_data<'a>(input: &'a AssetMap, inner_params: &'a AssetInnerMap)
    -> Result<Vec<Pair<'a>>> {
    let mut data_vec = Vec::new();
    set_input_attr(input, &mut data_vec)?;
    set_extra_attrs(inner_params, &mut data_vec)?;
    Ok(data_vec)
}

fn encrypt_update(input: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<u8>> {
    let auth_type = match input.get(&Tag::AuthType) {
        Some(Value::Number(res)) => res,
        _ => todo!(),
    };
    let access_type = match input.get(&Tag::Accessibility) {
        Some(Value::Number(res)) => res,
        _ => todo!(),
    };
    encrypt(calling_info, auth_type, access_type, input)
}

pub(crate) fn update(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    let alias = get_alias(input)?;

    if !data_exist_once(&alias, calling_info)? {
        loge!("asset not exist!");
        return Err(ErrCode::NotFound);
    }

    // a map collecting inner params
    let inner_params = construst_extra_params(calling_info, &IpcCode::Update)?;

    // construct db data from input map
    let mut db_data = construct_data(input, &inner_params)?;

    let cipher;
    // whether to update secret
    if input.contains_key(&Tag::Secret) {
        // todo 获取存储的数据
        cipher = encrypt_update(input, calling_info)?;
        logi!("get cipher len is [{}]", cipher.len()); // todo delete
        db_data.push(
            Pair {
                column_name: G_COLUMN_SECRET,
                value: DataValue::Blob(&cipher),
            }
        );
    }

    // call sql to update
    let update_num = update_data_once(&alias, calling_info, &db_data)?;

    logi!("update {} data", update_num);
    Ok(())
}
