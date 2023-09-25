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

use asset_common::{loge, definition::{Result, ErrCode}};

extern "C" {
    fn GetUserIdByUid(uid: u64, userId: &mut u32) -> bool;
}

/// xxx
pub(crate) fn get_calling_user_id(uid: u64) ->  Result<u32> {
    unsafe {
        let mut user_id = 0;
        if GetUserIdByUid(uid, &mut user_id) {
            Ok(user_id)
        } else {
            loge!("get userid failed!");
            Err(ErrCode::Failed) // ACCOUNT_FAIL
        }
    }
}