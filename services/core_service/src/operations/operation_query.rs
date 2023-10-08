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

use std::collections::HashSet;

use crate::{
    calling_process_info::CallingInfo,
    operations::operation_common::{
        construct_params_with_default, get_alias, decrypt, init_decrypt,
        db_adapter::{set_input_attr, query_data_once},
    },
};

use db_operator::types::Pair;

use asset_common::{definition::{AssetMap, Result, Insert, Value, ErrCode, Tag}, loge, logi};
use asset_ipc_interface::IpcCode;

fn precise_query(alias: &str, calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<Vec<AssetMap>> {
    let mut query_res = query_data_once(alias, calling_info, db_data)?;

    for map in &mut query_res {
        let auth_type = match map.get(&Tag::AuthType) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get auth type failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        let access_type = match map.get(&Tag::Accessibility) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get access type failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        let secret = match map.get(&Tag::Secret) {
            Some(Value::Bytes(res)) => res,
            _ => {
                loge!("get secret failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        map.insert_attr(Tag::Secret, decrypt(calling_info, auth_type, access_type, secret)?)?;
    }

    Ok(query_res)
}

fn fuzzy_query(calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<Vec<AssetMap>> {
    let mut query_res = query_data_once("", calling_info, db_data)?;

    for data in &mut query_res {
        data.remove(&Tag::Secret);
    }
    Ok(query_res)
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
            Err(ErrCode::SqliteError)
        },
    }
}

pub(crate) fn pre_query(input: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<u8>> {
    // get param map contains input params and default params
    let input_new = construct_params_with_default(input, &IpcCode::PreQuery)?;

    let mut data_vec = Vec::new();
    set_input_attr(&input_new, &mut data_vec)?;
    // get all pre query data
    let data_vec = Vec::new();
    let all_data: Vec<AssetMap> = fuzzy_query(calling_info, &data_vec)?;
    // get all secret key
    let mut secret_key_set = HashSet::new();
    for map in all_data.iter() {
        let auth_type = match map.get(&Tag::AuthType) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get auth type failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        let access_type = match map.get(&Tag::Accessibility) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get access type failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        secret_key_set.insert(format!("{}_{}_{}_{}", calling_info.user_id(), calling_info.uid(), *auth_type, *access_type).into_bytes());
    }
    // use secret key to get challenge
    let mut challenge_vec = Vec::new();
    // todo 遍历每一个密钥，获取challenge
    let challenge_seperator = b'_';
    for (idx, _) in secret_key_set.iter().enumerate() {
        let tmp_challenge = init_decrypt()?;
        challenge_vec.extend(tmp_challenge);
        if idx < secret_key_set.len() - 1 {
            challenge_vec.push(challenge_seperator);
        }
        // todo 根据challenge等信息创建session
    }
    if challenge_vec.is_empty() {
        Err(ErrCode::NotFound)
    } else {
        logi!("get challenge successful!");
        Ok(challenge_vec)
    }
}