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

use ipc_rust::RemoteObjRef;
use rust_samgr::get_service_proxy;

use asset_common::{
    logi, loge,
    definition::{AssetMap, Result, ErrCode},
};
use asset_ipc::asset_service::{IAsset, ASSET_SERVICE_ID};

fn get_asset_service() -> Result<RemoteObjRef<dyn IAsset>> {
    let object = get_service_proxy::<dyn IAsset>(ASSET_SERVICE_ID);
    match object {
        Ok(remote) => Ok(remote),
        Err(e) => {
            loge!("[FATAL]get_asset_service failed {}!", e);
            Err(ErrCode::ServiceUnvailable)
        }
    }
}

/// sender
pub(crate) struct AssetProxy {
    proxy: RemoteObjRef<dyn IAsset>,
}

/// 2222
impl AssetProxy {
    /// xxx
    pub(crate) fn build() -> Result<AssetProxy> {
        Ok(AssetProxy { proxy: get_asset_service()? })
    }

    /// add
    pub(crate) fn add(&self, input: &AssetMap) -> Result<()> {
        logi!("AssetIpcSender add");
        self.proxy.add(input)
    }
}
