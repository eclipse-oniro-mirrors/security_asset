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
    operations::operation_common::construct_params_with_default
};

use asset_common::definition::{AssetMap, Result};
use asset_ipc_interface::IpcCode;

pub(crate) fn query(input: &AssetMap, _calling_info: &CallingInfo) -> Result<Vec<AssetMap>> {
    let res_vec: Vec<AssetMap> = Vec::new();

    // get param map contains input params and default params
    let _input_new = construct_params_with_default(input, &IpcCode::Add)?;

    Ok(res_vec)
}
