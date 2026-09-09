/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

use std::sync::{Arc, Mutex};

pub use asset_definition::*;

use ipc::remote::RemoteObj;

/// This manager provides the capabilities for life cycle management of sensitive user data (Asset) such as passwords
/// and tokens, including adding, removing, updating, and querying.
#[allow(dead_code)]
pub struct Manager {
    remote: RemoteObj,
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Arc<Mutex<Manager>>> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Add an Asset.
    pub fn add(&mut self, _attributes: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&mut self, _query: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Update an Asset that matches a search query.
    pub fn update(&mut self, _query: &AssetMap, _attributes_to_update: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Add batch assets.
    pub fn batch_add(&mut self, _attributes_array: &[AssetMap]) -> Result<Vec<(u32, u32)>> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Remove batch assets.
    pub fn batch_remove(&mut self, _attributes_array: &[AssetMap]) -> Result<()> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Update batch assets.
    pub fn batch_update(
        &mut self,
        _attributes_array: &[AssetMap],
        _attributes_to_update_array: &[AssetMap],
    ) -> Result<Vec<(u32, u32)>> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&mut self, _query: &AssetMap) -> Result<Vec<u8>> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&mut self, _query: &AssetMap) -> Result<Vec<AssetMap>> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&mut self, _query: &AssetMap) -> Result<()> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }

    /// Query the result of synchronization.
    pub fn query_sync_result(&mut self, _query: &AssetMap) -> Result<SyncResult> {
        macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::Unsupported,
            "[FATAL][RUST SDK]Asset service is not supported in empty mode"
        )
    }
}

/// Build and initialize the Manager with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_build() -> Result<Arc<Mutex<Manager>>> {
    Manager::build()
}

/// Add an Asset with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_add(manager: &mut Manager, attributes: &AssetMap) -> Result<()> {
    manager.add(attributes)
}

/// Add batch assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_batch_add(manager: &mut Manager, attributes_array: &[AssetMap]) -> Result<Vec<(u32, u32)>> {
    manager.batch_add(attributes_array)
}

/// Remove Assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_remove(manager: &mut Manager, query: &AssetMap) -> Result<()> {
    manager.remove(query)
}

/// Remove batch assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_batch_remove(manager: &mut Manager, attributes_array: &[AssetMap]) -> Result<()> {
    manager.batch_remove(attributes_array)
}

/// Update batch assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_batch_update(
    manager: &mut Manager,
    attributes_array: &[AssetMap],
    attributes_to_update_array: &[AssetMap],
) -> Result<Vec<(u32, u32)>> {
    manager.batch_update(attributes_array, attributes_to_update_array)
}

/// Update an Asset with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_update(
    manager: &mut Manager,
    query: &AssetMap,
    attributes_to_update: &AssetMap,
) -> Result<()> {
    manager.update(query, attributes_to_update)
}

/// Pre-query Assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_pre_query(manager: &mut Manager, query: &AssetMap) -> Result<Vec<u8>> {
    manager.pre_query(query)
}

/// Query Assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_query(manager: &mut Manager, query: &AssetMap) -> Result<Vec<AssetMap>> {
    manager.query(query)
}

/// Post-query Assets with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_post_query(manager: &mut Manager, query: &AssetMap) -> Result<()> {
    manager.post_query(query)
}

/// Query sync result with stable export symbol.
#[no_mangle]
pub fn asset_sdk_manager_query_sync_result(manager: &mut Manager, query: &AssetMap) -> Result<SyncResult> {
    manager.query_sync_result(query)
}
