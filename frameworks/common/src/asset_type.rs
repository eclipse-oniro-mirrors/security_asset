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
pub mod asset_type_transform;

pub use crate::asset_type::asset_map::*;

/// The asset version.
pub struct Version {
    /// The major version.
    major: u32,

    /// The minor version.
    minor: u32,

    /// The patch version.
    patch: u32,
}

#[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
/// An enum type that indicates the type of the asset attribute value.
pub enum AssetType { // todo: asset_type mod defines, AssetType改成DataType，与数据库共用一个数据类型
    /// The type of the asset attribute value is int32.
    Int32 = 1 << 28,
    /// The type of the asset attribute value is uint32.
    Uint32 = 2 << 28,
    /// The type of the asset attribute value is int64.
    Int64 = 3 << 28,
    /// The type of the asset attribute value is uint64.
    Uint64 = 4 << 28,
    /// The type of the asset attribute value is bool.
    Bool = 5 << 28,
    /// The type of the asset attribute value is byte array.
    Bytes = 6 << 28,
}

/// asset value
#[derive(Clone, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum Value {
    /// bool for asset
    BOOL(bool),

    /// number for asset
    NUMBER(u32),

    /// bytes for asset
    Bytes(Vec<u8>),
}

enum_auto_impl_try_from! {
    /// An emum type that indicates the tag of the asset attribute.
    #[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
    pub enum Tag {
        /// A tag whose value is the asset, such as password and token.
        Secret = AssetType::Bytes as isize | 1,

        /// A tag whose value used to identify an asset.
        Alias = AssetType::Bytes as isize | 2,

        /// A tag whose value indicates when the asset can be accessed.
        Accessibility = AssetType::Uint32 as isize | 3,

        /// A tag whose value indicates what type of user authentication is required.
        AuthType = AssetType::Uint32 as isize | 4,

        /// A tag whose value indicates the validity period of user authentication, in seconds.
        AythValidityPeriod = AssetType::Uint32 as isize | 5,

        /// A tag whose value indicates the authentication challenge for anti-replay.
        AuthChallenge = AssetType::Bytes as isize | 6,

        /// A tag whose value indicates the credential after successful authentication of the user.
        AuthToken = AssetType::Bytes as isize | 7,

        /// A tag whose value indicates the type of asset synchronization.
        SyncType = AssetType::Uint32 as isize | 8,

        /// A tag whose value indicates the conflict handling policy for adding the asset with the same alias.
        ConfictPolicy = AssetType::Uint32 as isize | 9,

        /// A tag whose value indicates the first customized critical data of the asset.
        DataLabelCritical1 = AssetType::Bytes as isize | 10,

        /// A tag whose value indicates the second customized critical data of the asset.
        DataLabelCritical2 = AssetType::Bytes as isize | 11,

        /// A tag whose value indicates the third customized critical data of the asset.
        DataLabelCritical3 = AssetType::Bytes as isize | 12,

        /// A tag whose value indicates the fourth customized critical data of the asset.
        DataLabelCritical4 = AssetType::Bytes as isize | 13,

        /// A tag whose value indicates the first customized normal data of the asset.
        DataLabelNormal1 = AssetType::Bytes as isize | 14,

        /// A tag whose value indicates the second customized normal data of the asset.
        DataLabelNormal2 = AssetType::Bytes as isize | 15,

        /// A tag whose value indicates the third customized normal data of the asset.
        DataLabelNormal3 = AssetType::Bytes as isize | 16,

        /// A tag whose value indicates the fourth customized normal data of the asset.
        DataLabelNormal4 = AssetType::Bytes as isize | 17,

        /// A tag whose value indicates the type of the returned data.
        ReturnType = AssetType::Uint32 as isize | 18,

        /// A tag whose value indicates the maximum number of assets that can be returned in a query.
        ReturnLimit = AssetType::Uint32 as isize | 19,

        /// A tag whose value indicates the offset of the batch query result.
        ReturnOffset = AssetType::Uint32 as isize | 20,

        /// A tag whose value indicates the order by which the query result is returned.
        ReturnOrderBy = AssetType::Uint32 as isize | 21,

    }
}

enum_auto_impl_try_from! {
    /// Asset unified status code
    #[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
    pub enum AssetStatusCode {
        /// success
        Success = 0,
        /// failed
        Failed = -1,

        /// The error code indicates that the permission is denied.
        PermissionDenied = 201,

        /// The error code indicates that the parameter is invalid
        InvalidArgument = 401,

        /// The error code indicates that the capability is not supported.
        NotSupport = 801,

        /// The error code indicates that the asset service is unavailable.
        ServiceUnvailable = 24000001,

        /// The error code indicates that the asset to be queried is not found.
        NotFound = 24000002,

        /// The error code indicates that the asset to be added is duplicate.
        Duplicated = 24000003,

        /// The error code indicates that the asset access is denied.
        AccessDenied = 24000004,

        /// The error code indicates that the authentication token has expired.
        AuthTokenExpired = 24000005,

        /// The error code indicates that the system memory is insufficient.
        OutOfMemory = 24000006,

        /// The error code indicates that the asset or key is corrupted.
        DataCorrupted = 24000007,

        // to do
        /// The error code indicates that the ipc communication is failed.
        IpcFailed = 24000008,

        /// Generic error
        SqliteERROR = 1,

        /// Internal logic error in SQLite
        SqliteINTERNAL = 2,

        /// Access permission denied
        SqlitePERM = 3,

        /// Callback routine requested an abort
        SqliteABORT = 4,

        /// The database file is locked
        SqliteBUSY = 5,

        /// A table in the database is locked
        SqliteLOCKED = 6,

        /// A malloc() failed
        SqliteNOMEM = 7,

        /// Attempt to write a readonly database
        SqliteREADONLY = 8,

        /// Operation terminated by sqlite3_interrupt()
        SqliteINTERRUPT = 9,

        /// Some kind of disk I/O error occurred
        SqliteIOERR = 10,

        /// The database disk image is malformed
        SqliteCORRUPT = 11,

        /// Unknown opcode in sqlite3_file_control()
        SqliteNOTFOUND = 12,

        /// Insertion failed because database is full
        SqliteFULL = 13,

        /// Unable to open the database file
        SqliteCANTOPEN = 14,

        /// Database lock protocol error
        SqlitePROTOCOL = 15,

        /// Internal use only
        SqliteEMPTY = 16,

        /// The database schema changed
        SqliteSCHEMA = 17,

        /// String or BLOB exceeds size limit
        SqliteTOOBIG = 18,

        /// Abort due to constraint violation
        SqliteCONSTRAINT = 19,

        /// Data type mismatch
        SqliteMISMATCH = 20,

        /// Library used incorrectly
        SqliteMISUSE = 21,

        /// Uses OS features not supported on host
        SqliteNOLFS = 22,

        /// Authorization denied
        SqliteAUTH = 23,

        /// Not used
        SqliteFORMAT = 24,

        /// 2nd parameter to sqlite3_bind out of range
        SqliteRANGE = 25,

        /// File opened that is not a database file
        SqliteNOTADB = 26,

        /// Notifications from sqlite3_log()
        SqliteNOTICE = 27,

        /// Warnings from sqlite3_log()
        SqliteWARNING = 28,

        /// sqlite3_step() has another row ready
        SqliteROW = 100,

        /// sqlite3_step() has finished executing
        SqliteDONE = 101,
    }
}

/// asset result
pub type AssetResult<T> = std::result::Result<T, AssetStatusCode>;

#[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
/// auth type
pub enum AssetAuthType {
    /// None
    None = 0x00,
    /// any
    Any = 0xFF,
}

/// enum for AssetAccessibility
pub enum Accessibility {
    /// DevicePowerOn
    DevicePowerOn = 0,
    /// DevoceFirstUnlock
    DevoceFirstUnlock = 1,
    /// DeviceUnlock
    DeviceUnlock = 2,
    /// DeviceSecure
    DeviceSecure = 3,
}

/// AssetSyncType
pub enum AssetSyncType {
    /// None
    Never = 0,
    /// ThisDevice
    ThisDevice = 1 << 0,
    /// TrustedAccount
    TrustedAccount = 1 << 1,
    /// TrustedDevice
    TrustedDevice = 1 << 2,
}

/// AssetConflictPolicy
pub enum AssetConflictPolicy {
    /// OverRide
    OverRide = 0,
    /// Report
    Report = 1,
}

/// AssetReturnType
pub enum AssetReturnType {
    /// All
    All = 0,
    /// Attributes
    Attributes = 1,
}
