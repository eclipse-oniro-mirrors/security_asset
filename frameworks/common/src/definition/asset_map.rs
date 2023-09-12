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

//! map的各类操作

use ipc_rust::BorrowedMsgParcel;

use super::asset_type_transform::GetType;
use crate::definition::{Tag, Value, Result, ErrCode, DataType, AssetMap};
use crate::loge;

/// serialize the map to parcel
pub fn serialize_map_into_parcel(map: &AssetMap, parcel: &mut BorrowedMsgParcel) -> Result<()> {
    logi!("enter serialize_map_into_parcel");
    parcel.write(&(map.len() as u32))?;
    for v in map.iter() {
        parcel.write(&(*v.0 as u32))?;
        match v.1 {
            Value::BOOL(b) => {
                parcel.write::<bool>(b)?;
            },
            Value::NUMBER(n) => {
                parcel.write::<u32>(n)?;
            },
            Value::Bytes(a) => {
                parcel.write::<Vec<u8>>(a)?;
            },
        }
    }
    logi!("leave serialize_map_into_parcel ok");
    Ok(())
}

/// deserialize the map from parcel
pub fn deserialize_map_from_parcel(parcel: &BorrowedMsgParcel) -> Result<AssetMap> {
    logi!("enter deserialize_map_from_parcel");
    let len = parcel.read::<u32>()?;
    if len > 100 { // to do 外部输入，最大值校验
        loge!("The map size exceeds the limit.");
        return Err(ErrCode::InvalidArgument);
    }
    let mut map = AssetMap::with_capacity(len as usize);
    for _i in 0..len {
        let tag = parcel.read::<u32>()?;
        let asset_tag = Tag::try_from(tag)?;
        match asset_tag.get_type() {
            Ok(DataType::Bool) => {
                logi!("try get bool");
                let v = parcel.read::<bool>()?;
                map.insert(asset_tag, Value::BOOL(v));
            },
            Ok(DataType::Uint32) => {
                logi!("try get u32");
                let v = parcel.read::<u32>()?;
                map.insert(asset_tag, Value::NUMBER(v));
            },
            Ok(DataType::Bytes) => {
                logi!("try get uint8array");
                let v = parcel.read::<Vec<u8>>()?;
                map.insert(asset_tag, Value::Bytes(v));
            },
            Ok(DataType::Uint64 | DataType::Int32 | DataType::Int64) => {
                loge!("deserialize {} failed!", tag);
                return Err(ErrCode::IpcError);
            },
            Err(_) => {
                loge!("deserialize {} failed!", tag);
                return Err(ErrCode::IpcError);
            },
        }
    }
    logi!("leave deserialize_map_from_parcel ok");
    Ok(map)
}

/// xxx
pub trait InsertAttribute {
    ///
    fn insert_attr(&mut self, key: Tag, value: impl GetType) -> Result<()>;
}

impl InsertAttribute for AssetMap {
    fn insert_attr(&mut self, key: Tag, value: impl GetType) -> Result<()> {
        match value.get_type() {
            Ok(DataType::Uint32) => {
                if let Value::NUMBER(real) = value.get_real()  {
                    self.insert(key, Value::NUMBER(real));
                    return Ok(());
                }
                loge!("InsertAttribute u32 failed!");
                Err(ErrCode::InvalidArgument)
            },
            Ok(DataType::Bool) => {
                if let Value::BOOL(real) = value.get_real()  {
                    self.insert(key, Value::BOOL(real));
                    return Ok(());
                }
                loge!("InsertAttribute bool failed!");
                Err(ErrCode::Failed)
            },
            Ok(DataType::Bytes) => {
                if let Value::Bytes(real) = value.get_real()  {
                    self.insert(key, Value::Bytes(real));
                    return Ok(());
                }
                loge!("InsertAttribute byte failed!");
                Err(ErrCode::Failed)
            },
            _ => {
                loge!("insert {} failed invalid tag!", key as u32);
                Err(ErrCode::Failed)
            }
        }
    }
}
