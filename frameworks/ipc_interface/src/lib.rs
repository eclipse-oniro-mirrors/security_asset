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

use asset_common::{
    definition::{AssetMap, Result, DataType, ErrCode, IntoValue, Tag, Value},
    impl_enum_trait,
    loge,
    logi,
};

use ipc_rust::BorrowedMsgParcel;

/// SA id for Asset service
pub const SA_ID: i32 = 3511;
/// SA name for Asset service
pub const SA_NAME: &str = "security_asset_service";
/// IPC result code.
pub const IPC_SUCCESS: i32 = 0;

impl_enum_trait!{
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

/// Function between proxy and stub of Asset service
pub trait IAsset: ipc_rust::IRemoteBroker {
    /// Add an asset.
    fn add(&self, input: &AssetMap) -> Result<()>;

    /// Add an asset.
    fn query(&self, input: &AssetMap) -> Result<Vec<AssetMap>>;
}

/// max capacity in a map
const MAP_MAX_CAPACITY: u32 = 30;

/// serialize the map to parcel
pub fn serialize_map(map: &AssetMap, parcel: &mut BorrowedMsgParcel) -> Result<()> {
    logi!("enter serialize");
    parcel.write(&(map.len() as u32)).map_err(|_| ErrCode::IpcError)?;
    for v in map.iter() {
        parcel.write(&(*v.0 as u32)).map_err(|_| ErrCode::IpcError)?;
        match v.1 {
            Value::Bool(b) => parcel.write::<bool>(b).map_err(|_| ErrCode::IpcError)?,
            Value::Number(n) => parcel.write::<u32>(n).map_err(|_| ErrCode::IpcError)?,
            Value::Bytes(a) => parcel.write::<Vec<u8>>(a).map_err(|_| ErrCode::IpcError)?,
        }
    }
    logi!("leave serialize ok");
    Ok(())
}

/// deserialize the map from parcel
pub fn deserialize_map(parcel: &BorrowedMsgParcel) -> Result<AssetMap> {
    logi!("enter deserialize");
    let len = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
    if len > MAP_MAX_CAPACITY {
        loge!("The map size exceeds the limit.");
        return Err(ErrCode::InvalidArgument);
    }
    let mut map = AssetMap::with_capacity(len as usize);
    for _i in 0..len {
        let tag = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
        let asset_tag = Tag::try_from(tag)?;
        match asset_tag.data_type() {
            DataType::Bool => {
                logi!("try get u32");
                let v = parcel.read::<bool>().map_err(|_| ErrCode::IpcError)?;
                map.insert(asset_tag, Value::Bool(v));
            }
            DataType::Uint32 => {
                logi!("try get u32");
                let v = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
                map.insert(asset_tag, Value::Number(v));
            },
            DataType::Bytes => {
                logi!("try get uint8array");
                let v = parcel.read::<Vec<u8>>().map_err(|_| ErrCode::IpcError)?;
                map.insert(asset_tag, Value::Bytes(v));
            },
        }
    }
    logi!("leave deserialize ok");
    Ok(map)
}

/// serialize the vector of map to parcel
pub fn serialize_vector_map(vec: &Vec<AssetMap>, parcel: &mut BorrowedMsgParcel) -> Result<()> {
    logi!("enter serialize_vector_map");
    parcel.write(&(vec.len() as u32)).map_err(|_| ErrCode::IpcError)?;
    for i in 0..vec.len() {
        match vec.get(i) {
            Some(map) => {
                serialize_map(map, parcel)?;
            },
            None => {
                loge!("get map from vec in serialize_vector_map failed!");
                return Err(ErrCode::InvalidArgument);
            }
        }
    }
    logi!("leave serialize_vector_map ok");
    Ok(())
}

/// deserialize the vector of map from parcel
pub fn deserialize_vector_map(parcel: &BorrowedMsgParcel) -> Result<Vec<AssetMap>> {
    logi!("enter deserialize_vector_map");
    let len = parcel.read::<u32>().map_err(|_| ErrCode::IpcError)?;
    if len > MAP_MAX_CAPACITY { // todo 最大允许值
        return Err(ErrCode::IpcError);
    }
    let mut res_vec = Vec::with_capacity(len as usize);
    for _i in 0..len {
        res_vec.push(deserialize_map(parcel)?);
    }
    logi!("leave deserialize_vector_map ok");
    Ok(res_vec)
}