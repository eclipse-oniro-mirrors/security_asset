/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

//! This module defines the interface of the Asset Rust SDK.

use std::sync::{Mutex, Arc};

pub use asset_definition::*;

use ipc::{remote::RemoteObj};

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Arc<Mutex<Manager>>> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Add an Asset.
    pub fn add(&mut self, _attributes: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&mut self, _query: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Update an Asset that matches a search query.
    pub fn update(&mut self, _query: &AssetMap, _attributes_to_update: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&mut self, _query: &AssetMap) -> Result<Vec<u8>> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&mut self, _query: &AssetMap) -> Result<Vec<AssetMap>> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&mut self, _query: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Query the result of synchronization.
    pub fn query_sync_result(&mut self, _query: &AssetMap) -> Result<SyncResult> {
        macros_lib::log_throw_error!(
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }
}
