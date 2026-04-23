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

use ipc::{parcel::MsgParcel, remote::RemoteStub, IpcResult, IpcStatusCode};
use saf_common::{AutoCounter, Counter};

use saf_ipc::{
    deserialize_generate_ticket_request, deserialize_verify_ticket_request, serialize_i32_vec, serialize_ticket_infos,
    TicketInfo, VerifyTicketInfo, CMD_GENERATE_TICKET_BATCH, CMD_VERIFY_TICKET_BATCH, IPC_SUCCESS, SA_NAME,
};
use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;
use saf_sdk::{ErrCode, Result, SAFError};

use crate::wrapper;
use crate::SAFService;

const REDIRECT_START_CODE: u32 = 1000;
const C_REDIRECT_START_CODE: u32 = 500;

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

fn on_remote_request(stub: &SAFService, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    match code {
        CMD_GENERATE_TICKET_BATCH => {
            handle_generate_ticket_batch(stub, data, reply)?;
            Ok(())
        },
        CMD_VERIFY_TICKET_BATCH => {
            handle_verify_ticket_batch(stub, data, reply)?;
            Ok(())
        },
        _ => {
            if code >= C_REDIRECT_START_CODE {
                let res = wrapper::on_remote_request(code, data, reply);
                if res != 0 {
                    Err(IpcStatusCode::Failed)
                } else {
                    Ok(())
                }
            } else {
                Err(IpcStatusCode::Failed)
            }
        },
    }
}

fn handle_generate_ticket_batch(stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let (os_account_id, caller_id, messages) = deserialize_generate_ticket_request(data).map_err(|e| {
        loge!("[FATAL]Deserialize generate ticket request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    logi!(
        "[INFO]GenerateTicketBatch received, osAccountId: {}, callerId: {}, messageCount: {}",
        os_account_id,
        caller_id,
        messages.len()
    );

    let ticket_infos = generate_ticket_impl(stub, os_account_id, &caller_id, &messages).map_err(|e| {
        loge!("[FATAL]Generate ticket failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    reply.write::<i32>(&IPC_SUCCESS as &i32)?;
    serialize_ticket_infos(&ticket_infos, reply).map_err(|e| {
        loge!("[FATAL]Serialize ticket infos failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;
    reply.write::<i32>(&0)?;

    logi!("[INFO]GenerateTicketBatch success, ticketCount: {}", ticket_infos.len());
    Ok(())
}

fn handle_verify_ticket_batch(stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let (os_account_id, caller_id, verify_infos) = deserialize_verify_ticket_request(data).map_err(|e| {
        loge!("[FATAL]Deserialize verify ticket request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    logi!(
        "[INFO]VerifyTicketBatch received, osAccountId: {}, callerId: {}, verifyInfoCount: {}",
        os_account_id,
        caller_id,
        verify_infos.len()
    );

    let verify_res = verify_ticket_impl(stub, os_account_id, &caller_id, &verify_infos).map_err(|e| {
        loge!("[FATAL]Verify ticket failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    reply.write::<i32>(&IPC_SUCCESS as &i32)?;
    serialize_i32_vec(&verify_res, reply).map_err(|e| {
        loge!("[FATAL]Serialize verify results failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;
    reply.write::<i32>(&0)?;

    logi!("[INFO]VerifyTicketBatch success, resultCount: {}", verify_res.len());
    Ok(())
}

fn generate_ticket_impl(
    _stub: &SAFService,
    os_account_id: u32,
    caller_id: &str,
    messages: &Vec<String>,
) -> Result<Vec<TicketInfo>> {
    if let Ok(loader) = SAFPlugin::get_instance().load_plugin() {
        loader.generate_ticket_batch(os_account_id, caller_id, messages)
    } else {
        saf_definition::macros_lib::log_throw_error!(ErrCode::IpcError, "[FATAL]Load plugin failed for generate_ticket")
    }
}

fn verify_ticket_impl(
    _stub: &SAFService,
    os_account_id: u32,
    caller_id: &str,
    verify_infos: &Vec<VerifyTicketInfo>,
) -> Result<Vec<i32>> {
    if let Ok(loader) = SAFPlugin::get_instance().load_plugin() {
        loader.verify_ticket_batch(os_account_id, caller_id, verify_infos)
    } else {
        saf_definition::macros_lib::log_throw_error!(ErrCode::IpcError, "[FATAL]Load plugin failed for verify_ticket")
    }
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
