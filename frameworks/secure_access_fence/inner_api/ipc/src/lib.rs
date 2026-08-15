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

pub mod remote_message_wrapper;
pub mod sub_modules;

use ipc::{
    parcel::{Deserialize, MsgParcel},
    IpcStatusCode,
};

use saf_definition::{
    macros_lib, Conversion, DataType, ErrCode, Result, SAFError, SAFMap, Tag, Value, VerifyTicketInfo
};

pub use sub_modules::{
    deserialize_permission_queries,
    deserialize_remote_auth_packages,
    deserialize_remote_info,
    deserialize_remote_user_auth_results_vec,
    deserialize_verify_ticket_infos,
    serialize_bool_vec,
    serialize_cli_infos,
    serialize_remote_auth_packages,
    serialize_verify_ticket_infos,
};

// ============================================================================
// Constants
// ============================================================================

/// SA id for SAF service.
pub const SA_ID: i32 = 66532;
/// SA name for SAF service.
pub const SA_NAME: &str = "OHOS.Security.SAF.ISecureAccessFence";
/// IPC result code.
pub const IPC_SUCCESS: u32 = 0;
/// IPC code for BatchGenerateTicket.
pub const CMD_BATCH_GENERATE_TICKET: u32 = 1; 
/// IPC code for BatchVerifyTicket. 
pub const CMD_BATCH_VERIFY_TICKET: u32 = 2; 
/// IPC code for VerifyTicket. 
pub const CMD_VERIFY_TICKET: u32 = 3; 
/// IPC code for GenerateControlledDevicePackage. 
pub const CMD_GENERATE_CONTROLLED_DEVICE_PACKAGE: u32 = 4; 
/// IPC code for VerifyControlledDevicePackage. 
pub const CMD_VERIFY_CONTROLLED_DEVICE_PACKAGE: u32 = 5; 
/// IPC code for GenerateControllerDevicePackage. 
pub const CMD_GENERATE_CONTROLLER_DEVICE_PACKAGE: u32 = 6; 
/// IPC code for VerifyControllerDevicePackage. 
pub const CMD_VERIFY_CONTROLLER_DEVICE_PACKAGE: u32 = 7;

const MAX_MAP_CAPACITY: u32 = 64;
pub(crate) const MAX_VEC_CAPACITY: u32 = 0x10000;
pub(crate) const MAX_TICKET_CAPACITY: u32 = 99;

macros_lib::impl_enum_trait! {
    /// Code used to identify the function to be called.
    #[derive(Clone, Copy)]
    #[derive(Eq, PartialEq)]
    pub enum IpcCode {
        /// Code for Check access, Not use.
        CheckAccess = ipc::FIRST_CALL_TRANSACTION,
    }
}

// ============================================================================
// Error Handling
// ============================================================================

/// Convert ipc error into SAF error.
pub fn ipc_err_handle(e: IpcStatusCode) -> SAFError {
    match e {
        IpcStatusCode::ServiceDied => {
            SAFError::new(ErrCode::ServiceUnavailable, format!("[FATAL][IPC]Ipc status code = {}", e as i32))
        },
        _ => SAFError::new(ErrCode::IpcError, format!("[FATAL][IPC]Ipc status code = {}", e)),
    }
}

/// deserialize T from parcel
pub fn deserialize<T: Deserialize>(parcel: &mut MsgParcel) -> Result<T> {
    let value = parcel.read::<T>().map_err(|_| {
        macros_lib::log_and_into_saf_error!(
            ErrCode::IpcReadDataFail,
            "[FATAL]deserialize T from parcel failed!"
        )
    })?;
    Ok(value)
}

// ============================================================================
// Map Serialization
// ============================================================================

/// serialize the map to parcel
pub fn serialize_map(map: &SAFMap, parcel: &mut MsgParcel) -> Result<()> {
    if map.len() > MAX_MAP_CAPACITY as usize {
        return macros_lib::log_throw_error!(
            ErrCode::IpcWriteDataFail,
            "[FATAL][IPC]The map size exceeds the limit."
        );
    }
    parcel.write(&(map.len() as u32)).map_err(ipc_err_handle)?;
    for (&tag, value) in map.iter() {
        if tag.data_type() != value.data_type() {
            return macros_lib::log_throw_error!(
                ErrCode::IpcWriteDataFail,
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
            Value::String(s) => parcel.write::<String>(s).map_err(ipc_err_handle)?,
            Value::StringList(v) => parcel.write::<Vec<String>>(v).map_err(ipc_err_handle)?,
            Value::NumberList(v) => parcel.write::<Vec<i32>>(v).map_err(ipc_err_handle)?,
        }
    }
    Ok(())
}

/// deserialize the map from parcel
pub fn deserialize_map(parcel: &mut MsgParcel) -> Result<SAFMap> {
    let count = parcel.read::<u32>().map_err(ipc_err_handle)?;
    if count > MAX_MAP_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::IpcReadDataFail,
            "[FATAL][IPC]The map size exceeds the limit."
        );
    }
    let mut map = SAFMap::with_capacity(count as usize);
    for _ in 0..count {
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
            DataType::String => {
                let v = parcel.read::<String>().map_err(ipc_err_handle)?;
                map.insert(tag, Value::String(v));
            },
            DataType::StringList => {
                let v = parcel.read::<Vec<String>>().map_err(ipc_err_handle)?;
                map.insert(tag, Value::StringList(v));
            },
            DataType::NumberList => {
                let v = parcel.read::<Vec<i32>>().map_err(ipc_err_handle)?;
                map.insert(tag, Value::NumberList(v));
            },
        }
    }
    Ok(map)
}

/// Serialize the collection of map to parcel.
pub fn serialize_maps(vec: &Vec<SAFMap>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::IpcWriteDataFail,
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
    let count = parcel.read::<u32>().map_err(ipc_err_handle)?;
    if count > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::IpcReadDataFail,
            "[FATAL][IPC]The vector size exceeds the limit."
        );
    }
    let mut res_vec = Vec::with_capacity(count as usize);
    for _i in 0..count {
        res_vec.push(deserialize_map(parcel)?);
    }
    Ok(res_vec)
}

// ============================================================================
// Batch Request Deserialization (for stub)
// ============================================================================

/// Deserialize BatchGenerateTicket request parameters from MsgParcel.
pub fn deserialize_batch_generate_ticket_request(parcel: &mut MsgParcel) -> Result<(i32, String, Vec<String>)> {
    let os_account_id = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let caller_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let messages = parcel.read_string16_vec().map_err(ipc_err_handle)?;
    Ok((os_account_id, caller_id, messages))
}

/// Deserialize BatchVerifyTicket request parameters from MsgParcel.
pub fn deserialize_batch_verify_ticket_request(parcel: &mut MsgParcel) -> Result<(i32, String, Vec<VerifyTicketInfo>)> {
    let os_account_id = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let caller_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let verify_infos = deserialize_verify_ticket_infos(parcel)?;
    Ok((os_account_id, caller_id, verify_infos))
}

/// Deserialize VerifyTicket request parameters from MsgParcel.
pub fn deserialize_verify_ticket_request(
    parcel: &mut MsgParcel,
) -> Result<(i32, String, String)> {
    let os_account_id = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let caller_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let verify_info = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok((os_account_id, caller_id, verify_info))
}

/// Serialize vector of i32 to MsgParcel (for reply).
pub fn serialize_i32_vec(vec: &Vec<i32>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::IpcWriteDataFail,
            "[FATAL][IPC]i32 vector size exceeds limit: {}",
            vec.len()
        );
    }
    let count = i32::try_from(vec.len()).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArrayLen,
            "[FATAL][IPC]i32 vector length overflows i32")
    })?;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    for val in vec {
        parcel.write::<i32>(val).map_err(ipc_err_handle)?;
    }
    Ok(())
}