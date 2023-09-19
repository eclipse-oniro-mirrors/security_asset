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

use crate::{
    calling_process_info::CallingInfo,
    operations::operation_common::{
        construct_params_with_default, get_alias, decrypt,
        db_adapter::{set_input_attr, query_data_once, convert_db_data_into_map},
    },
};

use db_operator::types::Pair;

use asset_common::{definition::{AssetMap, Result, Insert, Value, ErrCode, Tag}, loge};
use asset_ipc_interface::IpcCode;

fn precise_query(alias: &str, calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<Vec<AssetMap>> {
    let query_res = query_data_once(alias, calling_info, db_data)?;

    let mut res_vec = convert_db_data_into_map(&query_res)?;

    for map in &mut res_vec {
        let auth_type = match map.get(&Tag::AuthType) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get auth type failed!");
                return Err(ErrCode::SqliteERROR);
            },
        };
        let access_type = match map.get(&Tag::Accessibility) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get access type failed!");
                return Err(ErrCode::SqliteERROR);
            },
        };
        let secret = match map.get(&Tag::Secret) {
            Some(Value::Bytes(res)) => res,
            _ => {
                loge!("get secret failed!");
                return Err(ErrCode::SqliteERROR);
            },
        };
        map.insert_attr(Tag::Secret, decrypt(calling_info, auth_type, access_type, secret)?)?;
    }

    Ok(res_vec)
}

fn fuzzy_query(_calling_info: &CallingInfo, _db_data: &[Pair]) -> Result<Vec<AssetMap>> {
    let mut db_datas: Vec<AssetMap> = Vec::new();
    // todo 查询数据库，批量查询
    for data in &mut db_datas {
        data.remove(&Tag::Secret);
    }
    Ok(db_datas)
}

pub(crate) fn query(input: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<AssetMap>> {

    // get param map contains input params and default params
    let input_new = construct_params_with_default(input, &IpcCode::Query)?;

    let mut data_vec = Vec::new();
    set_input_attr(&input_new, &mut data_vec)?;
    match get_alias(&input_new) {
        Ok(alias) => {
            precise_query(&alias, calling_info, &data_vec)
        },
        Err(ErrCode::NotFound) => {
            fuzzy_query(calling_info, &data_vec)
        }
        _ => {
            loge!("get alias and not not found failed!");
            Err(ErrCode::SqliteERROR)
        },
    }
}
