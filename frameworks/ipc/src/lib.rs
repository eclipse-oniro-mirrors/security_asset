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

//! This module defines IPC interfaces and constants.

use ipc::{parcel::MsgParcel, IpcStatusCode};

use asset_definition::{
    impl_enum_trait, log_throw_error, AssetError, AssetMap, Conversion, DataType, ErrCode, Result, SyncResult,
    Tag, Value,
};

/// SA id for Asset service.
pub const SA_ID: i32 = 8100;
/// SA name for Asset service.
pub const SA_NAME: &str = "security_asset_service";
/// IPC result code.
pub const IPC_SUCCESS: u32 = 0;

const MAX_MAP_CAPACITY: u32 = 64;
const MAX_VEC_CAPACITY: u32 = 0x10000;

impl_enum_trait! {
    /// Code used to identify the function to be called.
    #[derive(Clone, Copy)]
    #[derive(Eq, PartialEq)]
    pub enum IpcCode {
        /// Code for AddAsset.
        Add = ipc::FIRST_CALL_TRANSACTION,
        /// Code for RemoveAsset.
        Remove,
        /// Code for UpdateAsset.
        Update,
        /// Code for PreQueryAsset.
        PreQuery,
        /// Code for QueryAsset.
        Query,
        /// Code for PostQueryAsset.
        PostQuery,
        /// Code for QuerySyncResult.
        QuerySyncResult,
    }
}

/// serialize the map to parcel
pub fn serialize_map(map: &AssetMap, parcel: &mut MsgParcel) -> Result<()> {
    if map.len() as u32 > MAX_MAP_CAPACITY {
        return log_throw_error!(ErrCode::InvalidArgument, "[FALTAL][IPC]The map size exceeds the limit.");
    }
    parcel.write(&(map.len() as u32)).map_err(ipc_err_handle)?;
    for (&tag, value) in map.iter() {
        if tag.data_type() != value.data_type() {
            return log_throw_error!(
                ErrCode::InvalidArgument,
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
pub fn deserialize_map(parcel: &mut MsgParcel) -> Result<AssetMap> {
    let len = parcel.read::<u32>().map_err(ipc_err_handle)?;
    if len > MAX_MAP_CAPACITY {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][IPC]The map size exceeds the limit.");
    }
    let mut map = AssetMap::with_capacity(len as usize);
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
pub fn serialize_maps(vec: &Vec<AssetMap>, parcel: &mut MsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][IPC]The vector size exceeds the limit.");
    }
    parcel.write::<u32>(&(vec.len() as u32)).map_err(ipc_err_handle)?;
    for map in vec.iter() {
        serialize_map(map, parcel)?;
    }
    Ok(())
}

/// Deserialize the collection of map from parcel.
pub fn deserialize_maps(parcel: &mut MsgParcel) -> Result<Vec<AssetMap>> {
    let len = parcel.read::<u32>().map_err(ipc_err_handle)?;
    if len > MAX_VEC_CAPACITY {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][IPC]The vector size exceeds the limit.");
    }
    let mut res_vec = Vec::with_capacity(len as usize);
    for _i in 0..len {
        res_vec.push(deserialize_map(parcel)?);
    }
    Ok(res_vec)
}

/// Serialize the sync result to parcel.
pub fn serialize_sync_result(sync_result: &SyncResult, parcel: &mut MsgParcel) -> Result<()> {
    parcel.write::<i32>(&sync_result.error_code).map_err(ipc_err_handle)?;
    parcel.write::<u32>(&sync_result.total_count).map_err(ipc_err_handle)?;
    parcel.write::<u32>(&sync_result.failed_count).map_err(ipc_err_handle)
}

/// Deserialize the sync result from parcel.
pub fn deserialize_sync_result(parcel: &mut MsgParcel) -> Result<SyncResult> {
    Ok(SyncResult {
        error_code: parcel.read::<i32>().map_err(ipc_err_handle)?,
        total_count: parcel.read::<u32>().map_err(ipc_err_handle)?,
        failed_count: parcel.read::<u32>().map_err(ipc_err_handle)?
    })
}

/// Convert ipc error into Asset error.
pub fn ipc_err_handle(e: IpcStatusCode) -> AssetError {
    AssetError::new(ErrCode::IpcError, format!("[FATAL][IPC]Ipc status code = {}", e))
}
