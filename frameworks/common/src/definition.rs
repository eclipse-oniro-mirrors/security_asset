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

mod extension;

use std::collections::HashMap;

use crate::impl_enum_trait;

/// An enum type containing the data type definitions for Asset attribute value.
pub enum DataType {
    /// The data type of Asset attribute value is bool.
    Bool = 1 << 28,

    /// The data type of Asset attribute value is uint32.
    Uint32 = 2 << 28,

    /// The data type of Asset attribute value is byte array.
    Bytes = 3 << 28,
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

        /// A tag whose value is a bool indicating whether a screen lock password is set for the device.
        RequirePasswordSet = DataType::Bool as isize | 0x04,

        /// A tag whose value is a 32-bit unsigned integer indicating
        /// the user authentication type for Asset access control.
        AuthType = DataType::Uint32 as isize | 0x05,

        /// A tag whose value is a 32-bit unsigned integer indicating
        /// the validity period in seconds of user authentication.
        AuthValidityPeriod = DataType::Uint32 as isize | 0x06,

        /// A tag whose value is a byte array indicating the authentication challenge for anti-replay protection.
        AuthChallenge = DataType::Bytes as isize | 0x07,

        /// A tag whose value is a byte array indicating the authentication token after a user is verified.
        AuthToken = DataType::Bytes as isize | 0x08,

        /// A tag whose value is a 32-bit unsigned integer indicating the type of Asset synchronization.
        SyncType = DataType::Uint32 as isize | 0x10,

        /// A tag whose value is a 32-bit unsigned integer indicating the strategy for resolving Asset conflicts.
        ConflictResolution = DataType::Uint32 as isize | 0x11,

        /// A tag whose value is a byte array indicating the first user-defined Asset data label (not allow to update).
        DataLabelCritical1 = DataType::Bytes as isize | 0x20,

        /// A tag whose value is a byte array indicating the second user-defined Asset data label (not allow to update).
        DataLabelCritical2 = DataType::Bytes as isize | 0x21,

        /// A tag whose value is a byte array indicating the third user-defined Asset data label (not allow to update).
        DataLabelCritical3 = DataType::Bytes as isize | 0x22,

        /// A tag whose value is a byte array indicating the fourth user-defined Asset data label (not allow to update).
        DataLabelCritical4 = DataType::Bytes as isize | 0x23,

        /// A tag whose value is a byte array indicating the first user-defined Asset data label (allow to update).
        DataLabelNormal1 = DataType::Bytes as isize | 0x24,

        /// A tag whose value is a byte array indicating the second user-defined Asset data label (allow to update).
        DataLabelNormal2 = DataType::Bytes as isize | 0x25,

        /// A tag whose value is a byte array indicating the third user-defined Asset data label (allow to update).
        DataLabelNormal3 = DataType::Bytes as isize | 0x26,

        /// A tag whose value is a byte array indicating the fourth user-defined Asset data label (allow to update).
        DataLabelNormal4 = DataType::Bytes as isize | 0x27,

        /// A tag whose value is a 32-bit unsigned integer indicating the return type of the queried Asset.
        ReturnType = DataType::Uint32 as isize | 0x30,

        /// A tag whose value is a 32-bit unsigned integer indicating the maximum number of returned Assets in a query.
        ReturnLimit = DataType::Uint32 as isize | 0x31,

        /// A tag whose value is a 32-bit unsigned integer indicating the offset of return data in batch query.
        ReturnOffset = DataType::Uint32 as isize | 0x32,

        /// A tag whose value is a 32-bit unsigned integer indicating how the query results are sorted.
        ReturnOrderBy = DataType::Uint32 as isize | 0x33,
    }
}

/// A type that indicates the secret or attribute value of an Asset tag.
#[derive(Clone)]
#[repr(C)]
pub enum Value {
    /// Asset attribute value, whose data type is bool.
    Bool(bool),

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

        /// The error code indicates that the screen lock status mismatches.
        StatusMismatch = 24000006,

        /// The error code indicates insufficient memory.
        OutOfMemory = 24000007,

        /// The error code indicates that the Asset or encryption key is corrupted.
        DataCorrupted = 24000008,

        /// The error code indicates that the ipc communication is abnormal.
        IpcError = 24000009,

        /// The error code indicates that the database operation is failed.
        SqliteError = 24000010,

        /// The error code indicates that the operation of calling bundle manager service is failed.
        BmsError = 24000011,

        /// The error code indicates that the cryptography operation is failed.
        CryptoError = 24000012,

        /// The error code indicates that the operation of calling OS account service is failed.
        AccountError = 24000013,

        /// The error code indicates that the operation of calling common event service is failed.
        CommonEventError = 24000014,

        /// The error code indicates that the operation of calling access token service is failed.
        AccessTokenError = 24000015,

        /// The error code indicates that the operation of file is failed.
        FileOperationError = 24000016,

        /// The error code indicates that the operation of getting system time failed.
        SystemTimeError = 24000017,

        /// The error code indicates that the amount of map element or other limited quotas exceed the limit.
        ExceedLimit = 24000018,
    }
}

/// Alias of the Asset result type.
pub type Result<T> = std::result::Result<T, ErrCode>;

impl_enum_trait! {
    /// An enum type indicates when the Asset is accessible.
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    pub enum Accessibility {
        /// The secret value in the Asset can only be accessed after the device is first unlocked.
        DeviceFirstUnlock = 1,

        /// The secret value in the Asset can only be accessed while the device is unlocked.
        DeviceUnlock = 2,
    }
}

impl_enum_trait! {
    /// An enum type indicates the user authentication type for Asset access control.
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
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
#[repr(C)]
pub struct Version {
    /// The major version.
    pub major: u32,

    /// The minor version.
    pub minor: u32,

    /// The patch version.
    pub patch: u32,
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