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

//! This module implements the proxy of the Asset service.

#![allow(dead_code)]

use ipc_rust::{
    FromRemoteObj, IRemoteBroker, IRemoteObj, IpcResult, IpcStatusCode, MsgParcel, RemoteObj, RemoteObjRef,
};

use asset_definition::{AssetMap, ErrCode, Result};
use asset_ipc::{deserialize_maps, serialize_map, IAsset, IpcCode, IPC_SUCCESS, SA_NAME};
use asset_log::loge;

/// Proxy of Asset Service.
pub(crate) struct AssetProxy {
    remote: RemoteObj,
}

impl AssetProxy {
    /// Create proxy object by RemoteObj.
    fn from_remote_object(remote: &RemoteObj) -> IpcResult<Self> {
        Ok(Self { remote: remote.clone() })
    }

    /// Get proxy object descriptor.
    pub fn get_descriptor() -> &'static str {
        SA_NAME
    }
}

impl IRemoteBroker for AssetProxy {
    /// Get RemoteObject object from proxy.
    fn as_object(&self) -> Option<RemoteObj> {
        Some(self.remote.clone())
    }
}

fn ipc_err_handle(e: IpcStatusCode) -> ErrCode {
    loge!("[FATAL][IPC]Ipc status code = {}", e);
    ErrCode::IpcError
}

impl FromRemoteObj for AssetProxy {
    /// Convert RemoteObj to RemoteObjRef<dyn IAsset>.
    fn try_from(object: RemoteObj) -> IpcResult<RemoteObjRef<AssetProxy>> {
        Ok(RemoteObjRef::new(Box::new(AssetProxy::from_remote_object(&object)?)))
    }
}

impl AssetProxy {
    fn send_request(&self, parcel: MsgParcel, ipc_code: IpcCode) -> Result<MsgParcel> {
        let reply = self.remote.send_request(ipc_code as u32, &parcel, false).map_err(ipc_err_handle)?;
        match reply.read::<u32>().map_err(ipc_err_handle)? {
            IPC_SUCCESS => Ok(reply),
            e => Err(ErrCode::try_from(e)?),
        }
    }
}

impl IAsset for AssetProxy {
    fn add(&self, attributes: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new().ok_or(ErrCode::IpcError)?;
        serialize_map(attributes, &mut parcel.borrowed())?;
        self.send_request(parcel, IpcCode::Add)?;
        Ok(())
    }

    fn remove(&self, query: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new().ok_or(ErrCode::IpcError)?;
        serialize_map(query, &mut parcel.borrowed())?;
        self.send_request(parcel, IpcCode::Remove)?;
        Ok(())
    }

    fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new().ok_or(ErrCode::IpcError)?;
        serialize_map(query, &mut parcel.borrowed())?;
        serialize_map(attributes_to_update, &mut parcel.borrowed())?;
        self.send_request(parcel, IpcCode::Update)?;
        Ok(())
    }

    fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        let mut parcel = MsgParcel::new().ok_or(ErrCode::IpcError)?;
        serialize_map(query, &mut parcel.borrowed())?;
        let reply = self.send_request(parcel, IpcCode::PreQuery)?;
        let res = reply.read::<Vec<u8>>().map_err(ipc_err_handle)?;
        Ok(res)
    }

    fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        let mut parcel = MsgParcel::new().ok_or(ErrCode::IpcError)?;
        serialize_map(query, &mut parcel.borrowed())?;
        let mut reply = self.send_request(parcel, IpcCode::Query)?;
        let res = deserialize_maps(&reply.borrowed())?;
        Ok(res)
    }

    fn post_query(&self, query: &AssetMap) -> Result<()> {
        let mut parcel = MsgParcel::new().ok_or(ErrCode::IpcError)?;
        serialize_map(query, &mut parcel.borrowed())?;
        self.send_request(parcel, IpcCode::PostQuery)?;
        Ok(())
    }
}
