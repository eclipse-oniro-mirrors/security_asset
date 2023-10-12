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

//! This crate implements the interface of asset

pub use asset_common::definition;

use ipc_rust::RemoteObjRef;

use asset_common::{definition::{AssetMap, ErrCode, Result, Version}, loge};
use asset_ipc_interface::{IAsset, SA_ID};
use asset_ipc_proxy::AssetProxy;

fn get_remote() -> Result<RemoteObjRef<AssetProxy>> {
    let object = rust_samgr::get_service_proxy::<AssetProxy>(SA_ID);
    match object {
        Ok(remote) => Ok(remote),
        Err(e) => {
            loge!("[FATAL][RUST SDK]get remote service failed. Error = {}", e);
            Err(ErrCode::ServiceUnvailable)
        }
    }
}

/// This manager provides the capabilities for life cycle management of sensitive user data (Asset) such as passwords
/// and tokens, including adding, removing, updating, and querying.
pub struct Manager {
    remote: RemoteObjRef<AssetProxy>,
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Self> {
        let remote = get_remote()?;
        Ok(Self { remote })
    }

    /// Add an Asset.
    pub fn add(&self, attributes: &AssetMap) -> Result<()> {
        self.remote.add(attributes)
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&self, query: &AssetMap) -> Result<()> {
        self.remote.remove(query)
    }

    /// Update an Asset that matches a search query.
    pub fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        self.remote.update(query, attributes_to_update)
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        self.remote.pre_query(query)
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        self.remote.query(query)
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&self, query: &AssetMap) -> Result<()> {
        self.remote.post_query(query)
    }

    /// Get the version of Asset.
    pub fn get_version() -> Version {
        Version {
            major: 1,
            minor: 0,
            patch: 0
        }
    }
}
