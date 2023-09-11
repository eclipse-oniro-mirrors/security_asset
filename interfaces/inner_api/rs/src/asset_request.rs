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

//! This create implement the send request

use asset_common::{
    asset_log_info,
    definition::{AssetMap, Result, ErrCode}, asset_log_error,
};

use asset_ipc_define_lib::asset_service::{AssetBroker, ASSET_SERVICE_ID};

use ipc_rust::RemoteObjRef;

use rust_samgr::get_service_proxy;

use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

fn get_asset_service() -> Result<RemoteObjRef<dyn AssetBroker>> {
    let object = get_service_proxy::<dyn AssetBroker>(ASSET_SERVICE_ID);
    match object {
        Ok(remote) => Ok(remote),
        Err(e) => {
            asset_log_error!("get_asset_service failed {}!", @public(e));
            Err(ErrCode::ServiceUnvailable)
        }
    }
}

/// sender
pub(crate) struct AssetIpcProxy {
    proxy: RemoteObjRef<dyn AssetBroker>,
}

/// 2222
impl AssetIpcProxy {
    /// xxx
    pub(crate) fn new() -> Result<AssetIpcProxy> {
        Ok(AssetIpcProxy { proxy: get_asset_service()? })
    }

    /// xxx
    pub(crate) fn insert(&self, input: &AssetMap) -> Result<AssetMap> {
        asset_log_info!("AssetIpcSender insert");
        self.proxy.insert(input)
    }

    /// add
    pub(crate) fn add(&self, input: &AssetMap) -> Result<AssetMap> {
        asset_log_info!("AssetIpcSender add");
        self.proxy.add(input)
    }
}
