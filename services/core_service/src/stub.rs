/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use ipc::{parcel::MsgParcel, remote::RemoteStub, IpcResult, IpcStatusCode};

use asset_definition::{AssetError, Result};
use asset_ipc::{deserialize_map, serialize_maps, IpcCode, IPC_SUCCESS, SA_NAME};
use asset_log::{loge, logi};

use crate::{counter::AutoCounter, unload_handler::DELAYED_UNLOAD_TIME_IN_SEC, unload_sa, AssetService};

const UPGRADE_CODE: u32 = 18100;
const UPGRADE_TOKEN: &str = "OHOS.Updater.RestoreData";

impl RemoteStub for AssetService {
    fn on_remote_request(
        &self,
        code: u32,
        data: &mut ipc::parcel::MsgParcel,
        reply: &mut ipc::parcel::MsgParcel,
    ) -> i32 {
        let _counter_user = AutoCounter::new();
        self.system_ability.cancel_idle();
        unload_sa(DELAYED_UNLOAD_TIME_IN_SEC as u64);

        if code == UPGRADE_CODE {
            return on_extension_request(self, code, data, reply);
        }

        match on_remote_request(self, code, data, reply) {
            Ok(_) => IPC_SUCCESS as i32,
            Err(e) => e as i32,
        }
    }

    fn descriptor(&self) -> &'static str {
        SA_NAME
    }
}

fn on_remote_request(stub: &AssetService, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    match data.read_interface_token() {
        Ok(interface_token) if interface_token == stub.descriptor() => {},
        _ => {
            loge!("[FATAL][SA]Invalid interface token.");
            Err(IpcStatusCode::failed);
        }
    }
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

fn on_extension_request(stub: &AssetService, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
    match data.read_interface_token() {
        Ok(interface_token) if interface_token == UPGRADE_TOKEN => {},
        _ => {
            loge!("[FATAL][SA]Invalid interface token.");
            return IpcStatusCode::failed as i32;
        }
    }
    match data.read::<i32>() {
        Ok(user_id) => {
            logi!("[INFO]User id is {}.", user_id);
            IPC_SUCCESS as i32,
        }
        _ => IpcStatusCode::failed as i32,
    }
}

fn asset_err_handle(e: AssetError) -> IpcStatusCode {
    loge!("[IPC]Asset error code = {}, msg is {}", e.code, e.msg);
    IpcStatusCode::InvalidValue
}

fn reply_handle(ret: Result<()>, reply: &mut MsgParcel) -> IpcResult<()> {
    match ret {
        Ok(_) => reply.write::<u32>(&IPC_SUCCESS),
        Err(e) => {
            reply.write::<u32>(&(e.code as u32))?;
            reply.write::<String>(&e.msg)
        },
    }
}
