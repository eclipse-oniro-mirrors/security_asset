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

//! This create implement the asset

use std::ffi::{c_char, CString};

use ipc_rust::{
    define_remote_object, BorrowedMsgParcel, IpcResult, IRemoteObj,
    IpcStatusCode, MsgParcel, RemoteObj, RemoteStub,
};

use asset_common::{
    logi, loge,
    definition::{AssetMap, Result, ErrCode, SerializeAsset, DeserializeAsset},
};
use super::iasset::{IAsset, IpcCode};

/// IPC entry of the Asset SDK
fn on_remote_request(stub: &dyn IAsset, code: u32, data: &BorrowedMsgParcel,
    reply: &mut BorrowedMsgParcel) -> IpcResult<()> {
    logi!("on_remote_request, calling function: {}", code);
    let input_map = AssetMap::deserialize(data);
    if input_map.is_err() {
        loge!("deserialize in on_remote_request failed!");
        return Err(IpcStatusCode::InvalidValue);
    }
    if let Ok(ipc_code) = IpcCode::try_from(code) {
        match ipc_code {
            IpcCode::Add => {
                logi!("on_remote_request add");

                match stub.add(input_map.as_ref().unwrap()) {
                    Ok(_) => {
                        reply.write::<i32>(&(ErrCode::Success as i32))?;
                    },
                    Err(e) => {
                        reply.write::<i32>(&(e as i32))?;
                    }
                }
            },
            IpcCode::Remove => (),
            _ => ()
        }
        Ok(())
    } else {
        Err(IpcStatusCode::InvalidValue)
    }
}

define_remote_object!(
    IAsset["security_asset_service"] {
        stub: AssetStub(on_remote_request),
        proxy: AssetProxy,
    }
);

// Make RemoteStub<AssetStub> object can call IAsset function directly.
impl IAsset for RemoteStub<AssetStub> {
    fn add(&self, input: &AssetMap) -> Result<()> {
        self.0.add(input)
    }
}

impl IAsset for AssetProxy {
    fn add(&self, input: &AssetMap) -> Result<()> {
        let parce_new = MsgParcel::new();
        match parce_new {
            Some(mut send_parcel) => {
                input.serialize(&mut send_parcel.borrowed())?;
                let reply_parcel = self.remote.send_request(IpcCode::Add as u32, &send_parcel, false);
                if let Ok(reply) = reply_parcel {
                    let res_code = ErrCode::try_from(reply.read::<i32>()?)?;
                    if res_code != ErrCode::Success {
                        return Err(res_code);
                    }
                    Ok(())
                } else {
                    loge!("AssetProxy transform {} failed!", IpcCode::Add);
                    Err(ErrCode::Failed)
                }
            },
            None => Err(ErrCode::Failed)
        }
    }
}
