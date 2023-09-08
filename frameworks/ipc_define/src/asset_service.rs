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

use hilog_rust::{hilog, HiLogLabel, LogType};
use ipc_rust::{
    define_remote_object, BorrowedMsgParcel, IRemoteBroker, IRemoteObj, IpcResult, IpcStatusCode,
    MsgParcel, RemoteObj, RemoteStub, FIRST_CALL_TRANSACTION,
};
use std::ffi::{c_char, CString};

use asset_common_lib::{
    asset_log_info, enum_auto_impl_try_from,
    asset_type::{AssetMap, AssetResult, AssetStatusCode, SerializeAsset, DeserializeAsset}, asset_log_error,
};

enum_auto_impl_try_from!{
    /// Asset ipc code
    #[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
    pub enum AssetIpcCode {
        /// insert data
        Insert = FIRST_CALL_TRANSACTION,
        /// add an asset
        Add,
    }
}

impl fmt::Display for AssetIpcCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            AssetIpcCode::Insert => write!(f, "insert"),
            AssetIpcCode::Add => write!(f, "add"),
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

    /// add an assert
    fn add(&self, input: &AssetMap) -> AssetResult<AssetMap>;
}

fn on_asset_remote_request(
    stub: &dyn AssetBroker,
    code: u32,
    data: &BorrowedMsgParcel,
    reply: &mut BorrowedMsgParcel,
) -> IpcResult<()> {
    let input_map = AssetMap::deserialize(data);
    if input_map.is_err() {
        asset_log_error!("deserialize in on_asset_remote_request failed!");
        return Err(IpcStatusCode::InvalidValue);
    }
    if let Ok(ipc_code) = AssetIpcCode::try_from(code) {
        match ipc_code {
            AssetIpcCode::Insert => {
                asset_log_info!("on_asset_remote_request Insert");
                let res: AssetMap = stub.insert(input_map.as_ref().unwrap()).unwrap();
                let ser_res = res.serialize(reply);
                if ser_res.is_err() {
                    return Err(IpcStatusCode::InvalidValue);
                }
            },
            AssetIpcCode::Add => {
                asset_log_info!("on_asset_remote_request add");
                let res: AssetMap = stub.add(input_map.as_ref().unwrap())?;
                res.serialize(reply)?;
                // to do : 将错误码放到map里，否则会有错误信息丢失
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
    fn insert(&self, input: &AssetMap) -> AssetResult<AssetMap> {
        self.0.insert(input)
    }

    fn add(&self, input: &AssetMap) -> AssetResult<AssetMap> {
        self.0.add(input)
    }
}

fn transform(proxy: &AssetProxy, code: AssetIpcCode, input: &AssetMap) -> AssetResult<AssetMap> {
    let parce_new = MsgParcel::new();
    match parce_new {
        Some(mut send_parcel) => {
            input.serialize(&mut send_parcel.borrowed())?;

            let reply_parcel =
                proxy.remote.send_request(code as u32, &send_parcel, false);
            if let Ok(reply) = reply_parcel {
                let ret = AssetMap::deserialize(reply.borrowed_ref())?;
                Ok(ret)
            } else {
                asset_log_error!("AssetProxy transform {} failed!", code);
                Err(AssetStatusCode::Failed)
            }
        },
        None => Err(AssetStatusCode::Failed),
    }
}

impl AssetBroker for AssetProxy {
    fn insert(&self, input: &AssetMap) -> AssetResult<AssetMap> {
        transform(self, AssetIpcCode::Insert, input)
    }

    fn add(&self, input: &AssetMap) -> AssetResult<AssetMap> {
        transform(self, AssetIpcCode::Add, input)
    }
}
