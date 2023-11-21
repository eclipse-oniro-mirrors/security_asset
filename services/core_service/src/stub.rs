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

//! This module implements the stub of the Asset service.

use ipc_rust::{BorrowedMsgParcel, FileDesc, IRemoteStub, IpcResult, IpcStatusCode, RemoteStub, String16};

use asset_definition::{AssetError, Result};
use asset_ipc::{deserialize_map, serialize_maps, IAsset, IpcCode, IPC_SUCCESS, SA_NAME};
use asset_log::loge;

/// IPC stub type.
pub struct AssetStub(Box<dyn IAsset + Sync + Send>);

impl AssetStub {
    /// Create a new remote stub service.
    pub fn new_remote_stub<T: IAsset + Send + Sync + 'static>(obj: T) -> Option<RemoteStub<Self>> {
        RemoteStub::new(AssetStub(Box::new(obj)))
    }
}

impl IRemoteStub for AssetStub {
    /// Get stub object descriptor.
    fn get_descriptor() -> &'static str {
        SA_NAME
    }

    /// Callback to deal IPC request for this stub.
    fn on_remote_request(&self, code: u32, data: &BorrowedMsgParcel, reply: &mut BorrowedMsgParcel) -> i32 {
        match on_remote_request(&*self.0, code, data, reply) {
            Ok(_) => IPC_SUCCESS as i32,
            Err(e) => e as i32,
        }
    }

    /// Callback to dump.
    fn on_dump(&self, file: &FileDesc, args: &mut Vec<String16>) -> i32 {
        self.0.dump(file, args)
    }
}

fn asset_err_handle(e: AssetError) -> IpcStatusCode {
    loge!("[IPC]Asset error code = {}, msg is {}", e.code, e.msg);
    IpcStatusCode::InvalidValue
}

fn reply_handle(ret: Result<()>, reply: &mut BorrowedMsgParcel) -> IpcResult<()> {
    match ret {
        Ok(_) => reply.write::<u32>(&IPC_SUCCESS),
        Err(e) => {
            reply.write::<u32>(&(e.code as u32))?;
            reply.write::<String>(&e.msg)
        },
    }
}

fn on_remote_request(
    stub: &dyn IAsset,
    code: u32,
    data: &BorrowedMsgParcel,
    reply: &mut BorrowedMsgParcel,
) -> IpcResult<()> {
    let ipc_code = IpcCode::try_from(code).map_err(asset_err_handle)?;
    let map = deserialize_map(data).map_err(asset_err_handle)?;
    match ipc_code {
        IpcCode::Add => reply_handle(stub.add(&map), reply),
        IpcCode::Remove => reply_handle(stub.remove(&map), reply),
        IpcCode::Update => {
            let update_map = deserialize_map(data).map_err(asset_err_handle)?;
            reply_handle(stub.update(&map, &update_map), reply)
        },
        IpcCode::PreQuery => match stub.pre_query(&map) {
            Ok(res) => {
                reply_handle(Ok(()), reply)?;
                reply.write::<Vec<u8>>(&res)
            },
            Err(e) => reply_handle(Err(e), reply),
        },
        IpcCode::Query => match stub.query(&map) {
            Ok(res) => {
                reply_handle(Ok(()), reply)?;
                serialize_maps(&res, reply).map_err(asset_err_handle)
            },
            Err(e) => reply_handle(Err(e), reply),
        },
        IpcCode::PostQuery => reply_handle(stub.post_query(&map), reply),
    }
}
