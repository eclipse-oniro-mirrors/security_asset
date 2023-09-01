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

use crate::asset_type::{AssetTag, AssetValue, AssetResult, AssetStatusCode, AssetType};

use hilog_rust::{hilog, HiLogLabel, LogType};
use ipc_rust::BorrowedMsgParcel;
use std::collections::HashMap;
use std::ffi::{c_char, CString};

/// asset map
pub type AssetMap = HashMap<AssetTag, AssetValue>;

/// x
pub trait SerializeAsset {
    /// xxx
    fn serialize(&self, parcel: &mut BorrowedMsgParcel) -> AssetResult<()>;
}

/// x
pub trait DeserializeAsset {
    /// xxx
    fn deserialize(parcel: &BorrowedMsgParcel) -> AssetResult<AssetMap>;
}

impl SerializeAsset for AssetMap {
    fn serialize(&self, parcel: &mut BorrowedMsgParcel) -> AssetResult<()> {
        parcel.write(&(self.len() as u32))?;
        for v in self.iter() {
            parcel.write(&(*v.0 as u32))?;
            match v.1 {
                AssetValue::BOOL(b) => {
                    parcel.write(b)?;
                },
                AssetValue::NUMBER(n) => {
                    parcel.write(n)?;
                },
                AssetValue::UINT8ARRAY(a) => {
                    parcel.write(a)?;
                },
            }
        }
        Ok(())
    }
}

impl DeserializeAsset for AssetMap {
    fn deserialize(parcel: &BorrowedMsgParcel) -> AssetResult<AssetMap> {
        let len = parcel.read::<u32>()?;
        let mut map = AssetMap::with_capacity(len as usize);
        for _i in 0..len {
            let tag = parcel.read::<u32>()?;
            let asset_tag = AssetTag::try_from(tag);
            if asset_tag.is_err() {
                asset_log_error!("deserialize tah failed!");
                return Err(AssetStatusCode::IpcFailed);
            }
            let asset_tag = asset_tag.unwrap();
            match asset_tag.get_type() {
                Ok(AssetType::Bool) => {
                    let v = parcel.read::<bool>()?;
                    map.insert(asset_tag, AssetValue::BOOL(v));
                },
                Ok(AssetType::U32) => {
                    let v = parcel.read::<u32>()?;
                    map.insert(asset_tag, AssetValue::NUMBER(v));
                },
                Ok(AssetType::Uint8Array) => {
                    let v = parcel.read::<Vec<u8>>()?;
                    map.insert(asset_tag, AssetValue::UINT8ARRAY(v));
                },
                Err(_) => {
                    return Err(AssetStatusCode::IpcFailed);
                },
            }
        }
        Ok(map)
    }
}
