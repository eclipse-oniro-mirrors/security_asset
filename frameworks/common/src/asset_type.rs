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

#![allow(dead_code)]

use std::collections::HashMap;
use std::fmt;

use ipc_rust::MsgParcel;

mod asset_serialize_deserialize;
mod asset_tag_operator;

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
/// asset tag type
pub enum AssetType {
    /// bool
    Bool = 1 << 28,
    /// u32
    U32 = 2 << 28,
    /// u8array
    Uint8Array = 3 << 28,
}

/// asset tag
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum AssetTag {
    /// alias tag
    AssetTagAlias = AssetType::Uint8Array as isize | 1,
    /// auth type
    AssetTagAuthType = AssetType::U32 as isize | 1,
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
/// auth type
pub enum AssetAuthType {
    /// sss
    NoAuth
}

/// aset value
#[derive(Clone, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum AssetValue {
    /// bool for asset
    BOOL(bool),

    /// number for asset
    NUMBER(u32),

    /// uint8array for asset
    UINT8ARRAY(Vec<u8>),
}

/// asset map
pub type AssetMap = HashMap<AssetTag, AssetValue>;

/// x
pub trait SerializeAsset {
    /// xxx
    fn serialize(&self, parcel: &mut MsgParcel) -> AssetResult<()>;
}

/// x
pub trait DeserializeAsset {
    /// xxx
    fn deserialize(parcel: &MsgParcel) -> AssetResult<AssetMap>;
}

/// Asset unified status code
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum AssetStatusCode {
    /// success
    Ok = 1,
    /// failed
    Failed = -1,
}

impl fmt::Display for AssetStatusCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            AssetStatusCode::Ok => write!(f, "Ok"),
            AssetStatusCode::Failed => write!(f, "Failed"),
        }
    }
}

/// Asset ipc code
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum AssetIpcCode {
    /// insert data
    Insert = 1,
}

impl fmt::Display for AssetIpcCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            AssetIpcCode::Insert => write!(f, "insert"),
        }
    }
}

/// asset result
pub type AssetResult<T> = std::result::Result<T, AssetStatusCode>;