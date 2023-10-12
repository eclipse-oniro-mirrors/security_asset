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

mod calling_type;
mod calling_user_id;

use asset_common::definition::Result;
use calling_type::{get_calling_owner, Owner};
use calling_user_id::get_calling_user_id;

use ipc_rust::get_calling_uid;

/// calling info
pub(crate) struct CallingInfo {
    owner_type: Owner,
    user_id: i32,
}

impl CallingInfo {
    /// x
    pub(crate) fn build() -> Result<Self> {
        let uid = get_calling_uid();
        let user_id = get_calling_user_id(uid)?;
        Ok(CallingInfo {
            owner_type: get_calling_owner(uid, user_id)?,
            user_id
        })
    }

    /// get owner type hap: 1 native: 2
    pub(crate) fn owner_type(&self) -> u32 {
        self.owner_type.get_type_num()
    }

    /// get owner info hap:
    pub(crate) fn owner_text(&self) -> Result<String> {
        self.owner_type.get_owner_text()
    }

    /// get user id
    pub(crate) fn user_id(&self) -> i32 {
        self.user_id
    }
}