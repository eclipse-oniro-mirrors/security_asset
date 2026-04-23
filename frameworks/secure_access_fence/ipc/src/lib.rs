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

//! This module defines IPC interfaces and constants.

use ipc::{
    parcel::{Deserialize, MsgParcel},
    IpcStatusCode,
};

use saf_definition::{macros_lib, Conversion, DataType, ErrCode, Result, SAFError, SAFMap, Tag, Value};

/// SA id for SAF service.
pub const SA_ID: i32 = 66532;
/// SA name for SAF service.
pub const SA_NAME: &str = "secure_access_fence";
/// IPC result code.
pub const IPC_SUCCESS: u32 = 0;

/// IPC code for GenerateTicketBatch.
pub const CMD_GENERATE_TICKET_BATCH: u32 = 1;
/// IPC code for VerifyTicketBatch.
pub const CMD_VERIFY_TICKET_BATCH: u32 = 2;
/// IPC code for QueryPermissionBySubCommandBatch.
pub const CMD_QUERY_PERMISSION_BATCH: u32 = 500;

const MAX_MAP_CAPACITY: u32 = 64;
const MAX_VEC_CAPACITY: u32 = 0x10000;
const MAX_TICKET_CAPACITY: u32 = 100;

macros_lib::impl_enum_trait! {
    /// Code used to identify the function to be called.
    #[derive(Clone, Copy)]
    #[derive(Eq, PartialEq)]
    pub enum IpcCode {
        /// Code for Check access, Not use.
        CheckAccess = ipc::FIRST_CALL_TRANSACTION,
    }
}

/// Ticket verify info structure (matching IDL definition).
#[derive(Debug, Clone)]
pub struct TicketVerifyInfo {
    /// Message to verify.
    pub message: String,
    /// Ticket to verify.
    pub ticket: String,
}

/// deserialize T from parcel
pub fn deserialize<T: Deserialize>(parcel: &mut MsgParcel) -> Result<T> {
    let value = parcel.read::<T>().map_err(|_| {
        macros_lib::log_and_into_saf_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL]deserialize T from parcel failed!"
        )
    })?;
    Ok(value)
}

/// serialize the map to parcel
pub fn serialize_map(map: &SAFMap, parcel: &mut MsgParcel) -> Result<()> {
    if map.len() as u32 > MAX_MAP_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]The map size exceeds the limit."
        );
    }
    parcel.write(&(map.len() as u32)).map_err(ipc_err_handle)?;
    for (&tag, value) in map.iter() {
        if tag.data_type() != value.data_type() {
            return macros_lib::log_throw_error!(
                ErrCode::ParamVerificationFailed,
                "[FATAL][IPC]Data type mismatch, key type: {}, value type: {}",
                tag.data_type(),
                value.data_type()
            );
        }
        parcel.write(&(tag as u32)).map_err(ipc_err_handle)?;
        match value {
            Value::Bool(b) => parcel.write::<bool>(b).map_err(ipc_err_handle)?,
            Value::Number(n) => parcel.write::<u32>(n).map_err(ipc_err_handle)?,
            Value::Bytes(a) => parcel.write::<Vec<u8>>(a).map_err(ipc_err_handle)?,
        }
    }
    Ok(())
}

/// deserialize the map from parcel
pub fn deserialize_map(parcel: &mut MsgParcel) -> Result<SAFMap> {
    let len = parcel.read::<u32>().map_err(ipc_err_handle)?;
    if len > MAX_MAP_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]The map size exceeds the limit."
        );
    }
    let mut map = SAFMap::with_capacity(len as usize);
    for _ in 0..len {
        let tag = parcel.read::<u32>().map_err(ipc_err_handle)?;
        let tag = Tag::try_from(tag)?;
        match tag.data_type() {
            DataType::Bool => {
                let v = parcel.read::<bool>().map_err(ipc_err_handle)?;
                map.insert(tag, Value::Bool(v));
            },
            DataType::Number => {
                let v = parcel.read::<u32>().map_err(ipc_err_handle)?;
                map.insert(tag, Value::Number(v));
            },
            DataType::Bytes => {
                let v = parcel.read::<Vec<u8>>().map_err(ipc_err_handle)?;
                map.insert(tag, Value::Bytes(v));
            },
        }
    }
    Ok(map)
}

/// Serialize the collection of map to parcel.
pub fn serialize_maps(vec: &Vec<SAFMap>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]The vector size exceeds the limit."
        );
    }
    parcel.write::<u32>(&(vec.len() as u32)).map_err(ipc_err_handle)?;
    for map in vec.iter() {
        serialize_map(map, parcel)?;
    }
    Ok(())
}

/// Deserialize the collection of map from parcel.
pub fn deserialize_maps(parcel: &mut MsgParcel) -> Result<Vec<SAFMap>> {
    let len = parcel.read::<u32>().map_err(ipc_err_handle)?;
    if len > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]The vector size exceeds the limit."
        );
    }
    let mut res_vec = Vec::with_capacity(len as usize);
    for _i in 0..len {
        res_vec.push(deserialize_map(parcel)?);
    }
    Ok(res_vec)
}

/// Convert ipc error into SAF error.
pub fn ipc_err_handle(e: IpcStatusCode) -> SAFError {
    match e {
        IpcStatusCode::ServiceDied => {
            SAFError::new(ErrCode::ServiceUnavailable, format!("[FATAL][IPC]Ipc status code = {}", e as i32))
        },
        _ => SAFError::new(ErrCode::IpcError, format!("[FATAL][IPC]Ipc status code = {}", e)),
    }
}

/// Deserialize GenerateTicketBatch request parameters from MsgParcel.
pub fn deserialize_generate_ticket_request(parcel: &mut MsgParcel) -> Result<(u32, String, Vec<String>)> {
    let os_account_id = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let caller_id = parcel.read::<String>().map_err(ipc_err_handle)?;
    let messages = deserialize_string_vec(parcel)?;
    Ok((os_account_id, caller_id, messages))
}

/// Deserialize VerifyTicketBatch request parameters from MsgParcel.
pub fn deserialize_verify_ticket_request(
    parcel: &mut MsgParcel,
) -> Result<(u32, String, Vec<TicketVerifyInfo>, String)> {
    let os_account_id = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let caller_id = parcel.read::<String>().map_err(ipc_err_handle)?;
    let verify_infos = deserialize_ticket_verify_infos(parcel)?;
    let challenge = parcel.read::<String>().map_err(ipc_err_handle)?;
    Ok((os_account_id, caller_id, verify_infos, challenge))
}

/// Deserialize vector of strings from MsgParcel.
pub fn deserialize_string_vec(parcel: &mut MsgParcel) -> Result<Vec<String>> {
    let len = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if len < 0 || len as u32 > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]String vector size invalid: {}",
            len
        );
    }
    let mut vec = Vec::with_capacity(len as usize);
    for _ in 0..len {
        vec.push(parcel.read::<String>().map_err(ipc_err_handle)?);
    }
    Ok(vec)
}

/// Deserialize TicketVerifyInfo from MsgParcel.
pub fn deserialize_ticket_verify_info(parcel: &mut MsgParcel) -> Result<TicketVerifyInfo> {
    let message = parcel.read::<String>().map_err(ipc_err_handle)?;
    let ticket = parcel.read::<String>().map_err(ipc_err_handle)?;
    Ok(TicketVerifyInfo { message, ticket })
}

/// Deserialize vector of TicketVerifyInfo from MsgParcel.
pub fn deserialize_ticket_verify_infos(parcel: &mut MsgParcel) -> Result<Vec<TicketVerifyInfo>> {
    let len = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if len < 0 || len as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]TicketVerifyInfo vector size invalid: {}",
            len
        );
    }
    let mut vec = Vec::with_capacity(len as usize);
    for _ in 0..len {
        vec.push(deserialize_ticket_verify_info(parcel)?);
    }
    Ok(vec)
}

/// Serialize vector of strings to MsgParcel (for reply).
pub fn serialize_string_vec(vec: &Vec<String>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]String vector size exceeds limit: {}",
            vec.len()
        );
    }
    parcel.write::<i32>(&(vec.len() as i32)).map_err(ipc_err_handle)?;
    for s in vec {
        parcel.write::<String>(s).map_err(ipc_err_handle)?;
    }
    Ok(())
}

/// Serialize vector of i32 to MsgParcel (for reply).
pub fn serialize_i32_vec(vec: &Vec<i32>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_TICKET_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[FATAL][IPC]i32 vector size exceeds limit: {}",
            vec.len()
        );
    }
    parcel.write::<i32>(&(vec.len() as i32)).map_err(ipc_err_handle)?;
    for val in vec {
        parcel.write::<i32>(val).map_err(ipc_err_handle)?;
    }
    Ok(())
}
