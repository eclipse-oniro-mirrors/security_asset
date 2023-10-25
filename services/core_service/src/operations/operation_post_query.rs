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

use asset_crypto_manager::crypto::CryptoManager;
use asset_definition::{AssetMap, ErrCode, Result, Tag, Value};

use crate::{calling_info::CallingInfo, operations::common};

const REQUIRED_ATTRS: [Tag; 1] = [Tag::AuthChallenge];

fn check_arguments(query: &AssetMap) -> Result<()> {
    common::check_required_tags(query, &REQUIRED_ATTRS)?;
    common::check_value_validity(query)
}

// todo: to implement
pub(crate) fn post_query(handle: &AssetMap, _calling_info: &CallingInfo) -> Result<()> {
    check_arguments(handle)?;
    let Some(Value::Bytes(ref challenge)) = handle.get(&Tag::AuthChallenge)
        else { return Err(ErrCode::InvalidArgument) };

    // todo crypto manager的获取需要改用单例模式
    let mut crypto_manager = CryptoManager::new();
    // todo 等接口改了之后删掉challenge_pos参数
    crypto_manager.remove(challenge);
    Ok(())
}
