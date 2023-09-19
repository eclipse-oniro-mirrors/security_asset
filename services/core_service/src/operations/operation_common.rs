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

mod crypto_adapter;
mod db_adapter;
mod default_params;
mod extra_params;
mod file_operator;

pub(crate) use crypto_adapter::{encrypt, decrypt};
pub(crate) use extra_params::construst_extra_params;
pub(crate) use db_adapter::{set_extra_attrs, set_input_attr, insert_one_data, query_one_data};
pub(crate) use file_operator::create_user_db_dir;
pub(crate) use default_params::construct_params_with_default;

use asset_common::{definition::{AssetMap, Result, Value, ErrCode, Tag},
    loge};

pub(crate) fn get_alias(input: &AssetMap) -> Result<String> {
    let alias;
    if let Some(Value::Bytes(alias_vec)) = input.get(&Tag::Alias) {
        let alias_try = String::from_utf8(alias_vec.clone());
        if let Ok(alias_ok) = alias_try {
            alias = alias_ok;
        } else {
            loge!("parse alias from utf8 failed!");
            return Err(ErrCode::InvalidArgument);
        }
    } else {
        loge!("get alias failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(alias)
}
