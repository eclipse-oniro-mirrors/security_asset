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

//! This module implements the stub of the asset service.

use ipc_rust::{
    BorrowedMsgParcel, FileDesc, IpcResult, IpcStatusCode, IRemoteStub,
    RemoteStub, String16
};

use asset_ipc_interface::{IAsset, IpcCode, IPC_SUCCESS, serialize_vector_map, deserialize_map};
use asset_common::{
    loge,
    logi,
};

/// IPC entry of the Asset service
fn on_remote_request(stub: &dyn IAsset, code: u32, data: &BorrowedMsgParcel,
    reply: &mut BorrowedMsgParcel) -> IpcResult<()> {
    logi!("on_remote_request, calling function: {}", code);
    let input_map = deserialize_map(data).map_err(|_| IpcStatusCode::InvalidValue)?;
    let ipc_code = IpcCode::try_from(code).map_err(|_| IpcStatusCode::InvalidValue)?;
    match ipc_code {
        IpcCode::Add => {
            logi!("on_remote_request add");
            match stub.add(&input_map) {
                Ok(_) => {
                    reply.write::<i32>(&IPC_SUCCESS)?;
                },
                Err(e) => {
                    reply.write::<i32>(&(e as i32))?;
                }
            }
        },
        IpcCode::Query => {
            logi!("on_remote_request query");
            match stub.query(&input_map) {
                Ok(res) => {
                    reply.write::<i32>(&IPC_SUCCESS)?;
                    serialize_vector_map(&res, reply).map_err(|e| {
                        loge!("serialize_vector_map failed! {}", e);
                        IpcStatusCode::InvalidValue
                    })?;
                },
                Err(e) => {
                    reply.write::<i32>(&(e as i32))?;
                }
            }
        },
        _ => {},
    }
    Ok(())
}

/// IPC stub type
pub struct AssetStub(Box<dyn IAsset + Sync + Send>);

impl AssetStub {
    /// Create a new remote stub service
    #[allow(dead_code)]
    pub fn new_remote_stub<T: IAsset + Send + Sync + 'static>(obj: T) -> Option<RemoteStub<Self>> {
        RemoteStub::new(AssetStub(Box::new(obj)))
    }
}

impl IRemoteStub for AssetStub {
    /// Get stub object descriptor.
    fn get_descriptor() -> &'static str {
        "security_asset_service"
    }

    /// Callback to deal IPC request for this stub.
    fn on_remote_request(&self, code: u32, data: &BorrowedMsgParcel, reply: &mut BorrowedMsgParcel) -> i32 {
        let result = on_remote_request(&*self.0, code, data, reply);

        match result {
            Ok(_) => 0,
            Err(error) => {
                error as i32
            }
        }
    }

    /// Callback to dump.
    fn on_dump(&self, file: &FileDesc, args: &mut Vec<String16>) -> i32 {
        self.0.dump(file, args)
    }
}
