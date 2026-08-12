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

//! Remote control related serialization functions.

use ipc::parcel::MsgParcel;
use saf_definition::{
    macros_lib, ErrCode, Result,
    CommandInfo, OperationInfo, OperationType,
    RemoteControlParams, RemoteInfo, Role,
    PermissionQuery, RemoteAuthPackage,
    RemoteUserAuthItem, RemoteUserAuthResults,
};
use crate::{ipc_err_handle, MAX_TICKET_CAPACITY, MAX_VEC_CAPACITY};
use crate::remote_message_wrapper;

/// Deserialize CommandInfo from MsgParcel.
pub fn deserialize_command_info(parcel: &mut MsgParcel) -> Result<CommandInfo> {
    let cmd_name = parcel.read_string16().map_err(ipc_err_handle)?;
    let sub_cmd = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(CommandInfo { cmd_name, sub_cmd })
}

/// Serialize CommandInfo to MsgParcel.
pub fn serialize_command_info(info: &CommandInfo, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write_string16(&info.cmd_name).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.sub_cmd).map_err(ipc_err_handle)?;
    Ok(())
}

/// Deserialize OperationInfo from MsgParcel.
pub fn deserialize_operation_info(parcel: &mut MsgParcel) -> Result<OperationInfo> {
    let operation_type_val = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let operation_type = match operation_type_val {
        1 => OperationType::Cli,
        2 => OperationType::Api,
        _ => return macros_lib::log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL][IPC]Invalid operation type: {}",
            operation_type_val
        ),
    };
    let cli_cmd_info = deserialize_command_info(parcel)?;
    let permission = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(OperationInfo {
        operation_type,
        cli_cmd_info,
        permission,
    })
}

/// Deserialize OperationInfo vector from MsgParcel.
pub fn deserialize_operation_infos(parcel: &mut MsgParcel) -> Result<Vec<OperationInfo>> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]OperationInfo vector size invalid: {}",
            count
        );
    }
    let mut vec = Vec::with_capacity(count as usize);
    for _ in 0..count {
        vec.push(deserialize_operation_info(parcel)?);
    }
    Ok(vec)
}

/// Serialize OperationInfo to MsgParcel.
pub fn serialize_operation_info(info: &OperationInfo, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write::<u32>(&(info.operation_type as u32)).map_err(ipc_err_handle)?;
    serialize_command_info(&info.cli_cmd_info, parcel)?;
    parcel.write_string16(&info.permission).map_err(ipc_err_handle)?;
    Ok(())
}

/// Serialize OperationInfo vector to MsgParcel.
pub fn serialize_operation_infos(infos: &Vec<OperationInfo>, parcel: &mut MsgParcel) -> Result<()> {
    if infos.len() as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]OperationInfo vector size exceeds limit: {}",
            infos.len()
        );
    }
    let count = i32::try_from(infos.len()).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArrayLen,
            "[FATAL][IPC]OperationInfo vector length overflows i32")
    })?;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    for info in infos {
        serialize_operation_info(info, parcel)?;
    }
    Ok(())
}

/// Deserialize RemoteControlParams from MsgParcel.
pub fn deserialize_remote_control_params(parcel: &mut MsgParcel) -> Result<RemoteControlParams> {
    let challenge = parcel.read_string16().map_err(ipc_err_handle)?;
    let remote_control_ticket = parcel.read_string16().map_err(ipc_err_handle)?;
    let controlled_device_name = parcel.read_string16().map_err(ipc_err_handle)?;
    let controller_device_name = parcel.read_string16().map_err(ipc_err_handle)?;
    let sign_verify_msg = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(RemoteControlParams {
        challenge,
        remote_control_ticket,
        controlled_device_name,
        controller_device_name,
        sign_verify_msg,
    })
}

/// Serialize RemoteControlParams to MsgParcel.
pub fn serialize_remote_control_params(
    params: &RemoteControlParams,
    parcel: &mut MsgParcel,
) -> Result<()> {
    parcel.write_string16(&params.challenge).map_err(ipc_err_handle)?;
    parcel.write_string16(&params.remote_control_ticket).map_err(ipc_err_handle)?;
    parcel.write_string16(&params.controlled_device_name).map_err(ipc_err_handle)?;
    parcel.write_string16(&params.controller_device_name).map_err(ipc_err_handle)?;
    parcel.write_string16(&params.sign_verify_msg).map_err(ipc_err_handle)?;
    Ok(())
}

/// Deserialize RemoteInfo from MsgParcel.
pub fn deserialize_remote_info(parcel: &mut MsgParcel) -> Result<RemoteInfo> {
    let role_val = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let role = match role_val {
        1 => Role::Controller,
        2 => Role::Controlled,
        _ => return macros_lib::log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL][IPC]Invalid role: {}",
            role_val
        ),
    };
    let remote_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let domain_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let remote_control_params = deserialize_remote_control_params(parcel)?;
    Ok(RemoteInfo {
        role,
        remote_id,
        domain_id,
        remote_control_params,
    })
}

/// Serialize RemoteInfo to MsgParcel.
pub fn serialize_remote_info(info: &RemoteInfo, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write::<u32>(&(info.role as u32)).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.remote_id).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.domain_id).map_err(ipc_err_handle)?;
    serialize_remote_control_params(&info.remote_control_params, parcel)?;
    Ok(())
}

/// Deserialize PermissionQuery from MsgParcel.
pub fn deserialize_permission_query(parcel: &mut MsgParcel) -> Result<PermissionQuery> {
    let operation_info = deserialize_operation_infos(parcel)?;
    let need_ticket = parcel.read::<bool>().map_err(ipc_err_handle)?;
    let ticket_expire_time_ms = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let caller_token_id = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let domain_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let remote_info = deserialize_remote_info(parcel)?;
    Ok(PermissionQuery {
        operation_info,
        need_ticket,
        ticket_expire_time_ms,
        caller_token_id,
        domain_id,
        remote_info,
    })
}

/// Deserialize PermissionQuery vector from MsgParcel.
pub fn deserialize_permission_queries(parcel: &mut MsgParcel) -> Result<Vec<PermissionQuery>> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]PermissionQuery vector size invalid: {}",
            count
        );
    }
    let mut vec = Vec::with_capacity(count as usize);
    for _ in 0..count {
        vec.push(deserialize_permission_query(parcel)?);
    }
    Ok(vec)
}

/// Deserialize RemoteAuthPackage from MsgParcel.
pub fn deserialize_remote_auth_package(parcel: &mut MsgParcel) -> Result<RemoteAuthPackage> {
    let remote_message_str = parcel.read_string16().map_err(ipc_err_handle)?;
    let remote_message = remote_message_wrapper::deserialize_remote_message_from_json(&remote_message_str)?;
    let challenge = parcel.read_string16().map_err(ipc_err_handle)?;
    let ticket = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(RemoteAuthPackage { remote_message, challenge, ticket })
}

/// Serialize RemoteAuthPackage to MsgParcel.
pub fn serialize_remote_auth_package(package: &RemoteAuthPackage, parcel: &mut MsgParcel) -> Result<()> {
    let remote_message_str = remote_message_wrapper::serialize_remote_message_to_json(&package.remote_message)?;
    parcel.write_string16(&remote_message_str).map_err(ipc_err_handle)?;
    parcel.write_string16(&package.challenge).map_err(ipc_err_handle)?;
    parcel.write_string16(&package.ticket).map_err(ipc_err_handle)?;
    Ok(())
}

/// Deserialize RemoteAuthPackage vector from MsgParcel.
pub fn deserialize_remote_auth_packages(parcel: &mut MsgParcel) -> Result<Vec<RemoteAuthPackage>> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]RemoteAuthPackage vector size invalid: {}",
            count
        );
    }
    let mut vec = Vec::with_capacity(count as usize);
    for _ in 0..count {
        vec.push(deserialize_remote_auth_package(parcel)?);
    }
    Ok(vec)
}

/// Serialize RemoteAuthPackage vector to MsgParcel.
pub fn serialize_remote_auth_packages(packages: &Vec<RemoteAuthPackage>, parcel: &mut MsgParcel) -> Result<()> {
    if packages.len() as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]RemoteAuthPackage vector size exceeds limit: {}",
            packages.len()
        );
    }
    let count = i32::try_from(packages.len()).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArrayLen,
            "[FATAL][IPC]RemoteAuthPackage vector length overflows i32")
    })?;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    for package in packages {
        serialize_remote_auth_package(package, parcel)?;
    }
    Ok(())
}

/// Serialize bool vector to MsgParcel (for reply).
pub fn serialize_bool_vec(vec: &Vec<bool>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]bool vector size exceeds limit: {}",
            vec.len()
        );
    }
    let count = i32::try_from(vec.len()).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArrayLen,
            "[FATAL][IPC]bool vector length overflows i32")
    })?;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    for val in vec {
        parcel.write::<bool>(val).map_err(ipc_err_handle)?;
    }
    Ok(())
}

/// Deserialize RemoteUserAuthItem from MsgParcel.
pub fn deserialize_remote_user_auth_item(parcel: &mut MsgParcel) -> Result<RemoteUserAuthItem> {
    let permission = parcel.read_string16().map_err(ipc_err_handle)?;
    let auth_result = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(RemoteUserAuthItem {
        permission,
        auth_result,
    })
}

/// Serialize RemoteUserAuthItem to MsgParcel.
pub fn serialize_remote_user_auth_item(
    item: &RemoteUserAuthItem,
    parcel: &mut MsgParcel,
) -> Result<()> {
    parcel.write_string16(&item.permission).map_err(ipc_err_handle)?;
    parcel.write_string16(&item.auth_result).map_err(ipc_err_handle)?;
    Ok(())
}

/// Deserialize RemoteUserAuthResults from MsgParcel.
pub fn deserialize_remote_user_auth_results(
    parcel: &mut MsgParcel,
) -> Result<RemoteUserAuthResults> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]RemoteUserAuthItem vector size invalid: {}",
            count
        );
    }
    let mut results = Vec::with_capacity(count as usize);
    for _ in 0..count {
        results.push(deserialize_remote_user_auth_item(parcel)?);
    }
    
    let permission_query = deserialize_permission_query(parcel)?;
    
    Ok(RemoteUserAuthResults {
        results,
        permission_query,
    })
}

/// Deserialize RemoteUserAuthResults vector from MsgParcel.
pub fn deserialize_remote_user_auth_results_vec(
    parcel: &mut MsgParcel,
) -> Result<Vec<RemoteUserAuthResults>> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]RemoteUserAuthResults vector size invalid: {}",
            count
        );
    }
    let mut vec = Vec::with_capacity(count as usize);
    for _ in 0..count {
        vec.push(deserialize_remote_user_auth_results(parcel)?);
    }
    Ok(vec)
}
