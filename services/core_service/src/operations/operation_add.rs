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

use core::panic;

use asset_common::{
    definition::{AssetMap, Result, ConflictResolution, ErrCode, Tag, Value},
    logi, loge,
};
use asset_ipc_interface::IpcCode;
use db_operator::{database_table_helper::{G_COLUMN_SECRET, G_COLUMN_ALIAS, G_COLUMN_OWNER, G_COLUMN_OWNER_TYPE}, types::{DataValue, Pair}};

use crate::{
    operations::operation_common::{
        create_user_db_dir, construct_params_with_default, encrypt,
        db_adapter::{insert_data_once, data_exist_once, replace_data_once, construct_db_data}
    },
    definition_inner::OperationCode,
    calling_info::CallingInfo
};

fn check_resolve_conflict(input: &AssetMap, calling_info: &CallingInfo, db_data: &Vec<Pair<>>)
    -> Result<()> {
    let Some(Value::Bytes(alias)) = input.get(&Tag::Alias) else {
        panic!()
    };
    let query_db_data = vec![
        Pair {
            column_name: G_COLUMN_ALIAS,
            value: DataValue::Blob(alias.clone())
        },
        Pair {
            column_name: G_COLUMN_OWNER,
            value: DataValue::Blob(calling_info.owner_info().clone())
        },
        Pair {
            column_name: G_COLUMN_OWNER_TYPE,
            value: DataValue::Integer(calling_info.owner_type())
        }
    ];

    if data_exist_once(calling_info, &query_db_data)? {
        match input.get(&Tag::ConflictResolution) {
            Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 =>
                return replace_data_once(calling_info, db_data),
            _ => {
                loge!("[FATAL]The specified alias already exists.");
                return Err(ErrCode::Duplicated);
            },
        }
    }
    Ok(())
}

pub(crate) fn add(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    // create user dir
    create_user_db_dir(calling_info.user_id())?;
    // get param map contains input params and default params
    let input_new = construct_params_with_default(input, &IpcCode::Add)?;

    // construct db data from input map and inner params
    let mut db_data = construct_db_data(&input_new, calling_info, &OperationCode::Add)?;

    let Value::Bytes(secret) = input_new.get(&Tag::Secret).unwrap() else { panic!("Impossible error for secret type.") };

    let cipher = encrypt(calling_info, &input_new, secret)?;
    logi!("get cipher len is [{}]", cipher.len()); // todo delete
    db_data.push(
        Pair {
            column_name: G_COLUMN_SECRET,
            value: DataValue::Blob(cipher),
        }
    );

    check_resolve_conflict(&input_new, calling_info, &db_data)?;

    // call sql to add
    let insert_num = insert_data_once(calling_info, db_data)?;

    logi!("insert {} data", insert_num);
    Ok(())
}
