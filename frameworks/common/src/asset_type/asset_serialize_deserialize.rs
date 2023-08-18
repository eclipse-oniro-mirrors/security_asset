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

//! This create implement the asset

use super::{AssetMap, AssetTag, SerializeAsset, DeserializeAsset, AssetResult, AssetStatusCode, AssetValue};

use ipc_rust::{MsgParcel, IpcResult, IpcStatusCode};

impl AssetTag {
    fn from(u: u32) -> AssetResult<AssetTag>
    {
        // match u {
        //     // AssetTag::AssetTagAlias as u32 => {
        //     //     return Ok(AssetTag::AssetTagAlias)
        //     // },
        //     0 => {
        //         return Ok(AssetTag::AssetTagAlias)
        //     },
        //     _ => {}
        // }
        // Err(AssetStatusCode::Failed)

        if u == AssetTag::AssetTagAlias as u32 {
            return Ok(AssetTag::AssetTagAlias);
        }
        Err(AssetStatusCode::Failed)
    }
}

fn serialize_ipc(map: &AssetMap, parcel: &mut MsgParcel) -> IpcResult<()>
{
    parcel.write(&(map.len() as u32))?;
    for v in map.iter() {
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
            }
        }
    }
    Ok(())
}

impl SerializeAsset for AssetMap {
    fn serialize(&self, parcel: &mut MsgParcel) -> AssetResult<()>
    {
        match serialize_ipc(self, parcel) {
            Ok(_) => Ok(()),
            Err(_) => Err(AssetStatusCode::Failed)
        }
    }
}

fn deserialize_ipc(parcel: &MsgParcel) -> IpcResult<AssetMap>
{
    let len = parcel.read::<u32>()?;
    let mut map = AssetMap::with_capacity(len as usize);
    for _i in 0..len {
        let tag = parcel.read::<u32>()?;
        // to do 从tag可判断出value类型
        // let _value = parcel.read()?;
        
        // map.insert(tag, value);
        if let Ok(t) = AssetTag::from(tag) {
            map.insert(t, AssetValue::BOOL(true)); // to do delete
        } else {
            return Err(IpcStatusCode::Failed);
        }
    }
    Ok(map)
}

impl DeserializeAsset for AssetMap {
    fn deserialize(parcel: &MsgParcel) -> AssetResult<AssetMap>
    {
        match deserialize_ipc(parcel) {
            Ok(map) => Ok(map),
            Err(_) => Err(AssetStatusCode::Failed)
        }
    }
}