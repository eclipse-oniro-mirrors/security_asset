/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

//! This module defines the interface of the SAF Rust SDK.
#![feature(once_cell_try)]
pub use saf_definition::*;

use std::sync::{Arc, Mutex, OnceLock};

use saf_log::logw;
use ipc::{parcel::MsgParcel, remote::RemoteObj};
use samgr::manage::SystemAbilityManager;

pub use saf_ipc::{
    ipc_err_handle, serialize_map, IpcCode, IPC_SUCCESS, SA_ID, SA_NAME,
};

const LOAD_TIMEOUT_IN_SECONDS: i32 = 5;

fn load_saf_service() -> Result<RemoteObj> {
    let timeout: i32 = LOAD_TIMEOUT_IN_SECONDS;
    match SystemAbilityManager::load_system_ability(SA_ID, timeout) {
        Some(remote) => Ok(remote),
        None => {
            macros_lib::log_throw_error!(ErrCode::ServiceUnavailable, "[FATAL][RUST SDK]get remote service failed")
        },
    }
}

/// This manager provides the capabilities for safety access.
pub struct Manager {
    remote: RemoteObj,
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Arc<Mutex<Manager>>> {
        static INSTANCE: OnceLock<Arc<Mutex<Manager>>> = OnceLock::new();
        INSTANCE.get_or_try_init(|| {
            logw!("Create instance for Manager.");
            let remote = load_saf_service()?;
            Ok(Arc::new(Mutex::new(Manager { remote })))
        }).cloned()
    }

    /// Check access for certain application.
    pub fn check_access(&mut self, attributes: &SAFMap) -> Result<()> {
        self.process_one_agr_request(attributes, IpcCode::CheckAccess)?;
        Ok(())
    }

    fn rebuild(&mut self) -> Result<()> {
        self.remote = load_saf_service()?;
        Ok(())
    }

    fn process_one_agr_request(&mut self, attributes: &SAFMap, ipc_code: IpcCode) -> Result<MsgParcel> {
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


    fn send_request(&self, mut parcel: MsgParcel, ipc_code: IpcCode) -> Result<MsgParcel> {
        let mut reply = self.remote.send_request(ipc_code as u32, &mut parcel).map_err(ipc_err_handle)?;
        match reply.read::<u32>().map_err(ipc_err_handle)? {
            IPC_SUCCESS => Ok(reply),
            e => {
                let msg = reply.read::<String>().map_err(ipc_err_handle)?;
                macros_lib::throw_error!(ErrCode::try_from(e)?, "{}", msg)
            },
        }
    }

    fn descriptor(&self) -> &'static str {
        SA_NAME
    }
}

