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

use crate::{
    calling_info::CallingInfo,
    operations::operation_common::{
        construct_params_with_default, decrypt,
        db_adapter::{set_input_attr, query_data_once},
    },
};

use db_operator::types::Pair;

use asset_common::definition::{AssetMap, Result, Insert, Tag};
use asset_ipc_interface::IpcCode;

fn single_query(calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<Vec<AssetMap>> {
    let mut query_res = query_data_once(calling_info, db_data)?;

    for map in &mut query_res {
        map.insert_attr(Tag::Secret, decrypt(calling_info, map)?)?;
    }

    Ok(query_res)
}

pub(crate) fn batch_query(calling_info: &CallingInfo, db_data: &Vec<Pair>) -> Result<Vec<AssetMap>> {
    let mut query_res = query_data_once(calling_info, db_data)?;

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
    if input_new.contains_key(&Tag::Alias) {
        single_query(calling_info, &data_vec)
    } else {
        batch_query(calling_info, &data_vec)
    }
}
