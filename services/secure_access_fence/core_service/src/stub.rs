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

const DEFAULT_DOMAIN_ID: &str = "";

use ipc::{parcel::MsgParcel, remote::RemoteStub, IpcResult, IpcStatusCode, Skeleton};
use saf_common::{get_user_id, AutoCounter, Counter};

use saf_ipc::{
    deserialize_batch_generate_ticket_request, deserialize_batch_verify_ticket_request, deserialize_permission_queries,
    deserialize_remote_auth_packages, deserialize_remote_info, deserialize_remote_user_auth_results_vec,
    deserialize_task_identity_info, deserialize_verify_ticket_request, serialize_bool_vec, serialize_cli_infos,
    serialize_i32_vec, serialize_remote_auth_packages, serialize_task_identity, serialize_u8_to_i32_list,
    serialize_verify_ticket_infos, CMD_BATCH_GENERATE_TICKET, CMD_BATCH_VERIFY_TICKET,
    CMD_GENERATE_CONTROLLED_DEVICE_PACKAGE, CMD_GENERATE_CONTROLLER_DEVICE_PACKAGE, CMD_GET_REMOTE_GRANT_STATUS,
    CMD_GENERATE_TASK_CHALLENGE, CMD_GENERATE_TASK_IDENTITY, CMD_QUERY_STATIC_CLI_CMD_PERMISSIONS,
    CMD_REQUEST_TOOL_TICKET, CMD_RETRIEVE_AUTH_TOKEN_BY_TASK_ID, CMD_REVOKE_TASK_IDENTITY,
    CMD_UPDATE_REMOTE_GRANT_STATUS, CMD_VERIFY_CONTROLLED_DEVICE_PACKAGE, CMD_VERIFY_CONTROLLER_DEVICE_PACKAGE,
    CMD_VERIFY_TICKET, IPC_SUCCESS, SA_NAME,
};
use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;
use saf_sdk::{ErrCode, Result, SAFError};

use crate::critical_manager::CriticalManager;
use crate::remote_grant_status;
use crate::wrapper;
use crate::SAFService;
use crate::task_identity::validator;

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
        CriticalManager::get_instance().set_critical(true);

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
    match data.read_interface_token() {
        Ok(interface_token) if interface_token == stub.descriptor() => {},
        _ => {
            loge!("[FATAL][SA]Invalid interface token.");
            return Err(IpcStatusCode::Failed);
        },
    }

    match code {
        CMD_BATCH_GENERATE_TICKET => handle_batch_generate_ticket(stub, data, reply),
        CMD_BATCH_VERIFY_TICKET => handle_batch_verify_ticket(stub, data, reply),
        CMD_VERIFY_TICKET => handle_verify_ticket(stub, data, reply),
        CMD_GENERATE_CONTROLLED_DEVICE_PACKAGE => handle_generate_controlled_device_package(stub, data, reply),
        CMD_VERIFY_CONTROLLED_DEVICE_PACKAGE => handle_verify_controlled_device_package(stub, data, reply),
        CMD_GENERATE_CONTROLLER_DEVICE_PACKAGE => handle_generate_controller_device_package(stub, data, reply),
        CMD_VERIFY_CONTROLLER_DEVICE_PACKAGE => handle_verify_controller_device_package(stub, data, reply),
        CMD_GET_REMOTE_GRANT_STATUS => handle_get_remote_grant_status(data, reply),
        CMD_UPDATE_REMOTE_GRANT_STATUS => handle_update_remote_grant_status(data, reply),
        CMD_REQUEST_TOOL_TICKET => handle_request_tool_ticket(stub, data, reply),
        CMD_QUERY_STATIC_CLI_CMD_PERMISSIONS => handle_query_static_cli_cmd_permissions(stub, data, reply),
        CMD_GENERATE_TASK_CHALLENGE => {
            handle_generate_task_challenge(stub, data, reply)
        }
        CMD_GENERATE_TASK_IDENTITY => {
            handle_generate_task_identity(stub, data, reply)
        }
        CMD_REVOKE_TASK_IDENTITY => {
            handle_revoke_task_identity(stub, data, reply)
        }
        CMD_RETRIEVE_AUTH_TOKEN_BY_TASK_ID => {
            handle_retrieve_auth_token_by_task_id(stub, data, reply)
        }
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

fn get_calling_os_account_id() -> IpcResult<i32> {
    let uid = Skeleton::calling_uid();
    get_user_id(uid).map_err(|e| {
        loge!("[FATAL]Get user id failed: {}", e.msg);
        IpcStatusCode::Failed
    })
}

fn handle_batch_generate_ticket(stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let (os_account_id, caller_id, messages) = deserialize_batch_generate_ticket_request(data).map_err(|e| {
        loge!("[FATAL]Deserialize batch generate ticket request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    logi!(
        "BatchGenerateTicket received, osAccountId: {}, callerId: {}, messageCount: {}",
        os_account_id,
        caller_id,
        messages.len()
    );

    let result = stub.batch_generate_ticket(os_account_id as i32, &caller_id, DEFAULT_DOMAIN_ID, &messages);
    match result {
        Ok(ticket_infos) => {
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            serialize_verify_ticket_infos(&ticket_infos, reply).map_err(|e| {
                loge!("[FATAL]Serialize ticket infos failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("BatchGenerateTicket success, ticketCount: {}", ticket_infos.len());
        },
        Err(e) => {
            loge!("[FATAL]BatchGenerateTicket failed: {}", e.msg);
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            let empty_infos: Vec<saf_definition::VerifyTicketInfo> = vec![];
            serialize_verify_ticket_infos(&empty_infos, reply).map_err(|e| {
                loge!("[FATAL]Serialize empty ticket infos failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_batch_verify_ticket(stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let (os_account_id, caller_id, verify_infos) = deserialize_batch_verify_ticket_request(data).map_err(|e| {
        loge!("[FATAL]Deserialize batch verify ticket request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    logi!(
        "BatchVerifyTicket received, osAccountId: {}, callerId: {}, verifyInfoCount: {}",
        os_account_id,
        caller_id,
        verify_infos.len()
    );

    let result = stub.batch_verify_ticket(os_account_id as i32, &caller_id, DEFAULT_DOMAIN_ID, &verify_infos);
    match result {
        Ok(verify_res) => {
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            serialize_i32_vec(&verify_res, reply).map_err(|e| {
                loge!("[FATAL]Serialize verify results failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("BatchVerifyTicket success, resultCount: {}", verify_res.len());
        },
        Err(e) => {
            loge!("[FATAL]BatchVerifyTicket failed: {}", e.msg);
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            let empty_res: Vec<i32> = vec![];
            serialize_i32_vec(&empty_res, reply).map_err(|e| {
                loge!("[FATAL]Serialize empty verify results failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_verify_ticket(stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let (os_account_id, caller_id, verify_info_str) = deserialize_verify_ticket_request(data).map_err(|e| {
        loge!("[FATAL]Deserialize verify ticket request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    logi!("VerifyTicket received, osAccountId: {}, callerId: {}", os_account_id, caller_id);

    let result = stub.verify_ticket(os_account_id as i32, &caller_id, &verify_info_str);
    match result {
        Ok(cli_infos) => {
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            serialize_cli_infos(&cli_infos, reply).map_err(|e| {
                loge!("[FATAL]Serialize cli infos failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("VerifyTicket success, cliInfoCount: {}", cli_infos.len());
        },
        Err(e) => {
            loge!("[FATAL]VerifyTicket failed: {}", e.msg);
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            let empty_infos: Vec<saf_definition::CliInfo> = vec![];
            serialize_cli_infos(&empty_infos, reply).map_err(|e| {
                loge!("[FATAL]Serialize empty cli infos failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_generate_controlled_device_package(
    stub: &SAFService,
    data: &mut MsgParcel,
    reply: &mut MsgParcel,
) -> IpcResult<()> {
    let queries = deserialize_permission_queries(data).map_err(|e| {
        loge!("[FATAL]Deserialize GenerateControlledDevicePackage request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    let os_account_id = get_calling_os_account_id()?;

    logi!("GenerateControlledDevicePackage received, os_account_id: {}, queryCount: {}", os_account_id, queries.len());

    let result = stub.generate_controlled_device_package(os_account_id, queries);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    serialize_remote_auth_packages(&result.packages, reply).map_err(|e| {
        loge!("[FATAL]Serialize packages failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;
    reply.write::<i32>(&result.error_code)?;

    if result.error_code != 0 {
        loge!("GenerateControlledDevicePackage partial failure, error_code={}", result.error_code);
    } else {
        logi!("GenerateControlledDevicePackage success, packageCount={}", result.packages.len());
    }

    Ok(())
}

fn handle_verify_controlled_device_package(
    stub: &SAFService,
    data: &mut MsgParcel,
    reply: &mut MsgParcel,
) -> IpcResult<()> {
    let packages = deserialize_remote_auth_packages(data).map_err(|e| {
        loge!("[FATAL]Deserialize VerifyControlledDevicePackage request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    let os_account_id = get_calling_os_account_id()?;

    logi!("VerifyControlledDevicePackage received, os_account_id: {}, packageCount: {}", os_account_id, packages.len());

    let result = stub.verify_controlled_device_package(os_account_id, packages);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    serialize_bool_vec(&result.results, reply).map_err(|e| {
        loge!("[FATAL]Serialize verify results failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;
    reply.write::<i32>(&result.error_code)?;

    if result.error_code != 0 {
        loge!("VerifyControlledDevicePackage system error, error_code={}", result.error_code);
    } else {
        logi!("VerifyControlledDevicePackage success, resultCount: {}", result.results.len());
    }

    Ok(())
}

fn handle_generate_controller_device_package(
    stub: &SAFService,
    data: &mut MsgParcel,
    reply: &mut MsgParcel,
) -> IpcResult<()> {
    let remote_user_auth_results = deserialize_remote_user_auth_results_vec(data).map_err(|e| {
        loge!("[FATAL]Deserialize GenerateControllerDevicePackage request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    let os_account_id = get_calling_os_account_id()?;

    logi!(
        "GenerateControllerDevicePackage received, os_account_id: {}, resultCount: {}",
        os_account_id,
        remote_user_auth_results.len()
    );

    let result = stub.generate_controller_device_package(os_account_id, remote_user_auth_results);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    serialize_remote_auth_packages(&result.packages, reply).map_err(|e| {
        loge!("[FATAL]Serialize packages failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;
    reply.write::<i32>(&result.error_code)?;

    if result.error_code != 0 {
        loge!("GenerateControllerDevicePackage partial failure, error_code={}", result.error_code);
    } else {
        logi!("GenerateControllerDevicePackage success, packageCount={}", result.packages.len());
    }

    Ok(())
}

fn handle_verify_controller_device_package(
    stub: &SAFService,
    data: &mut MsgParcel,
    reply: &mut MsgParcel,
) -> IpcResult<()> {
    let packages = deserialize_remote_auth_packages(data).map_err(|e| {
        loge!("[FATAL]Deserialize VerifyControllerDevicePackage packages failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    let remote_info = deserialize_remote_info(data).map_err(|e| {
        loge!("[FATAL]Deserialize VerifyControllerDevicePackage remote_info failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    let os_account_id = get_calling_os_account_id()?;

    logi!("VerifyControllerDevicePackage received, os_account_id: {}, packageCount: {}", os_account_id, packages.len());

    let result = stub.verify_controller_device_package(os_account_id, packages, &remote_info);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    serialize_bool_vec(&result.results, reply).map_err(|e| {
        loge!("[FATAL]Serialize verify results failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;
    reply.write::<i32>(&result.error_code)?;

    if result.error_code != 0 {
        loge!("VerifyControllerDevicePackage system error, error_code={}", result.error_code);
    } else {
        logi!("VerifyControllerDevicePackage success, resultCount: {}", result.results.len());
    }

    Ok(())
}

fn handle_get_remote_grant_status(_data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    logi!("GetRemoteGrantStatus received");

    let result = remote_grant_status::get_remote_grant_status();

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;

    match result {
        Ok(status) => {
            reply.write::<i32>(&status)?;
            reply.write::<i32>(&0)?;
            logi!("GetRemoteGrantStatus success, status={}", status);
        },
        Err(e) => {
            reply.write::<i32>(&0)?;
            reply.write::<i32>(&(e.code as i32))?;
            loge!("GetRemoteGrantStatus failed: {}", e.msg);
        },
    }

    Ok(())
}

fn handle_update_remote_grant_status(data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let status = data.read::<i32>().map_err(|e| {
        loge!("[FATAL]Read status failed: {:?}", e);
        IpcStatusCode::Failed
    })?;

    logi!("UpdateRemoteGrantStatus received, status={}", status);

    let result = remote_grant_status::update_remote_grant_status(status);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;

    match result {
        Ok(()) => {
            reply.write::<i32>(&0)?;
            logi!("UpdateRemoteGrantStatus success");
        },
        Err(e) => {
            reply.write::<i32>(&(e.code as i32))?;
            loge!("UpdateRemoteGrantStatus failed: {}", e.msg);
        },
    }

    Ok(())
}

fn deserialize_request_tool_ticket_request(
    data: &mut MsgParcel,
) -> IpcResult<(Vec<wrapper::ffi::CxxCommandInfo>, u64)> {
    let count = data.read::<i32>().map_err(|e| {
        loge!("[FATAL]RequestToolTicket: read cliCmdInfos count failed: {:?}", e);
        IpcStatusCode::Failed
    })?;
    if count < 0 || count as usize > crate::MAX_CLI_CMD_INFOS_COUNT {
        loge!("[FATAL]RequestToolTicket: invalid cliCmdInfos count: {}", count);
        return Err(IpcStatusCode::Failed);
    }

    let mut cli_cmd_infos: Vec<wrapper::ffi::CxxCommandInfo> = Vec::with_capacity(count as usize);
    for i in 0..count {
        let cmd_name = data.read_string16().map_err(|e| {
            loge!("[FATAL]RequestToolTicket: read cmdName[{}] failed: {:?}", i, e);
            IpcStatusCode::Failed
        })?;
        let sub_cmd = data.read_string16().map_err(|e| {
            loge!("[FATAL]RequestToolTicket: read subCmd[{}] failed: {:?}", i, e);
            IpcStatusCode::Failed
        })?;
        cli_cmd_infos.push(wrapper::ffi::CxxCommandInfo { cmd_name, sub_cmd });
    }

    let ticket_expire_time_ms_raw = data.read::<i32>().map_err(|e| {
        loge!("[FATAL]RequestToolTicket: read ticketExpireTimeMs failed: {:?}", e);
        IpcStatusCode::Failed
    })?;
    if ticket_expire_time_ms_raw < 0 {
        loge!("[FATAL]RequestToolTicket: ticketExpireTimeMs is negative: {}", ticket_expire_time_ms_raw);
        return Err(IpcStatusCode::Failed);
    }
    let ticket_expire_time_ms = ticket_expire_time_ms_raw as u64;

    Ok((cli_cmd_infos, ticket_expire_time_ms))
}

fn handle_request_tool_ticket(stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel) -> IpcResult<()> {
    let (cli_cmd_infos, ticket_expire_time_ms) = deserialize_request_tool_ticket_request(data)?;

    logi!(
        "RequestToolTicket received, cmdCount: {}, ticketExpireTimeMs: {}",
        cli_cmd_infos.len(),
        ticket_expire_time_ms
    );

    let os_account_id = get_calling_os_account_id()?;
    let result = stub.request_tool_ticket(os_account_id, cli_cmd_infos, ticket_expire_time_ms);

    match result {
        Ok(ticket_result) => {
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            let perm_count = ticket_result.permission_results.len() as i32;
            reply.write::<i32>(&perm_count)?;
            for perm in &ticket_result.permission_results {
                reply.write_string16(&perm.permission)?;
                reply.write::<i32>(&perm.permission_status)?;
                reply.write::<i32>(&perm.auth_status)?;
                reply.write::<i32>(&perm.flag)?;
            }
            reply.write_string16(&ticket_result.ticket)?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("RequestToolTicket success, permCount: {}, hasTicket: {}", perm_count, ticket_result.has_ticket);
        },
        Err(e) => {
            loge!("[FATAL]RequestToolTicket failed: {}", e.msg);
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            reply.write::<i32>(&0)?;
            reply.write_string16("")?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn deserialize_query_static_cli_cmd_permissions_request(
    data: &mut MsgParcel,
) -> IpcResult<(Vec<wrapper::ffi::CxxCommandInfo>, u32)> {
    let count = data.read::<i32>().map_err(|e| {
        loge!("[FATAL]QueryStaticCliCmdPermissions: read cmds count failed: {:?}", e);
        IpcStatusCode::Failed
    })?;
    if count < 0 || count as usize > crate::MAX_CLI_CMD_INFOS_COUNT {
        loge!("[FATAL]QueryStaticCliCmdPermissions: invalid cmds count: {}", count);
        return Err(IpcStatusCode::Failed);
    }

    let mut cmds: Vec<wrapper::ffi::CxxCommandInfo> = Vec::with_capacity(count as usize);
    for i in 0..count {
        let cmd_name = data.read_string16().map_err(|e| {
            loge!("[FATAL]QueryStaticCliCmdPermissions: read cmdName[{}] failed: {:?}", i, e);
            IpcStatusCode::Failed
        })?;
        let sub_cmd = data.read_string16().map_err(|e| {
            loge!("[FATAL]QueryStaticCliCmdPermissions: read subCmd[{}] failed: {:?}", i, e);
            IpcStatusCode::Failed
        })?;
        cmds.push(wrapper::ffi::CxxCommandInfo { cmd_name, sub_cmd });
    }

    let caller_token_id_raw = data.read::<i32>().map_err(|e| {
        loge!("[FATAL]QueryStaticCliCmdPermissions: read callerTokenId failed: {:?}", e);
        IpcStatusCode::Failed
    })?;
    if caller_token_id_raw <= 0 {
        loge!("[FATAL]QueryStaticCliCmdPermissions: callerTokenId is invalid: {}", caller_token_id_raw);
        return Err(IpcStatusCode::Failed);
    }
    let caller_token_id = caller_token_id_raw as u32;

    Ok((cmds, caller_token_id))
}

fn handle_query_static_cli_cmd_permissions(
    stub: &SAFService,
    data: &mut MsgParcel,
    reply: &mut MsgParcel,
) -> IpcResult<()> {
    let (cmds, caller_token_id) = deserialize_query_static_cli_cmd_permissions_request(data)?;

    logi!("QueryStaticCliCmdPermissions received, cmdCount: {}, callerTokenId: {}", cmds.len(), caller_token_id);

    let os_account_id = get_calling_os_account_id()?;
    let result = stub.query_static_cli_cmd_permissions(os_account_id, cmds, caller_token_id);

    match result {
        Ok(cmd_permissions) => {
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            let count = cmd_permissions.len() as i32;
            reply.write::<i32>(&count)?;
            for perm in &cmd_permissions {
                reply.write_string16(&perm.cmd_name)?;
                reply.write_string16(&perm.sub_cmd)?;
                let perm_count = perm.permissions.len() as i32;
                reply.write::<i32>(&perm_count)?;
                for p in &perm.permissions {
                    reply.write_string16(p)?;
                }
                reply.write::<i32>(&perm.query_ret)?;
            }
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("QueryStaticCliCmdPermissions success, count: {}", count);
        },
        Err(e) => {
            loge!("[FATAL]QueryStaticCliCmdPermissions failed: {}", e.msg);
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            reply.write::<i32>(&0)?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_generate_task_challenge(
    stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel
) -> IpcResult<()> {
    let info = deserialize_task_identity_info(data).map_err(|e| {
        loge!("[FATAL]Deserialize GenerateTaskChallenge request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    if let Err(e) = validator::validate_task_id_info(&info) {
        loge!("[FATAL]GenerateTaskChallenge validation failed: {}", e.msg);
        reply.write::<i32>(&(IPC_SUCCESS as i32))?;
        let empty: Vec<u8> = vec![];
        serialize_u8_to_i32_list(&empty, reply).map_err(|_| IpcStatusCode::Failed)?;
        reply.write::<i32>(&(e.code as i32))?;
        return Ok(());
    }

    let caller_token = crate::wrapper::ffi::GetCallingTokenId();
    let os_account_id = crate::wrapper::ffi::GetOsAccountIdFromTokenId(caller_token);

    logi!("GenerateTaskChallenge received, os_account_id: {}, task_id_len: {}",
        os_account_id, info.task_id.len());

    let result = stub.generate_task_challenge(os_account_id, info);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    match result {
        Ok(challenge) => {
            serialize_u8_to_i32_list(&challenge, reply).map_err(|e| {
                loge!("[FATAL]Serialize challenge failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("GenerateTaskChallenge success, challenge_len: {}", challenge.len());
        },
        Err(e) => {
            loge!("[FATAL]GenerateTaskChallenge failed: {}", e.msg);
            let empty: Vec<u8> = vec![];
            serialize_u8_to_i32_list(&empty, reply).map_err(|err| {
                loge!("[FATAL]Serialize empty challenge failed: {}", err.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_generate_task_identity(
    stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel
) -> IpcResult<()> {
    let info = deserialize_task_identity_info(data).map_err(|e| {
        loge!("[FATAL]Deserialize GenerateTaskIdentity request failed: {}", e.msg);
        IpcStatusCode::Failed
    })?;

    if let Err(e) = validator::validate_task_id_info(&info) {
        loge!("[FATAL]GenerateTaskIdentity validation failed: {}", e.msg);
        reply.write::<i32>(&(IPC_SUCCESS as i32))?;
        let empty = saf_definition::TaskIdentity::default();
        serialize_task_identity(&empty, reply).map_err(|_| IpcStatusCode::Failed)?;
        reply.write::<i32>(&(e.code as i32))?;
        return Ok(());
    }

    let caller_token = crate::wrapper::ffi::GetCallingTokenId();
    let os_account_id = crate::wrapper::ffi::GetOsAccountIdFromTokenId(caller_token);

    logi!("GenerateTaskIdentity received, os_account_id: {}, task_id_len: {}",
        os_account_id, info.task_id.len());

    let result = stub.generate_task_identity(os_account_id, info);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    match result {
        Ok(identity) => {
            serialize_task_identity(&identity, reply).map_err(|e| {
                loge!("[FATAL]Serialize task identity failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("GenerateTaskIdentity success");
        },
        Err(e) => {
            loge!("[FATAL]GenerateTaskIdentity failed: {}", e.msg);
            let empty = saf_definition::TaskIdentity::default();
            serialize_task_identity(&empty, reply).map_err(|err| {
                loge!("[FATAL]Serialize empty task identity failed: {}", err.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_revoke_task_identity(
    stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel
) -> IpcResult<()> {
    let task_ids = data.read_string16_vec().map_err(|e| {
        loge!("[FATAL]Deserialize RevokeTaskIdentity request failed: {:?}", e);
        IpcStatusCode::Failed
    })?;

    if let Err(e) = validator::validate_revoke_params(&task_ids) {
        loge!("[FATAL]RevokeTaskIdentity validation failed: {}", e.msg);
        reply.write::<i32>(&(IPC_SUCCESS as i32))?;
        reply.write::<i32>(&(e.code as i32))?;
        return Ok(());
    }

    let caller_token = crate::wrapper::ffi::GetCallingTokenId();
    let os_account_id = crate::wrapper::ffi::GetOsAccountIdFromTokenId(caller_token);

    logi!("RevokeTaskIdentity received, os_account_id: {}, task_count: {}",
        os_account_id, task_ids.len());

    let result = stub.revoke_task_identity(os_account_id, task_ids);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    match result {
        Ok(()) => {
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("RevokeTaskIdentity success");
        },
        Err(e) => {
            loge!("[FATAL]RevokeTaskIdentity failed: {}", e.msg);
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn handle_retrieve_auth_token_by_task_id(
    stub: &SAFService, data: &mut MsgParcel, reply: &mut MsgParcel
) -> IpcResult<()> {
    let caller_token = data.read::<i32>().map_err(|e| {
        loge!("[FATAL]Deserialize RetrieveAuthTokenByTaskId callerToken failed: {:?}", e);
        IpcStatusCode::Failed
    })?;
    let task_id = data.read_string16().map_err(|e| {
        loge!("[FATAL]Deserialize RetrieveAuthTokenByTaskId taskId failed: {:?}", e);
        IpcStatusCode::Failed
    })?;

    if caller_token <= 0 {
        loge!("[FATAL]RetrieveAuthTokenByTaskId invalid caller_token: {}", caller_token);
        reply.write::<i32>(&(IPC_SUCCESS as i32))?;
        let empty: Vec<u8> = vec![];
        serialize_u8_to_i32_list(&empty, reply).map_err(|_| IpcStatusCode::Failed)?;
        let code = saf_sdk::ErrCode::InvalidArgument as i32;
        reply.write::<i32>(&code)?;
        return Ok(());
    }
    if let Err(e) = validator::validate_path_segment(&task_id) {
        loge!("[FATAL]RetrieveAuthTokenByTaskId validation failed: {}", e.msg);
        reply.write::<i32>(&(IPC_SUCCESS as i32))?;
        let empty: Vec<u8> = vec![];
        serialize_u8_to_i32_list(&empty, reply).map_err(|_| IpcStatusCode::Failed)?;
        reply.write::<i32>(&(e.code as i32))?;
        return Ok(());
    }

    // RetrieveAuthTokenByTaskId resolves os_account_id from callerToken parameter.
    let os_account_id = crate::wrapper::ffi::GetOsAccountIdFromTokenId(caller_token as u32);

    logi!("RetrieveAuthTokenByTaskId received, os_account_id: {}, caller_token: {}, task_id_len: {}",
        os_account_id, caller_token, task_id.len());

    let result = stub.retrieve_auth_token_by_task_id(os_account_id, caller_token, task_id);

    reply.write::<i32>(&(IPC_SUCCESS as i32))?;
    match result {
        Ok(auth_token) => {
            serialize_u8_to_i32_list(&auth_token, reply).map_err(|e| {
                loge!("[FATAL]Serialize authToken failed: {}", e.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(IPC_SUCCESS as i32))?;
            logi!("RetrieveAuthTokenByTaskId success, auth_token_len: {}", auth_token.len());
        },
        Err(e) => {
            loge!("[FATAL]RetrieveAuthTokenByTaskId failed: {}", e.msg);
            let empty: Vec<u8> = vec![];
            serialize_u8_to_i32_list(&empty, reply).map_err(|err| {
                loge!("[FATAL]Serialize empty authToken failed: {}", err.msg);
                IpcStatusCode::Failed
            })?;
            reply.write::<i32>(&(e.code as i32))?;
        },
    }
    Ok(())
}

fn on_extension_request(_stub: &SAFService, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
    if let Ok(load) = SAFPlugin::get_instance().load_plugin() {
        match load.on_remote_request(code, data, reply) {
            Ok(res) => {
                logi!("process redirect request success.");
                return res;
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
