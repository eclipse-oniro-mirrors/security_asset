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
pub mod plugin_interface;

use ipc::{parcel::MsgParcel, remote::RemoteObj};
use samgr::manage::SystemAbilityManager;

use asset_ipc::{deserialize_maps, ipc_err_handle, serialize_map, IpcCode, IPC_SUCCESS, SA_ID, SA_NAME};

const LOAD_TIMEOUT_IN_SECONDS: i32 = 4;

fn load_asset_service() -> Result<RemoteObj> {
    match SystemAbilityManager::load_system_ability(SA_ID, LOAD_TIMEOUT_IN_SECONDS) {
        Some(remote) => Ok(remote),
        None => {
            log_throw_error!(ErrCode::ServiceUnavailable, "[FATAL][RUST SDK]get remote service failed")
        },
    }
}

fn get_remote(need_check: bool) -> Result<RemoteObj> {
    if need_check {
        match SystemAbilityManager::check_system_ability(SA_ID) {
            Some(remote) => Ok(remote),
            None => load_asset_service()
        }
    } else {
        load_asset_service()
    }
}

/// This manager provides the capabilities for life cycle management of sensitive user data (Asset) such as passwords
/// and tokens, including adding, removing, updating, and querying.
pub struct Manager {
    remote: RemoteObj,
}

macro_rules! process_request {
    ($func:path, $manager:expr, $first_arg:expr) => {{
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(manager.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(attributes, &mut parcel)?;
        match manager.send_request(parcel, first_arg) {
            Ok(_) => (),
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(manager.descriptor()).map_err(ipc_err_handle)?;
                    serialize_map(attributes, &mut parcel)?;
                    manager.send_request(parcel, first_arg)?;
                },
                _ => return Err(e)
            }
        }
        Ok(())
    }};
    ($func:path, $calling_info:expr, $first_arg:expr, $second_arg:expr) => {{
        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        // Create de database directory if not exists.
        create_user_de_dir($calling_info.user_id())?;
        upload_system_event($func($calling_info, $first_arg, $second_arg), $calling_info, start, func_name, $first_arg)
    }};
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Self> {
        let remote = get_remote(true)?;
        Ok(Self { remote })
    }

    /// Add an Asset.
    pub fn add(&self, attributes: &AssetMap) -> Result<()> {
        process_request!(&self, IpcCode::Add)
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&self, query: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(query, &mut parcel)?;
        self.send_request(parcel, IpcCode::Remove)?;
        Ok(())
    }

    /// Update an Asset that matches a search query.
    pub fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(query, &mut parcel)?;
        serialize_map(attributes_to_update, &mut parcel)?;
        self.send_request(parcel, IpcCode::Update)?;
        Ok(())
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(query, &mut parcel)?;
        let mut reply = self.send_request(parcel, IpcCode::PreQuery)?;
        let res = reply.read::<Vec<u8>>().map_err(ipc_err_handle)?;
        Ok(res)
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(query, &mut parcel)?;
        let mut reply = self.send_request(parcel, IpcCode::Query)?;
        let res = deserialize_maps(&mut reply)?;
        Ok(res)
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&self, query: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
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
            },
        }
    }

    fn descriptor(&self) -> &'static str {
        SA_NAME
    }
}
