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
    operations::operation_common::{construct_params_with_default, get_alias, set_input_attr},
};

use asset_common::definition::{AssetMap, Result, Accessibility, AuthType};
use asset_ipc_interface::IpcCode;

use super::operation_common::{query_one_data, decrypt};

const ACCESSIBILITY_LIST: [Accessibility; 2] = [Accessibility::DeviceUnlock, Accessibility::DevoceFirstUnlock];
const AUTH_TYPE_LIST: [AuthType; 2] = [AuthType::Any, AuthType::None];

pub(crate) fn query(input: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<AssetMap>> {
    let res_vec: Vec<AssetMap> = Vec::new();

    // get param map contains input params and default params
    let input_new = construct_params_with_default(input, &IpcCode::Query)?;

    let mut data_vec = Vec::new();
    set_input_attr(&input_new, &mut data_vec)?;

    match get_alias(&input_new) {
        Ok(alias) => {
            let _data = query_one_data(&alias, calling_info, &data_vec)?;
            let _ = decrypt(calling_info, &1, &2, &Vec::from([1;99])); // todo
            Ok(res_vec)
        },
        Err(_) => todo!(),
    }
}
