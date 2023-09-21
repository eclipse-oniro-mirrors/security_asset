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

mod tag_value_match;
mod check_tag;
pub mod value_validity_check;

use asset_common::definition::{AssetMap, Result};

use asset_ipc_interface::IpcCode;

/// check the validity and comprehensiveness of input params
pub(crate) fn check_params(params: &AssetMap, code: &IpcCode) -> Result<()> {
    // check whether all required params are contained
    check_tag::check_tag_validity(params, code)?;

    // check the param tags and the param value's types are matched
    tag_value_match::check_tag_value_match(params)?;

    // check the validity of param value
    value_validity_check::check_value_validity(params)?;
    Ok(())
}
