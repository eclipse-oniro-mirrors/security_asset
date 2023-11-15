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

//! This module is used to switch the identity of the caller.

use asset_definition::{AssetError, ErrCode, Result};
use asset_log::loge;

pub(crate) struct IdentityScope {
    identity: String,
}

impl IdentityScope {
    pub(crate) fn build() -> Result<Self> {
        let identity = ipc_rust::reset_calling_identity().map_err(|e| {
            AssetError::new(ErrCode::IpcError, format!("[FATAL][SA]Reset calling identity failed, error is [{}].", e))
        })?;
        Ok(Self { identity })
    }
}

impl Drop for IdentityScope {
    fn drop(&mut self) {
        if !ipc_rust::set_calling_identity(self.identity.clone()) {
            loge!("[FATAL][SA]Set calling identity failed.");
        }
    }
}
