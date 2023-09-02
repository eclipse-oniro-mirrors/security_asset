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

mod asset_map;
#[macro_use]
mod asset_type_transform;

pub use crate::asset_type::asset_map::*;

/// version info
pub struct VersionInfo {
    major: u32,
    minor: u32,
    patch: u32,
}

#[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
/// asset tag type
pub enum AssetType {
    /// bool
    Bool = 1 << 28,
    /// u32
    U32 = 2 << 28,
    /// u8array
    Uint8Array = 3 << 28,
}

/// asset value
#[derive(Clone, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum Value {
    /// bool for asset
    BOOL(bool),

    /// number for asset
    NUMBER(u32),

    /// uint8array for asset
    UINT8ARRAY(Vec<u8>),
}

enum_auto_prepare!{
    /// asset tag
    #[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
    pub enum Tag {
        /// secret
        Secret = AssetType::Uint8Array as isize | 1,
        /// alias tag
        Alias = AssetType::Uint8Array as isize | 2,
        /// ACCESSIBILITY
        Accessibility = AssetType::U32 as isize | 3,
        /// auth type
        AuthType = AssetType::U32 as isize | 4,
        /// SyncType
        SyncType = AssetType::U32 as isize | 5,
        /// ConfictPolicy
        ConfictPolicy = AssetType::U32 as isize | 6,
        /// DataLabelCritical1
        DataLabelCritical1 = AssetType::Uint8Array as isize | 7,
        /// DataLabelCritical2
        DataLabelCritical2 = AssetType::Uint8Array as isize | 8,
        /// DataLabelCritical3
        DataLabelCritical3 = AssetType::Uint8Array as isize | 9,
        /// DataLabelCritical4
        DataLabelCritical4 = AssetType::Uint8Array as isize | 10,
        /// DataLabelNormal1
        DataLabelNormal1 = AssetType::Uint8Array as isize | 11,
        /// DataLabelNormal2
        DataLabelNormal2 = AssetType::Uint8Array as isize | 12,
        /// DataLabelNormal3
        DataLabelNormal3 = AssetType::Uint8Array as isize | 13,
        /// DataLabelNormal4
        DataLabelNormal4 = AssetType::Uint8Array as isize | 14,
        /// AuthValidtyPeriod
        AuthValidityPeriod = AssetType::U32 as isize | 15,
        /// ReturnLimit
        ReturnLimit = AssetType::U32 as isize | 16,
        /// ReturnOffset
        ReturnOffset = AssetType::U32 as isize | 17,
        /// ReturnOrderBy
        ReturnOrderBy = AssetType::U32 as isize | 18,
        /// ReturnType
        ReturnType = AssetType::U32 as isize | 19,
        /// AuthChallenge
        AuthChallenge = AssetType::Uint8Array as isize | 20,
        /// AuthToken
        AuthToken = AssetType::Uint8Array as isize | 21,
    }
}

enum_auto_prepare! {
    /// Asset unified status code
    #[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
    pub enum AssetStatusCode {
        /// success
        Ok = 1,
        /// failed
        Failed = -1,
        /// IpcFailed
        IpcFailed = -2,
        /// InvalidArgement
        InvalidArgument = -3,
    }
}

/// asset result
pub type AssetResult<T> = std::result::Result<T, AssetStatusCode>;

#[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
/// auth type
pub enum AssetAuthType {
    /// None
    None,
    /// any
    Any,
}

/// enum for AssetAccessibility
pub enum Accessibility {
    /// DevicePowerOn
    DevicePowerOn,
    /// DevoceFirstUnlock
    DevoceFirstUnlock,
    /// DeviceUnlock
    DeviceUnlock,
    /// DeviceSecure
    DeviceSecure,
}

/// AssetSyncType
pub enum AssetSyncType {
    /// None
    None,
    /// ThisDevice
    ThisDevice,
    /// TrustedAccount
    TrustedAccount,
    /// TrustedDevice
    TrustedDevice
}

/// AssetConflictPolicy
pub enum AssetConflictPolicy {
    /// OverRide
    OverRide,
    /// Report
    Report,
}

/// AssetReturnType
pub enum AssetReturnType {
    /// All
    All,
    /// Attributes
    Attributes
}
