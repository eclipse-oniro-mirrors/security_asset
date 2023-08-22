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

use std::fmt;

use ipc_rust::{IRemoteBroker, FIRST_CALL_TRANSACTION, MsgParcel, BorrowedMsgParcel, IpcStatusCode,
    define_remote_object, IpcResult, RemoteStub, RemoteObj, IRemoteObj};
use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

use asset_common_lib::{
    asset_type::{AssetResult, AssetMap, AssetStatusCode, SerializeAsset, DeserializeAsset},
    asset_log_error, asset_log_info
};

/// Asset ipc code
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum AssetIpcCode {
    /// insert data
    Insert = FIRST_CALL_TRANSACTION,
    /// read
    Read,
}

impl fmt::Display for AssetIpcCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            AssetIpcCode::Insert => write!(f, "insert"),
            AssetIpcCode::Read => write!(f, "read"),
        }
    }
}

impl TryFrom<u32> for AssetIpcCode {
    type Error = AssetStatusCode;
    fn try_from(code: u32) -> AssetResult<Self> {
        match code {
            _ if code == AssetIpcCode::Insert as u32 => Ok(AssetIpcCode::Insert),
            _ if code == AssetIpcCode::Read as u32 => Ok(AssetIpcCode::Read),
            _ => {
                asset_log_error!("try convert u32 to AssetIpcCode failed!");
                Err(AssetStatusCode::Failed)
            }
        }
    }
}

/// SA ID for "example_asset_ipc_service"
pub const ASSET_SERVICE_ID: i32 = 3511;

/// Function between proxy and stub of AssetService
pub trait AssetBroker: IRemoteBroker {
    /// xxx
    fn insert(&self, input: &AssetMap) -> AssetResult<AssetMap>;
    // fn transform(&self, code: u32, input: &AssetMap) -> AssetResult<AssetMap>;
}

fn on_asset_remote_request(_stub: &dyn AssetBroker, code: u32, data: &BorrowedMsgParcel,
    _reply: &mut BorrowedMsgParcel) -> IpcResult<()> {
    let input_map = AssetMap::deserialize(data);
    if input_map.is_err() {
        return Err(IpcStatusCode::InvalidValue);
    }
    if let Ok(ipc_code) = AssetIpcCode::try_from(code) {
        match ipc_code {
            AssetIpcCode::Insert => {
                asset_log_info!("on_asset_remote_request Insert");
                // stub.insert(input_map.as_ref().unwrap());
                // to do: pack reply to reply parcel
            },
            AssetIpcCode::Read => {
                asset_log_info!("on_asset_remote_request Read");
                // stub.insert(input_map.as_ref().unwrap()); // to do 
                // to do: pack reply to reply parcel
            }
        }
        Ok(())
    } else {
        Err(IpcStatusCode::InvalidValue)
    }
}

define_remote_object!(
    AssetBroker["security_asset_service"] {
        stub: AssetStub(on_asset_remote_request),
        proxy: AssetProxy,
    }
);

// Make RemoteStub<AssetStub> object can call AssetBroker function directly.
impl AssetBroker for RemoteStub<AssetStub> {
    // fn transform(&self, code: u32, input: &AssetMap) -> AssetResult<AssetMap> {
    //     self.0.transform(code, input)
    // }

    fn insert(&self, input: &AssetMap) -> AssetResult<AssetMap> {
        self.0.insert(input)
    }
}

fn transform(proxy: &AssetProxy, input: &AssetMap) -> AssetResult<AssetMap> {
    let parce_new = MsgParcel::new();
    match parce_new {
        Some(mut send_parcel) => {
            input.serialize(&mut send_parcel)?;

            let reply_parcel = proxy.remote.send_request(AssetIpcCode::Insert as u32,
                &send_parcel, false);
            if let Ok(reply) = reply_parcel {
                let ret = AssetMap::deserialize(reply.borrowed_ref())?;
                Ok(ret)
            } else {
                Err(AssetStatusCode::Failed)
            }
        },
        None => {
            Err(AssetStatusCode::Failed)
        }
    }
}

impl AssetBroker for AssetProxy {
    fn insert(&self, input: &AssetMap) -> AssetResult<AssetMap> {
        transform(self, input)
    }

    // fn transform(&self, code: u32, input: &AssetMap) -> AssetResult<AssetMap> {
    //     let mut send_parcel = MsgParcel::new();
    //     input.serialize(&mut parcel)?;

    //     let reply_parcel = self.remote.send_request(code,
    //         send_parcel, false);
    //     if let Ok(reply) = send_request {
    //         let ret = AssetMap::deserialize(reply)?;
    //         Ok(ret)
    //     } else {
    //         Err(AssetStatusCode::Failed)
    //     }
    // }
}