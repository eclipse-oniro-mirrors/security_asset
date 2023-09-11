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

use crate::definition::{Tag, Value, Result, ErrCode, DataType, AssetMap};

use hilog_rust::{hilog, HiLogLabel, LogType};
use ipc_rust::BorrowedMsgParcel;
use std::ffi::{c_char, CString};

use super::asset_type_transform::GetType;

/// x
pub trait SerializeAsset {
    /// xxx
    fn serialize(&self, parcel: &mut BorrowedMsgParcel) -> Result<()>;
}

/// x
pub trait DeserializeAsset {
    /// xxx
    fn deserialize(parcel: &BorrowedMsgParcel) -> Result<AssetMap>;
}

impl SerializeAsset for AssetMap {
    fn serialize(&self, parcel: &mut BorrowedMsgParcel) -> Result<()> {
        asset_log_info!("enter serialize");
        parcel.write(&(self.len() as u32))?;
        for v in self.iter() {
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
        asset_log_info!("leave serialize ok");
        Ok(())
    }
}

impl DeserializeAsset for AssetMap {
    fn deserialize(parcel: &BorrowedMsgParcel) -> Result<AssetMap> {
        asset_log_info!("enter deserialize");
        let len = parcel.read::<u32>()?;
        if len > 100 { // to do 外部输入，最大值校验
            return Err(ErrCode::InvalidArgument);
        }
        let mut map = AssetMap::with_capacity(len as usize);
        for _i in 0..len {
            let tag = parcel.read::<u32>()?;
            let asset_tag = Tag::try_from(tag)?;
            match asset_tag.get_type() {
                Ok(DataType::Bool) => {
                    asset_log_info!("try get bool");
                    let v = parcel.read::<bool>()?;
                    map.insert(asset_tag, Value::BOOL(v));
                },
                Ok(DataType::Uint32) => {
                    asset_log_info!("try get u32");
                    let v = parcel.read::<u32>()?;
                    map.insert(asset_tag, Value::NUMBER(v));
                },
                Ok(DataType::Bytes) => {
                    asset_log_info!("try get uint8array");
                    let v = parcel.read::<Vec<u8>>()?;
                    map.insert(asset_tag, Value::Bytes(v));
                },
                Ok(DataType::Uint64 | DataType::Int32 | DataType::Int64) => {
                    asset_log_error!("deserialize {} failed!", @public(tag));
                    return Err(ErrCode::IpcError);
                },
                Err(_) => {
                    asset_log_error!("deserialize {} failed!", @public(tag));
                    return Err(ErrCode::IpcError);
                },
            }
        }
        asset_log_info!("leave deserialize ok");
        Ok(map)
    }
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
                Err(ErrCode::InvalidArgument)
            },
            Ok(DataType::Bool) => {
                if let Value::BOOL(real) = value.get_real()  {
                    self.insert(key, Value::BOOL(real));
                    return Ok(());
                }
                Err(ErrCode::Failed)
            },
            Ok(DataType::Bytes) => {
                if let Value::Bytes(real) = value.get_real()  {
                    self.insert(key, Value::Bytes(real));
                    return Ok(());
                }
                Err(ErrCode::Failed)
            },
            Ok(DataType::Uint64 | DataType::Int32 | DataType::Int64) => {
                asset_log_error!("insert {} failed!", @public(key as u32));
                Err(ErrCode::IpcError)
            },
            Err(_) => {
                Err(ErrCode::Failed)
            }
        }
    }
}
