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

//! This module defines asset-related data structures.

#![allow(dead_code)]

mod extension;

use std::collections::HashMap;

use crate::impl_enum_trait;

/// An enum type containing the data type definitions for Asset attribute value.
pub enum DataType {
    /// The data type of Asset attribute value is uint32.
    Uint32 = 1 << 28,

    /// The data type of Asset attribute value is byte array.
    Bytes = 2 << 28,
}

impl_enum_trait! {
    /// An emum type that indicates the tag of the asset attribute.
    #[derive(Clone, Copy)]
    #[derive(Eq, Hash, PartialEq)]
    pub enum Tag {
        /// A tag whose value is a byte array indicating the sensitive user data such as passwords and tokens.
        Secret = DataType::Bytes as isize | 0x01,

        /// A tag whose value is a byte array identifying an Asset.
        Alias = DataType::Bytes as isize | 0x02,

        /// A tag whose value is a 32-bit unsigned integer indicating when the Asset can be accessed.
        Accessibility = DataType::Uint32 as isize | 0x03,

        /// A tag whose value is a 32-bit unsigned integer indicating
        /// the user authentication type for Asset access control.
        AuthType = DataType::Uint32 as isize | 0x04,

        /// A tag whose value is a 32-bit unsigned integer indicating
        /// the validity period in seconds of user authentication.
        AuthValidityPeriod = DataType::Uint32 as isize | 0x05,

        /// A tag whose value is a byte array indicating the authentication challenge for anti-replay protection.
        AuthChallenge = DataType::Bytes as isize | 0x06,

        /// A tag whose value is a byte array indicating the authentication token after a user is verified.
        AuthToken = DataType::Bytes as isize | 0x07,

        /// A tag whose value is a 32-bit unsigned integer indicating the type of Asset synchronization.
        SyncType = DataType::Uint32 as isize | 0x08,

        /// A tag whose value is a 32-bit unsigned integer indicating the strategy for resolving Asset conflicts.
        ConfictPolicy = DataType::Uint32 as isize | 0x09,

        /// A tag whose value is a byte array indicating the first user-defined Asset data label (not allow to update).
        DataLabelCritical1 = DataType::Bytes as isize | 0x0A,

        /// A tag whose value is a byte array indicating the second user-defined Asset data label (not allow to update).
        DataLabelCritical2 = DataType::Bytes as isize | 0x0B,

        /// A tag whose value is a byte array indicating the third user-defined Asset data label (not allow to update).
        DataLabelCritical3 = DataType::Bytes as isize | 0x0C,

        /// A tag whose value is a byte array indicating the fourth user-defined Asset data label (not allow to update).
        DataLabelCritical4 = DataType::Bytes as isize | 0x0D,

        /// A tag whose value is a byte array indicating the first user-defined Asset data label (allow to update).
        DataLabelNormal1 = DataType::Bytes as isize | 0x0E,

        /// A tag whose value is a byte array indicating the second user-defined Asset data label (allow to update).
        DataLabelNormal2 = DataType::Bytes as isize | 0x0F,

        /// A tag whose value is a byte array indicating the third user-defined Asset data label (allow to update).
        DataLabelNormal3 = DataType::Bytes as isize | 0x10,

        /// A tag whose value is a byte array indicating the fourth user-defined Asset data label (allow to update).
        DataLabelNormal4 = DataType::Bytes as isize | 0x11,

        /// A tag whose value is a 32-bit unsigned integer indicating the return type of the queried Asset.
        ReturnType = DataType::Uint32 as isize | 0x12,

        /// A tag whose value is a 32-bit unsigned integer indicating the maximum number of returned Assets in a query.
        ReturnLimit = DataType::Uint32 as isize | 0x13,

        /// A tag whose value is a 32-bit unsigned integer indicating the offset of return data in batch query.
        ReturnOffset = DataType::Uint32 as isize | 0x14,

        /// A tag whose value is a 32-bit unsigned integer indicating how the query results are sorted.
        ReturnOrderBy = DataType::Uint32 as isize | 0x15,
    }
}

/// A type that indicates the secret or attribute value of an Asset tag.
pub enum Value {
    /// Asset attribute value, whose data type is number.
    Number(u32),

    /// Asset attribute value, whose data type is byte array.
    Bytes(Vec<u8>),
}

/// A Map type containing tag-value pairs that describe the attributes of an Asset.
pub type AssetMap = HashMap<Tag, Value>;

impl_enum_trait! {
    /// An enum type containing the Asset result codes.
    #[derive(Clone, Copy)]
    #[derive(Debug)]
    #[derive(Eq, Hash, PartialEq)]
    pub enum ErrCode {
        /// failed, todo delete
        Failed = -1,

        /// The error code indicates that the caller doesn't have permission to operate.
        PermissionDenied = 201,

        /// The error code indicates that the argument is invalid.
        InvalidArgument = 401,

        /// The error code indicates that the capability is not supported.
        NotSupport = 801,

        /// The error code indicates that the Asset service is unavailable.
        ServiceUnvailable = 24000001,

        /// The error code indicates that the queried Asset can not be found.
        NotFound = 24000002,

        /// The error code indicates that the added Asset already exists.
        Duplicated = 24000003,

        /// The error code indicates that the access to Asset is denied.
        AccessDenied = 24000004,

        /// The error code indicates that the authentication token has expired.
        AuthTokenExpired = 24000005,

        /// The error code indicates insufficient memory.
        OutOfMemory = 24000006,

        /// The error code indicates that the Asset or encryption key is corrupted.
        DataCorrupted = 24000007,

        /// The error code indicates that the ipc communication is abnormal.
        IpcError = 24000008,

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

/// Alias of the Asset result type.
pub type Result<T> = std::result::Result<T, ErrCode>;

impl_enum_trait! {
    /// An enum type indicates when the Asset is accessible.
    pub enum Accessibility {
        /// The secret value in the Asset can only be accessed after the device is first unlocked.
        DevoceFirstUnlock = 1,

        /// The secret value in the Asset can only be accessed while the device is unlocked.
        DeviceUnlock = 2,

        /// The secret value in the Asset can only be accessed
        /// when the device is unlocked and a PIN/pattern/password is set on the device.
        DeviceSecure = 3,
    }
}

impl_enum_trait! {
    /// An enum type indicates the user authentication type for Asset access control.
    pub enum AuthType {
        /// The access to an Asset doesn't require user authentication.
        None = 0x00,

        /// The access to an Asset requires user authentication using either PIN/pattern/password or biometric traits.
        Any = 0xFF,
    }
}

impl_enum_trait! {
    /// An enum type indicates the type of Asset synchronization.
    pub enum SyncType {
        /// An Asset with this attribute value is never allowed to be transferred out.
        Never = 0,

        /// An Asset with this attribute value can only be restored to the device from which it was transferred out.
        ThisDevice = 1 << 0,

        /// An Asset with this attribute value can only be transferred out to a device of trusted account.
        TrustedAccount = 1 << 1,

        /// An Asset with this attribute value can only be transferred out to a trusted device (user authorized).
        TrustedDevice = 1 << 2,
    }
}

impl_enum_trait! {
    /// An enum type indicates the strategy for conflict resolution when handling duplicated Asset alias.
    pub enum ConflictResolution {
        /// Directly overwrite an Asset with duplicated alias when a conflict is detected.
        Overwrite = 0,

        /// Throw an error so that the caller can take measures when a conflict is detected.
        ThrowError = 1,
    }
}

impl_enum_trait! {
    /// An enum type indicates the return type of the queried Asset.
    pub enum ReturnType {
        /// Specify that the return data should contain both secret value and attributes.
        All = 0,

        /// Specify that the return data contains only attributes.
        Attributes = 1,
    }
}

/// The asset version.
pub struct Version {
    /// The major version.
    major: u32,

    /// The minor version.
    minor: u32,

    /// The patch version.
    patch: u32,
}

/// Automatically convert the input value to Asset Value, then insert into the collection.
pub trait Insert {
    /// Insert an attribute into the collection.
    fn insert_attr(&mut self, key: Tag, value: impl IntoValue) -> Result<()>;
}

/// Convert a specific type to the Asset Value type.
pub trait IntoValue {
    /// Get the data type of Asset Enum type.
    fn data_type(&self) -> DataType;
    /// Convert the Asset Enum type to the Value variant.
    fn into_value(self) -> Value;
}