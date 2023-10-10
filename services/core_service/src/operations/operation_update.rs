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
    definition_inner::AssetInnerMap,
    operations::operation_query
};

fn construct_data<'a>(input: &'a AssetMap, inner_params: &'a AssetInnerMap)
    -> Result<Vec<Pair<'a>>> {
    let mut data_vec = Vec::new();
    set_input_attr(input, &mut data_vec)?;
    set_extra_attrs(inner_params, &mut data_vec)?;
    Ok(data_vec)
}

pub(crate) fn update(query: &AssetMap, update: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    let alias = get_alias(query)?;

    if !data_exist_once(&alias, calling_info)? {
        loge!("asset not exist!");
        return Err(ErrCode::NotFound);
    }

    // a map collecting inner params
    let inner_params = construst_extra_params(calling_info, &IpcCode::Update)?;

    // construct db data from update map
    let mut db_data = construct_data(update, &inner_params)?;

    let cipher;
    // whether to update secret
    if let Some(Value::Bytes(secret)) = update.get(&Tag::Secret) {
        let query_res = operation_query::query(query, calling_info)?;
        if query_res.len() != 1 {
            loge!("query to-be-updated asset failed, found [{}] assets", query_res.len());
            return Err(ErrCode::NotFound);
        }
        let asset_map = query_res.get(0).unwrap();
        cipher = encrypt(calling_info, asset_map, secret)?;
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
