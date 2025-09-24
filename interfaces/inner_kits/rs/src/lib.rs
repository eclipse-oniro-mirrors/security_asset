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

use std::{sync::{OnceLock, Mutex, Arc, atomic::{AtomicU64, Ordering}}, time::{SystemTime, UNIX_EPOCH}};

pub use asset_definition::*;
pub mod plugin_interface;

use asset_log::logw;
use ipc::{parcel::MsgParcel, remote::RemoteObj};
use samgr::manage::SystemAbilityManager;

use asset_ipc::{
    deserialize_maps, deserialize_sync_result, ipc_err_handle, serialize_map, IpcCode, IPC_SUCCESS, SA_ID, SA_NAME,
};

const LOAD_TIMEOUT_IN_SECONDS: i32 = 4;

fn load_asset_service() -> Result<RemoteObj> {
    match SystemAbilityManager::load_system_ability(SA_ID, LOAD_TIMEOUT_IN_SECONDS) {
        Some(remote) => Ok(remote),
        None => {
            log_throw_error!(ErrCode::ServiceUnavailable, "[FATAL][RUST SDK]get remote service failed")
        },
    }
}

fn get_remote() -> Result<RemoteObj> {
    load_asset_service()
}

/// This manager provides the capabilities for life cycle management of sensitive user data (Asset) such as passwords
/// and tokens, including adding, removing, updating, and querying.
pub struct Manager {
    remote: RemoteObj,
    last_rebuild_time: AtomicU64,
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Arc<Mutex<Manager>> {
        static INSTANCE: OnceLock<Arc<Mutex<Manager>>> = OnceLock::new();
        INSTANCE.get_or_init(|| {
            logw!("Create instance for Manager.");
            let remote = get_remote().expect("Get remote failed.");
            Arc::new(Mutex::new(Manager { remote, last_rebuild_time: 0.into() }))
        }).clone()
    }

    /// Add an Asset.
    pub fn add(&mut self, attributes: &AssetMap) -> Result<()> {
        self.process_one_agr_request(attributes, IpcCode::Add)?;
        Ok(())
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&mut self, query: &AssetMap) -> Result<()> {
        self.process_one_agr_request(query, IpcCode::Remove)?;
        Ok(())
    }

    /// Update an Asset that matches a search query.
    pub fn update(&mut self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        self.process_two_agr_request(query, attributes_to_update, IpcCode::Update)?;
        Ok(())
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&mut self, query: &AssetMap) -> Result<Vec<u8>> {
        let mut reply = self.process_one_agr_request(query, IpcCode::PreQuery)?;
        let res = reply.read::<Vec<u8>>().map_err(ipc_err_handle)?;
        Ok(res)
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&mut self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        let mut reply = self.process_one_agr_request(query, IpcCode::Query)?;
        let res = deserialize_maps(&mut reply)?;
        Ok(res)
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&mut self, query: &AssetMap) -> Result<()> {
        self.process_one_agr_request(query, IpcCode::PostQuery)?;
        Ok(())
    }

    /// Query the result of synchronization.
    pub fn query_sync_result(&mut self, query: &AssetMap) -> Result<SyncResult> {
        match self.process_one_agr_request(query, IpcCode::QuerySyncResult) {
            Ok(mut reply) => {
                let sync_result = deserialize_sync_result(&mut reply)?;
                Ok(sync_result)
            },
            Err(mut e) => {
                if e.code == ErrCode::InvalidArgument {
                    e.code = ErrCode::ParamVerificationFailed;
                }
                Err(e)
            },
        }
    }

    fn rebuild(&mut self) -> Result<()> {
        let now = match SystemTime::now().duration_since(UNIX_EPOCH) {
            Ok(d) => Ok(d.as_secs()),
            Err(e) => log_throw_error!(ErrCode::GetSystemTimeError, "[FATAL]Get system time failed, err: {}", e),
        }?;
        let last_time = self.last_rebuild_time.load(Ordering::Relaxed);
        if last_time - now > (LOAD_TIMEOUT_IN_SECONDS as u64) {
            self.remote = get_remote()?;
            self.last_rebuild_time.store(now, Ordering::Relaxed);
        }
        Ok(())
    }

    fn process_one_agr_request(&mut self, attributes: &AssetMap, ipc_code: IpcCode) -> Result<MsgParcel> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(attributes, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(msg) => Ok(msg),
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_map(attributes, &mut parcel)?;
                    self.send_request(parcel, ipc_code)
                },
                _ => Err(e),
            },
        }
    }

    fn process_two_agr_request(
        &mut self,
        query: &AssetMap,
        attributes_to_update: &AssetMap,
        ipc_code: IpcCode,
    ) -> Result<MsgParcel> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(query, &mut parcel)?;
        serialize_map(attributes_to_update, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(msg) => Ok(msg),
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_map(query, &mut parcel)?;
                    serialize_map(attributes_to_update, &mut parcel)?;
                    self.send_request(parcel, ipc_code)
                },
                _ => Err(e),
            },
        }
    }

    fn send_request(&self, mut parcel: MsgParcel, ipc_code: IpcCode) -> Result<MsgParcel> {
        let mut reply = self.remote.send_request(ipc_code as u32, &mut parcel).map_err(ipc_err_handle)?;
        match reply.read::<u32>().map_err(ipc_err_handle)? {
            IPC_SUCCESS => Ok(reply),
            e => {
                let msg = reply.read::<String>().map_err(ipc_err_handle)?;
                throw_error!(ErrCode::try_from(e)?, "{}", msg)
            },
        }
    }

    fn descriptor(&self) -> &'static str {
        SA_NAME
    }
}
