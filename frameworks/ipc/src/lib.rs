/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

use ipc_rust::BorrowedMsgParcel;

use asset_definition::{impl_enum_trait, AssetMap, DataType, ErrCode, IntoValue, Result, Tag, Value};
use asset_log::loge;

/// SA id for Asset service.
pub const SA_ID: i32 = 0x00010140;
/// SA name for Asset service.
pub const SA_NAME: &str = "security_asset_service";
/// IPC result code.
pub const IPC_SUCCESS: u32 = 0;

const MAX_MAP_CAPACITY: u32 = 100;
const MAX_VEC_CAPACITY: u32 = 0x10000;

impl_enum_trait! {
    /// Code used to identify the function to be called.
    #[derive(Clone, Copy)]
    pub enum IpcCode {
        /// Code for AddAsset.
        Add = ipc_rust::FIRST_CALL_TRANSACTION,
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
    }
}

/// Function between proxy and stub of Asset service.
pub trait IAsset: ipc_rust::IRemoteBroker {
    /// Add an Asset.
    fn add(&self, attributes: &AssetMap) -> Result<()>;

    /// Remove one or more Assets that match a search query.
    fn remove(&self, query: &AssetMap) -> Result<()>;

    /// Update an Asset that matches a search query.
    fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()>;

    /// Preprocessing for querying one or more Assets that require user authentication.
    fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>>;

    /// Query one or more Assets that match a search query.
    fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>>;

    /// Post-processing for querying multiple Assets that require user authentication.
    fn post_query(&self, query: &AssetMap) -> Result<()>;
}

/// serialize the map to parcel
pub fn serialize_map(map: &AssetMap, parcel: &mut BorrowedMsgParcel) -> Result<()> {
    if map.len() as u32 > MAX_MAP_CAPACITY {
        loge!("[FALTAL][IPC]The map size exceeds the limit.");
        return Err(ErrCode::LimitExceeded);
    }
    parcel.write(&(map.len() as u32)).map_err(|_| ErrCode::IpcError)?;
    for (&tag, value) in map.iter() {
        if tag.data_type() != value.data_type() {
            loge!("[FATAL][IPC]Data type mismatch, key type: {}, value type: {}", tag.data_type(), value.data_type());
            return Err(ErrCode::InvalidArgument);
        }
        parcel.write(&(tag as u32)).map_err(|_| ErrCode::IpcError)?;
        match value {
            Value::Bool(b) => parcel.write::<bool>(b).map_err(|_| ErrCode::IpcError)?,
            Value::Number(n) => parcel.write::<u32>(n).map_err(|_| ErrCode::IpcError)?,
            Value::Bytes(a) => parcel.write::<Vec<u8>>(a).map_err(|_| ErrCode::IpcError)?,
        }
    }
    Ok(())
}

/// deserialize the map from parcel
pub fn deserialize_map(parcel: &BorrowedMsgParcel) -> Result<AssetMap> {
    let len = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
    if len > MAX_MAP_CAPACITY {
        loge!("[FATAL][IPC]The map size exceeds the limit.");
        return Err(ErrCode::LimitExceeded);
    }
    let mut map = AssetMap::with_capacity(len as usize);
    for _ in 0..len {
        let tag = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
        let tag = Tag::try_from(tag)?;
        match tag.data_type() {
            DataType::Bool => {
                let v = parcel.read::<bool>().map_err(|_| ErrCode::IpcError)?;
                map.insert(tag, Value::Bool(v));
            },
            DataType::Number => {
                let v = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
                map.insert(tag, Value::Number(v));
            },
            DataType::Bytes => {
                let v = parcel.read::<Vec<u8>>().map_err(|_| ErrCode::IpcError)?;
                map.insert(tag, Value::Bytes(v));
            },
        }
    }
    Ok(map)
}

/// Serialize the collection of map to parcel.
pub fn serialize_maps(vec: &Vec<AssetMap>, parcel: &mut BorrowedMsgParcel) -> Result<()> {
    if vec.len() as u32 > MAX_VEC_CAPACITY {
        loge!("[FATAL][IPC]The vector size exceeds the limit.");
        return Err(ErrCode::LimitExceeded);
    }
    parcel.write::<u32>(&(vec.len() as u32)).map_err(|_| ErrCode::IpcError)?;
    for map in vec.iter() {
        serialize_map(map, parcel)?;
    }
    Ok(())
}

/// Deserialize the collection of map from parcel.
pub fn deserialize_maps(parcel: &BorrowedMsgParcel) -> Result<Vec<AssetMap>> {
    let len = parcel.read::<u32>().map_err(|_| ErrCode::InvalidArgument)?;
    if len > MAX_VEC_CAPACITY {
        loge!("[FATAL][IPC]The vector size exceeds the limit.");
        return Err(ErrCode::LimitExceeded);
    }
    let mut res_vec = Vec::with_capacity(len as usize);
    for _i in 0..len {
        res_vec.push(deserialize_map(parcel)?);
    }
    Ok(res_vec)
}
