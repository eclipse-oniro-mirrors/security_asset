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

//! This module is used to clear resources after query the Asset that required secondary identity authentication.

use asset_constants::CallingInfo;
use asset_crypto_manager::crypto_manager::CryptoManager;
use asset_definition::{AssetMap, Extension, Result, Tag};

use crate::operations::common;

const REQUIRED_ATTRS: [Tag; 1] = [Tag::AuthChallenge];

fn check_arguments(query: &AssetMap) -> Result<()> {
    common::check_required_tags(query, &REQUIRED_ATTRS)?;
    common::check_value_validity(query)
}

pub(crate) fn post_query(handle: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(handle)?;
    let challenge = handle.get_bytes_attr(&Tag::AuthChallenge)?;

    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove(calling_info, challenge);
    Ok(())
}
