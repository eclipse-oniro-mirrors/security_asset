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

//! Task identity related serialization functions.

use ipc::parcel::MsgParcel;
use saf_definition::{
    macros_lib, ErrCode, Result,
    ExecuteType, TaskIdentity, TaskIdentityInfo,
};

use crate::ipc_err_handle;

/// Deserialize TaskIdentityInfo from MsgParcel.
pub fn deserialize_task_identity_info(parcel: &mut MsgParcel) -> Result<TaskIdentityInfo> {
    let task_id = parcel.read_string16().map_err(ipc_err_handle)?;
    let task_expire_time = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let execute_type_val = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let execute_type = match execute_type_val {
        1 => ExecuteType::Instant,
        2 => ExecuteType::Delayed,
        _ => return macros_lib::log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL][IPC]Invalid execute type: {}",
            execute_type_val
        ),
    };
    let allow_screen_lock_execution = parcel.read::<bool>().map_err(ipc_err_handle)?;
    let allowable_duration = parcel.read::<i32>().map_err(ipc_err_handle)?;
    let auth_token = deserialize_i32_u8_list(parcel)?;
    Ok(TaskIdentityInfo {
        task_id,
        task_expire_time,
        execute_type,
        allow_screen_lock_execution,
        allowable_duration,
        auth_token,
    })
}

/// Serialize TaskIdentity to MsgParcel (for reply).
pub fn serialize_task_identity(identity: &TaskIdentity, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write_string16(&identity.task_attributes).map_err(ipc_err_handle)?;
    parcel.write_string16(&identity.identity).map_err(ipc_err_handle)?;
    parcel.write_string16(&identity.challenge).map_err(ipc_err_handle)?;
    Ok(())
}

/// Maximum value for a u8 byte (255).
const U8_MAX: i32 = 255;

/// Deserialize an i32 list carrying raw bytes (authToken / challenge) from MsgParcel.
/// Each value must be in u8 range (0~255); values outside this range are rejected.
pub fn deserialize_i32_u8_list(parcel: &mut MsgParcel) -> Result<Vec<u8>> {
    let count = parcel.read::<i32>().map_err(ipc_err_handle)?;
    if count < 0 || count as u32 > crate::MAX_VEC_CAPACITY {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArrayLen,
            "[FATAL][IPC]i32 list size invalid: {}",
            count
        );
    }
    let mut vec = Vec::with_capacity(count as usize);
    for _ in 0..count {
        let val = parcel.read::<i32>().map_err(ipc_err_handle)?;
        if !(0..=U8_MAX).contains(&val) {
            return macros_lib::log_throw_error!(
                ErrCode::InvalidArgument,
                "[FATAL][IPC]i32 value out of u8 range: {}",
                val
            );
        }
        vec.push(val as u8);
    }
    Ok(vec)
}

/// Serialize raw bytes as an i32 list to MsgParcel (for reply).
pub fn serialize_u8_to_i32_list(bytes: &[u8], parcel: &mut MsgParcel) -> Result<()> {
    let count = i32::try_from(bytes.len()).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArrayLen,
            "[FATAL][IPC]bytes length overflows i32")
    })?;
    parcel.write::<i32>(&count).map_err(ipc_err_handle)?;
    for byte in bytes {
        parcel.write::<i32>(&(i32::from(*byte))).map_err(ipc_err_handle)?;
    }
    Ok(())
}

/// Deserialize TaskIdentity from MsgParcel (trailing field of PermissionQuery).
pub fn deserialize_task_identity(parcel: &mut MsgParcel) -> Result<TaskIdentity> {
    let task_attributes = parcel.read_string16().map_err(ipc_err_handle)?;
    let identity = parcel.read_string16().map_err(ipc_err_handle)?;
    let challenge = parcel.read_string16().map_err(ipc_err_handle)?;
    Ok(TaskIdentity {
        task_attributes,
        identity,
        challenge,
    })
}
