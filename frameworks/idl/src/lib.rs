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

//! This module defines IPC interfaces and constants.

use asset_common::{definition::{AssetMap, Result}, impl_enum_trait};

/// SA id for Asset service
pub const SA_ID: i32 = 3511;
/// SA name for Asset service
pub const SA_NAME: &str = "security_asset_service";
/// IPC result code.
pub const IPC_SUCCESS: i32 = 0;

impl_enum_trait!{
    /// Code used to identify the function to be called.
    #[derive(Clone, Copy)]
    pub enum IpcCode {
        /// Code for AddAsset.
        Add = ipc_rust::FIRST_CALL_TRANSACTION,
        /// Code for RemoveAsset.
        Remove,
        /// Code for UpdateAsset.
        Update,
        /// Code for PreQueryAsset.
        PreQuery,
        /// Code for QueryAsset.
        Query,
        /// Code for PostQueryAsset.
        PostQuery,
    }
}

/// Function between proxy and stub of Asset service
pub trait IAsset: ipc_rust::IRemoteBroker {
    /// Add an asset.
    fn add(&self, input: &AssetMap) -> Result<()>;
}
