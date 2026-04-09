/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! This module implements the stub of the SAF service.

use saf_common::{AutoCounter, Counter};
use ipc::{parcel::MsgParcel, remote::RemoteStub, IpcResult, IpcStatusCode};

use saf_ipc::{IPC_SUCCESS, SA_NAME};
use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;
use saf_sdk::{
    SAFError, ErrCode, Result,
};

use crate::SAFService;

const REDIRECT_START_CODE: u32 = 1000;

impl RemoteStub for SAFService {
    fn on_remote_request(
        &self,
        code: u32,
        data: &mut ipc::parcel::MsgParcel,
        reply: &mut ipc::parcel::MsgParcel,
    ) -> i32 {
        let counter = Counter::get_instance();
        if counter.lock().unwrap().is_stop() {
            loge!("[FATAL]Service is stop.");
            let _ = reply_handle(
                Err(SAFError { code: ErrCode::ServiceUnavailable, msg: "service stop".to_string() }),
                reply,
            );
            return IPC_SUCCESS as i32;
        }
        let _counter_user = AutoCounter::new();
        if !self.system_ability.cancel_idle() {
            loge!("[FATAL]Cancel idle failed. Service is stop.");
            let _ = reply_handle(
                Err(SAFError { code: ErrCode::ServiceUnavailable, msg: "service stop".to_string() }),
                reply,
            );
            return IPC_SUCCESS as i32;
        }

        if code >= REDIRECT_START_CODE {
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

fn on_remote_request(_stub: &SAFService, _code: u32, _data: &mut MsgParcel, _reply: &mut MsgParcel) -> IpcResult<()> {
    Err(IpcStatusCode::Failed)
}

fn on_extension_request(_stub: &SAFService, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
    if let Ok(load) = SAFPlugin::get_instance().load_plugin() {
        match load.on_remote_request(code, data, reply) {
            Ok(()) => {
                logi!("process redirect request success.");
                return IPC_SUCCESS as i32;
            },
            Err(code) => {
                loge!("process redirect request failed, code: {}", code);
                return code as i32;
            },
        }
    }
    IpcStatusCode::Failed as i32
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
