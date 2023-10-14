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

use asset_common::{
    definition::{AssetMap, Result, ErrCode, Tag, Value},
    logi, loge,
};

use asset_ipc_interface::IpcCode;

use db_operator::{database_table_helper::{G_COLUMN_SECRET, G_COLUMN_ALIAS}, types::{DataValue, Pair}};

// use crypto_manager::hukkey::Crypto;
use crate::{
    operations::operation_common::{
        encrypt, construct_params_with_default,
        db_adapter::{update_data_once, construct_db_data, query_data_once}
    },
    calling_info::CallingInfo,
    definition_inner::OperationCode,
};

pub(crate) fn update(query: &AssetMap, update: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    // alias is sure to exist for the pre-check
    let Some(Value::Bytes(alias)) = query.get(&Tag::Alias) else {
        panic!()
    };
    let update_new = construct_params_with_default(update, &IpcCode::Update)?;
    let query_new = construct_params_with_default(query, &IpcCode::Query)?;

    let mut update_db_data = construct_db_data(&update_new, calling_info, &OperationCode::Update)?;
    let query_db_data = construct_db_data(&query_new, calling_info, &OperationCode::Query)?;

    let cipher;
    // whether to update secret
    if let Some(Value::Bytes(secret)) = update_new.get(&Tag::Secret) {
        let query_res = query_data_once(calling_info, &query_db_data)?;
        if query_res.len() != 1 {
            loge!("query to-be-updated asset failed, found [{}] assets", query_res.len());
            return Err(ErrCode::NotFound);
        }
        let asset_map = query_res.get(0).unwrap();
        cipher = encrypt(calling_info, asset_map, secret)?;
        logi!("get cipher len is [{}]", cipher.len()); // todo delete
        update_db_data.push(
            Pair {
                column_name: G_COLUMN_SECRET,
                value: DataValue::Blob(cipher),
            }
        );
    }

    update_db_data.push(
        Pair {
            column_name: G_COLUMN_ALIAS,
            value: DataValue::Blob(alias.to_vec()),
        }
    );

    // call sql to update
    let update_num = update_data_once(calling_info, &query_db_data, &update_db_data)?;

    logi!("update {} data", update_num);
    Ok(())
}
