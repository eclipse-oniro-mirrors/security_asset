/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use asset_common::CallingInfo;
use asset_crypto_manager::crypto_manager::CryptoManager;
use asset_definition::{AssetMap, Extension, Result, Tag};

use crate::operations::common;

const REQUIRED_ATTRS: [Tag; 1] = [Tag::AuthChallenge];
const OPTIONAL_ATTRS: [Tag; 2] = [Tag::GroupId, Tag::UserId];

fn check_arguments(query: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    common::check_required_tags(query, &REQUIRED_ATTRS)?;

    let mut valid_tags = REQUIRED_ATTRS.to_vec();
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(query, &valid_tags)?;
    common::check_group_validity(query, calling_info)?;
    common::check_system_permission(query)?;
    common::check_value_validity(query)
}

pub(crate) fn post_query(calling_info: &CallingInfo, handle: &AssetMap) -> Result<()> {
    check_arguments(handle, calling_info)?;
    let challenge = handle.get_bytes_attr(&Tag::AuthChallenge)?;

    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove(calling_info, challenge);
    Ok(())
}
