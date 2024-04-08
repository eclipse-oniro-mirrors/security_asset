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

//! This module defines the interface of the Asset Rust SDK.

pub use asset_definition::*;

use ipc::{parcel::MsgParcel, remote::RemoteObj};
use samgr::manage::SystemAbilityManager;

use asset_ipc::{deserialize_maps, ipc_err_handle, serialize_map, IpcCode, IPC_SUCCESS, SA_ID};

fn get_remote() -> Result<RemoteObj> {
    match SystemAbilityManager::get_system_ability(SA_ID) {
        Some(remote) => Ok(remote),
        None => {
            log_throw_error!(ErrCode::ServiceUnavailable, "[FATAL][RUST SDK]get remote service failed")
        },
    }
}

/// This manager provides the capabilities for life cycle management of sensitive user data (Asset) such as passwords
/// and tokens, including adding, removing, updating, and querying.
pub struct Manager {
    remote: RemoteObj,
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Self> {
        let remote = get_remote()?;
        Ok(Self { remote })
    }

    /// Add an Asset.
    pub fn add(&self, attributes: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        serialize_map(attributes, &mut parcel)?;
        self.send_request(parcel, IpcCode::Add)?;
        Ok(())
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&self, query: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        serialize_map(query, &mut parcel)?;
        self.send_request(parcel, IpcCode::Remove)?;
        Ok(())
    }

    /// Update an Asset that matches a search query.
    pub fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        serialize_map(query, &mut parcel)?;
        serialize_map(attributes_to_update, &mut parcel)?;
        self.send_request(parcel, IpcCode::Update)?;
        Ok(())
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        let mut parcel = MsgParcel::new();
        serialize_map(query, &mut parcel)?;
        let mut reply = self.send_request(parcel, IpcCode::PreQuery)?;
        let res = reply.read::<Vec<u8>>().map_err(ipc_err_handle)?;
        Ok(res)
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        let mut parcel = MsgParcel::new();
        serialize_map(query, &mut parcel)?;
        let mut reply = self.send_request(parcel, IpcCode::Query)?;
        let res = deserialize_maps(&mut reply)?;
        Ok(res)
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&self, query: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        serialize_map(query, &mut parcel)?;
        self.send_request(parcel, IpcCode::PostQuery)?;
        Ok(())
    }

    fn send_request(&self, mut parcel: MsgParcel, ipc_code: IpcCode) -> Result<MsgParcel> {
        let mut reply = self.remote.send_request(ipc_code as u32, &mut parcel).map_err(ipc_err_handle)?;
        match reply.read::<u32>().map_err(ipc_err_handle)? {
            IPC_SUCCESS => Ok(reply),
            e => {
                let msg = reply.read::<String>().map_err(ipc_err_handle)?;
                log_throw_error!(ErrCode::try_from(e)?, "{}", msg)
            }
        }
    }
}
