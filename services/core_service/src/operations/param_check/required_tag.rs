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

//! This file implement the asset param check

use asset_common::{
    definition::{AssetMap, ErrCode, Result, Tag},
    loge};

use asset_ipc_interface::IpcCode;

const ADD_REQUIRED_PARAMS: [Tag; 2] = [
    Tag::Secret, Tag::Alias
];

fn check_required_params_inner(params: &AssetMap, required_params: &[Tag]) -> Result<()> {
    for param in required_params {
        if !params.contains_key(param) {
            loge!("tag [{}] missed", param);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}

pub(crate) fn check_required_params(params: &AssetMap, code: &IpcCode) -> Result<()> {
    match *code {
        IpcCode::Add => {
            check_required_params_inner(params, &ADD_REQUIRED_PARAMS)
        },
        _ => {
            Ok(())
        }
    }
}
