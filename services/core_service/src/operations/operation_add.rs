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
#![allow(dead_code)]

use asset_common::{
    definition::{AssetMap, Result, ConflictResolution, ErrCode, Tag, Value},
    logi, loge,
};
use asset_ipc_interface::IpcCode;
use db_operator::{database_table_helper::G_COLUMN_SECRET, types::{DataValue, Pair}};

// use crypto_manager::hukkey::Crypto;
use crate::{
    operations::operation_common::{
        get_alias, construst_extra_params, create_user_db_dir,
        construct_params_with_default, encrypt,
        db_adapter::{set_extra_attrs, set_input_attr, insert_data_once, data_exist_once, replace_data_once}
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

pub(crate) fn add(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    // create user dir
    create_user_db_dir(calling_info.user_id())?;

    // get param map contains input params and default params
    let input_new = construct_params_with_default(input, &IpcCode::Add)?;

    let alias = get_alias(&input_new)?;

    // a map collecting inner params
    let inner_params = construst_extra_params(calling_info, &IpcCode::Add)?;

    // construct db data from input map and inner params
    let mut db_data = construct_data(&input_new, &inner_params)?;

    let secret = match input.get(&Tag::Secret) {
        Some(Value::Bytes(res)) => res,
        _ => todo!(),
    };

    let cipher = encrypt(calling_info, &input_new, secret)?;
    logi!("get cipher len is [{}]", cipher.len()); // todo delete
    db_data.push(
        Pair {
            column_name: G_COLUMN_SECRET,
            value: DataValue::Blob(&cipher),
        }
    );

    if data_exist_once(&alias, calling_info)? {
        match input_new.get(&Tag::ConflictResolution) {
            Some(Value::Number(num)) if *num == ConflictResolution::ThrowError as u32 => {
                loge!("alias already exists");
                return Err(ErrCode::Duplicated);
            },
            Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 => {
                return replace_data_once(&alias, calling_info, &db_data);
            }
            _ => {
                loge!("not found ConflictResolution");
                return Err(ErrCode::InvalidArgument);
            },
        }
    }

    // call sql to add
    let insert_num =
        insert_data_once(&alias, calling_info, db_data)?;

    logi!("insert {} data", insert_num);
    Ok(())
}
