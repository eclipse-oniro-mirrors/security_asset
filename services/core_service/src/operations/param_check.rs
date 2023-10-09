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
#![allow(dead_code)]

mod check_tag;
pub mod value_validity_check;

use asset_common::definition::{AssetMap, Result};

pub(crate) enum ParamCode {
    /// Code for add params.
    Add,
    /// Code for remove params.
    Remove,
    /// Code for update params.
    Update,
    /// Code for update match params.
    UpdateQuery,
    /// Code for pre-query params.
    PreQuery,
    /// Code for query params.
    Query,
    /// Code for post params.
    PostQuery,
}

/// check the validity and comprehensiveness of input params
pub(crate) fn check_params(params: &AssetMap, code: &ParamCode) -> Result<()> {
    // check whether all required params are contained and valid
    check_tag::check_tag_validity(params, code)?;

    // check the validity of param value
    value_validity_check::check_value_validity(params)?; // todo: 和check_tag的命名风格保持一致
    Ok(())
}
