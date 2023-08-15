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

use super::AssetIpcSender;
use asset_common_lib::{
    asset_log_info,
    asset_type::{AssetIpcCode, AssetResult, AssetStatusCode},
};
use ipc_rust::{
    IRemoteBroker,
    // FromRemoteObj, IRemoteObj, RemoteObjRef,
    // get_service, IpcStatusCode,
    IpcResult,
};

use hilog_rust::{hilog, info, HiLogLabel, LogType};
use std::ffi::{c_char, CString};
// use crate::asset_ipc_client::{asset_serialize, asset_deserialize};

/// Function between proxy and stub of ITestService
pub trait AssetBroker: IRemoteBroker {
    /// Test sync transaction
    fn test_sync_transaction(&self, value: &str, delay_time: i32) -> IpcResult<i32>;
}

/// SA ID for "test.ipc.ITestService"
// pub const IPC_TEST_SERVICE_ID: i32 = 5555;

// fn get_test_service() -> RemoteObjRef<dyn AssetBroker>
// {
//     let object = get_service(IPC_TEST_SERVICE_ID).expect("get attest service failed");
//     let remote = <dyn AssetBroker as FromRemoteObj>::try_from(object);
//     let remote = match remote {
//         Ok(x) => x,
//         Err(error) => {
//             println!("convert RemoteObj to TestProxy failed: {}", error);
//             panic!();
//         }
//     };
//     remote
// }

/// 2222
impl AssetIpcSender {
    pub fn send_request(&self, ipc_code: AssetIpcCode, data: &str) -> AssetResult<AssetStatusCode> {
        asset_log_info!("AssetRequest", "test send request {}, {}", ipc_code, data);
        Ok(AssetStatusCode::Ok)
    }
}
