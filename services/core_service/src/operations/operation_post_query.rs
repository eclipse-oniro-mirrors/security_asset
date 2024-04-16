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

use asset_constants::CallingInfo;
use asset_crypto_manager::crypto_manager::CryptoManager;
use asset_definition::{AssetMap, Extension, Result, Tag, Value};

use crate::operations::common;

const REQUIRED_ATTRS: [Tag; 1] = [Tag::AuthChallenge];
const OPTIONAL_ATTRS: [Tag; 1] = [Tag::SpecificUserId];

fn check_arguments(query: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    common::check_required_tags(query, &REQUIRED_ATTRS)?;

    let mut valid_tags = REQUIRED_ATTRS.to_vec();
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_system_permission_if_needed(calling_info.has_specific_user_id())?;
    common::check_value_validity(query)
}

pub(crate) fn post_query(handle: &AssetMap, calling_info: &mut CallingInfo) -> Result<()> {
    if let Some(Value::Number(num)) = handle.get(&Tag::SpecificUserId) {
        calling_info.set_specific_user_id(*num as i32)?;
    }
    check_arguments(handle, calling_info)?;
    let challenge = handle.get_bytes_attr(&Tag::AuthChallenge)?;

    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove(calling_info, challenge);
    Ok(())
}
