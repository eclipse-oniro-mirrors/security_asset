
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

//! This create implement the asset

use std::fmt;

use asset_common::{
    impl_try_from,
    definition::{AssetMap, Result}
};

/// SA ID for Asset service
pub const SA_ID: i32 = 3511;

impl_try_from!{
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

impl fmt::Display for IpcCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            IpcCode::Add => write!(f, "AddAsset"),
            IpcCode::Remove => write!(f, "RemoveAsset"),
            IpcCode::Update => write!(f, "UpdateAsset"),
            IpcCode::PreQuery => write!(f, "PreQueryAsset"),
            IpcCode::Query => write!(f, "QueryAsset"),
            IpcCode::PostQuery => write!(f, "PostQueryAsset"),
        }
    }
}

/// Function between proxy and stub of Asset service
pub trait IAsset: ipc_rust::IRemoteBroker {
    /// add an assert
    fn add(&self, input: &AssetMap) -> Result<()>;
}