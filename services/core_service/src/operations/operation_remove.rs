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

use crate::{
    calling_info::CallingInfo,
    operations::operation_common::{
        get_alias,
        db_adapter::{set_input_attr, remove_data_once},
    },
};

use asset_common::{definition::{AssetMap, Result, ErrCode}, logi, loge};

pub(crate) fn remove(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    let mut data_vec = Vec::new();
    set_input_attr(input, &mut data_vec)?;
    match get_alias(input) {
        Ok(alias) => {
            // call sql to remove
            let remove_num = remove_data_once(&alias, calling_info, &data_vec)?;
            logi!("remove {} data", remove_num);
            Ok(())
        },
        Err(ErrCode::NotFound) => {
            // call sql to remove
            let remove_num = remove_data_once("", calling_info, &data_vec)?;
            logi!("remove {} data", remove_num);
            Ok(())
        },
        _ => {
            loge!("get alias and not not found failed!");
            Err(ErrCode::SqliteError)
        },
    }
}
