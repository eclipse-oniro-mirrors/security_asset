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

use asset_common_lib::{asset_type::{AssetMap, AssetResult, Tag}, asset_log_info};
use db_operator::{database_table_helper::DefaultDatabaseHelper, types::Pair};
use super::operation_common::*;

use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

fn construct_data(input: &AssetMap) -> AssetResult<Vec<Pair>> {
    let mut data_vec = Vec::new();

    get_set_attr(input, "Secret", Tag::Secret, &mut data_vec)?;

    get_set_attr(input, "SyncType", Tag::SyncType, &mut data_vec)?;
    get_set_attr(input, "AuthType", Tag::AuthType, &mut data_vec)?;

    get_set_delete_type(&mut data_vec)?;
    get_set_access_type(&mut data_vec)?;
    get_set_owner_type(&mut data_vec)?;
    get_set_version(&mut data_vec)?;
    get_set_current_time(&mut data_vec)?;
    get_set_update_time(&mut data_vec)?;

    Ok(data_vec)
}

pub(crate) fn add(input: &AssetMap) -> AssetResult<AssetMap> {
    // encrypt secret

    // arrange the table value
    let db_data = construct_data(input)?;

//

    // to do 创建用户目录

    // call sql to add
    let insert_num =
        DefaultDatabaseHelper::insert_datas_default_once(1, "owner1", "Alias1", db_data)?;

    asset_log_info!("inser {} data", @public(insert_num));
    Ok(AssetMap::new())
}
