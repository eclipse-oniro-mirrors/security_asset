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

mod tag_check;
pub(crate) mod value_check;

use asset_common::definition::{AssetMap, Result};
use crate::definition_inner::OperationCode;

/// check the validity and comprehensiveness of input argument
pub(crate) fn check_argument(argument: &AssetMap, code: &OperationCode) -> Result<()> {
    // check whether all required params are contained and valid
    tag_check::check_tag_validity(argument, code)?;

    // check the validity of param value
    value_check::check_value_validity(argument)?;
    Ok(())
}
