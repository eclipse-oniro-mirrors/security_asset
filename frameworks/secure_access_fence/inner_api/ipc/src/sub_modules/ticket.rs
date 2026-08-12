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

//! Ticket related serialization functions.

use ipc::parcel::MsgParcel;
use saf_definition::{macros_lib, ErrCode, Result};
use saf_definition::{VerifyTicketInfo, CliInfo};
use crate::{ipc_err_handle, MAX_TICKET_CAPACITY};

/// Deserialize VerifyTicketInfo from MsgParcel.
pub fn deserialize_verify_ticket_info(parcel: &mut MsgParcel) -> Result<VerifyTicketInfo> {
    let message = parcel.read_string16().map_err(ipc_err_handle)?;
    let challenge = parcel.read_string16().map_err(ipc_err_handle)?;
    let ticket = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(VerifyTicketInfo { message, challenge, ticket })
}

/// Deserialize vector of VerifyTicketInfo from MsgParcel.
pub fn deserialize_verify_ticket_infos(parcel: &mut MsgParcel) -> Result<Vec<VerifyTicketInfo>> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]VerifyTicketInfo vector size invalid: {}",
            count
        );
    }
    let mut vec = Vec::with_capacity(count as usize);
    for _ in 0..count {
        vec.push(deserialize_verify_ticket_info(parcel)?);
    }
    Ok(vec)
}

/// Serialize VerifyTicketInfo to MsgParcel (for reply).
pub fn serialize_verify_ticket_info(info: &VerifyTicketInfo, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write_string16(&info.message).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.challenge).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.ticket).map_err(ipc_err_handle)?;
    Ok(())
}

/// Serialize vector of VerifyTicketInfo to MsgParcel (for reply).
pub fn serialize_verify_ticket_infos(infos: &Vec<VerifyTicketInfo>, parcel: &mut MsgParcel) -> Result<()> {
    if infos.len() as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]VerifyTicketInfo vector size exceeds limit: {}",
            infos.len()
        );
    }
    let count = i32::try_from(infos.len()).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArrayLen,
            "[FATAL][IPC]VerifyTicketInfo vector length overflows i32")
    })?;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    for info in infos {
        serialize_verify_ticket_info(info, parcel)?;
    }
    Ok(())
}

/// Serialize CliInfo to MsgParcel (for reply).
pub fn serialize_cli_info(info: &CliInfo, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write_string16(&info.caller_token_id).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.cli_cmd_name).map_err(ipc_err_handle)?;
    parcel.write_string16(&info.sub_cli_cmd_name).map_err(ipc_err_handle)?;
    serialize_string_vec(&info.permission_list, parcel)?;
    Ok(())
}

/// Serialize vector of CliInfo to MsgParcel (for reply).
pub fn serialize_cli_infos(infos: &Vec<CliInfo>, parcel: &mut MsgParcel) -> Result<()> {
    write_vec_len(infos.len(), MAX_TICKET_CAPACITY as i32, ErrCode::InvalidArrayLen,
        "CliInfo", parcel)?;
    for info in infos {
        serialize_cli_info(info, parcel)?;
    }
    Ok(())
}

/// Serialize string vector to MsgParcel (for reply).
pub fn serialize_string_vec(vec: &Vec<String>, parcel: &mut MsgParcel) -> Result<()> {
    write_vec_len(vec.len(), MAX_TICKET_CAPACITY as i32, ErrCode::InvalidArrayLen,
        "string", parcel)?;
    for s in vec {
        parcel.write_string16(s).map_err(ipc_err_handle)?;
    }
    Ok(())
}

/// Write vector length to parcel with capacity check.
pub fn write_vec_len(vec_len: usize, max_capacity: i32, err_code: ErrCode, label: &str,
    parcel: &mut MsgParcel) -> Result<()> {
    if vec_len > max_capacity as usize {
        return macros_lib::log_throw_error!(
            err_code,
            "[FATAL][IPC]{} vector size exceeds limit: {}",
            label,
            vec_len
        );
    }
    let count = vec_len as i32;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    Ok(())
}