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

//! This module implements the proxy of the asset service.

use ipc_rust::{
    FromRemoteObj, IpcResult, IRemoteBroker, IRemoteObj,
    MsgParcel, RemoteObj, RemoteObjRef
};

use asset_common::{definition::{AssetMap, ErrCode, Result}, loge};
use asset_ipc_interface::{IAsset, IpcCode, IPC_SUCCESS, SA_NAME, serialize_map,
    deserialize_vector_map};

/// Proxy of Asset Service.
pub struct AssetProxy {
    remote: RemoteObj,
}

impl AssetProxy {
    /// Create proxy object by RemoteObj.
    fn from_remote_object(remote: &RemoteObj) -> IpcResult<Self> {
        Ok(Self {
            remote: remote.clone(),
        })
    }

    /// Get proxy object descriptor.
    #[allow(dead_code)]
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

impl IAsset for AssetProxy {
    fn add(&self, input: &AssetMap) -> Result<()> {
        let parce_new = MsgParcel::new();
        match parce_new {
            Some(mut send_parcel) => {
                serialize_map(input, &mut send_parcel.borrowed())?;
                let reply =
                    self.remote.send_request(IpcCode::Add as u32, &send_parcel, false).map_err(|_| ErrCode::IpcError)?;
                    let res_code = reply.read::<i32>().map_err(|_| ErrCode::IpcError)?;
                    if res_code != IPC_SUCCESS {
                        return Err(ErrCode::try_from(res_code)?);
                    }
                    Ok(())
            },
            None => Err(ErrCode::IpcError)
        }
    }

    fn query(&self, input: &AssetMap) -> Result<Vec<AssetMap>> {
        let parce_new = MsgParcel::new();
        match parce_new {
            Some(mut send_parcel) => {
                serialize_map(input, &mut send_parcel.borrowed())?;
                let mut reply =
                    self.remote.send_request(IpcCode::Query as u32, &send_parcel, false).map_err(|e| {
                        loge!("query send request failed! res = [{}]", e);
                        ErrCode::IpcError
                    })?;
                    let res_code = reply.read::<i32>().map_err(|_| ErrCode::IpcError)?;
                    if res_code != IPC_SUCCESS {
                        return Err(ErrCode::try_from(res_code)?);
                    }
                    Ok(deserialize_vector_map(&reply.borrowed())?)
            },
            None => Err(ErrCode::IpcError)
        }
    }

    fn update(&self, input: &AssetMap) -> Result<()> {
        let parce_new = MsgParcel::new();
        match parce_new {
            Some(mut send_parcel) => {
                serialize_map(input, &mut send_parcel.borrowed())?;
                let reply =
                    self.remote.send_request(IpcCode::Update as u32, &send_parcel, false).map_err(|_| ErrCode::IpcError)?;
                    let res_code = reply.read::<i32>().map_err(|_| ErrCode::IpcError)?;
                    if res_code != IPC_SUCCESS {
                        return Err(ErrCode::try_from(res_code)?);
                    }
                    Ok(())
            },
            None => Err(ErrCode::IpcError)
        }
    }
}

impl FromRemoteObj for AssetProxy {
    /// Convert RemoteObj to RemoteObjRef<dyn IAsset>
    fn try_from(object: RemoteObj) -> IpcResult<RemoteObjRef<AssetProxy>> {
        Ok(RemoteObjRef::new(Box::new(AssetProxy::from_remote_object(&object)?)))
    }
}