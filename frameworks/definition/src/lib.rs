/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use std::collections::HashMap;

mod extension;
#[macro_use]
pub mod macros;

impl_enum_trait! {
    /// An enum type containing the data type definitions for Asset attribute value.
    #[derive(Eq, PartialEq)]
    pub enum DataType {
        /// The data type of Asset attribute value is bool.
        Bool = 1 << 28,

        /// The data type of Asset attribute value is uint32.
        Number = 2 << 28,

        /// The data type of Asset attribute value is byte array.
        Bytes = 3 << 28,
    }
}

impl_tag_trait! {
    /// An emum type that indicates the tag of the asset attribute.
    #[derive(Clone, Copy)]
    #[derive(Debug)]
    #[derive(Eq, Hash, PartialEq)]
    pub enum Tag {
        /// A tag whose value is a byte array indicating the sensitive user data such as passwords and tokens.
        Secret = DataType::Bytes as isize | 0x01,

        /// A tag whose value is a byte array identifying an Asset.
        Alias = DataType::Bytes as isize | 0x02,

        /// A tag whose value is a 32-bit unsigned integer indicating when the Asset can be accessed.
        Accessibility = DataType::Number as isize | 0x03,

        /// A tag whose value is a bool indicating whether a screen lock password is set for the device.
        RequirePasswordSet = DataType::Bool as isize | 0x04,

        /// A tag whose value is a 32-bit unsigned integer indicating
        /// the user authentication type for Asset access control.
        AuthType = DataType::Number as isize | 0x05,

        /// A tag whose value is a 32-bit unsigned integer indicating
        /// the validity period in seconds of user authentication.
        AuthValidityPeriod = DataType::Number as isize | 0x06,

        /// A tag whose value is a byte array indicating the authentication challenge for anti-replay protection.
        AuthChallenge = DataType::Bytes as isize | 0x07,

        /// A tag whose value is a byte array indicating the authentication token after a user is verified.
        AuthToken = DataType::Bytes as isize | 0x08,

        /// A tag whose value is a 32-bit unsigned integer indicating the type of Asset synchronization.
        SyncType = DataType::Number as isize | 0x10,

        /// A tag whose value is a bool indicating whether Asset is stored persistently.
        IsPersistent = DataType::Bool as isize | 0x11,

        /// A tag whose value is a byte array indicating the first user-defined Asset data label (not allow to update).
        DataLabelCritical1 = DataType::Bytes as isize | 0x20,

        /// A tag whose value is a byte array indicating the second user-defined Asset data label (not allow to update).
        DataLabelCritical2 = DataType::Bytes as isize | 0x21,

        /// A tag whose value is a byte array indicating the third user-defined Asset data label (not allow to update).
        DataLabelCritical3 = DataType::Bytes as isize | 0x22,

        /// A tag whose value is a byte array indicating the fourth user-defined Asset data label (not allow to update).
        DataLabelCritical4 = DataType::Bytes as isize | 0x23,

        /// A tag whose value is a byte array indicating the first user-defined Asset data label (allow to update).
        DataLabelNormal1 = DataType::Bytes as isize | 0x30,

        /// A tag whose value is a byte array indicating the second user-defined Asset data label (allow to update).
        DataLabelNormal2 = DataType::Bytes as isize | 0x31,

        /// A tag whose value is a byte array indicating the third user-defined Asset data label (allow to update).
        DataLabelNormal3 = DataType::Bytes as isize | 0x32,

        /// A tag whose value is a byte array indicating the fourth user-defined Asset data label (allow to update).
        DataLabelNormal4 = DataType::Bytes as isize | 0x33,

        /// A local tag whose value is a byte array indicating
        /// the first user-defined Asset data label (allow to update).
        /// The information of a local tag will not be synchronized.
        DataLabelNormalLocal1 = DataType::Bytes as isize | 0x34,

        /// A local tag whose value is a byte array indicating
        /// the second user-defined Asset data label (allow to update).
        /// The information of a local tag will not be synchronized.
        DataLabelNormalLocal2 = DataType::Bytes as isize | 0x35,

        /// A local tag whose value is a byte array indicating
        /// the third user-defined Asset data label (allow to update).
        /// The information of a local tag will not be synchronized.
        DataLabelNormalLocal3 = DataType::Bytes as isize | 0x36,

        /// A local tag whose value is a byte array indicating
        /// the fourth user-defined Asset data label (allow to update).
        /// The information of a local tag will not be synchronized.
        DataLabelNormalLocal4 = DataType::Bytes as isize | 0x37,

        /// A tag whose value is a 32-bit unsigned integer indicating the return type of the queried Asset.
        ReturnType = DataType::Number as isize | 0x40,

        /// A tag whose value is a 32-bit unsigned integer indicating the maximum number of returned Assets in a query.
        ReturnLimit = DataType::Number as isize | 0x41,

        /// A tag whose value is a 32-bit unsigned integer indicating the offset of return data in batch query.
        ReturnOffset = DataType::Number as isize | 0x42,

        /// A tag whose value is a 32-bit unsigned integer indicating how the query results are sorted.
        ReturnOrderedBy = DataType::Number as isize | 0x43,

        /// A tag whose value is a 32-bit unsigned integer indicating the strategy for resolving Asset conflicts.
        ConflictResolution = DataType::Number as isize | 0x44,

        /// A tag whose value is a byte array indicating the update time of an Asset.
        UpdateTime = DataType::Bytes as isize | 0x45,

        /// A tag whose value is a byte array indicating the update time of an Asset.
        OperationType = DataType::Number as isize | 0x46,

        /// A tag whose value is a bool indicating whether the attributes of an asset are required to be encrypted.
        RequireAttrEncrypted = DataType::Bool as isize | 0x47,

        /// A tag whose value is a byte array indicating the group id an asset belongs to.
        GroupId = DataType::Bytes as isize | 0x48,

        /// A tag whose value is a 32-bit unsigned integer indicating the type of Asset encapsulation.
        WrapType = DataType::Number as isize | 0x49,

        /// A tag whose value is a 32-bit unsigned integer indicating the specific user id.
        UserId = DataType::Number as isize | 0x100,
    }
}

/// A type that indicates the secret or attribute value of an Asset tag.
#[derive(Clone)]
#[derive(Debug)]
#[derive(Eq, Hash, PartialEq)]
#[repr(C)]
pub enum Value {
    /// Asset attribute value, whose data type is bool.
    Bool(bool),

    /// Asset attribute value, whose data type is number.
    Number(u32),

    /// Asset attribute value, whose data type is byte array.
    Bytes(Vec<u8>),
}

impl Drop for Value {
    fn drop(&mut self) {
        if let Value::Bytes(bytes) = self {
            bytes.fill(0);
        }
    }
}

/// A Map type containing tag-value pairs that describe the attributes of an Asset.
pub type AssetMap = HashMap<Tag, Value>;

impl_enum_trait! {
    /// An enum type containing the Asset error codes.
    #[derive(Clone, Copy)]
    #[derive(Debug)]
    #[derive(Eq, Hash, PartialEq)]
    pub enum ErrCode {
        /// The error code indicates that the caller doesn't have the permission.
        PermissionDenied = 201,

        /// The error code indicates that the caller is not system application.
        NotSystemApplication = 202,

        /// The error code indicates that the argument is invalid.
        InvalidArgument = 401,

        /// The error code indicates that the ASSET service is unavailable.
        ServiceUnavailable = 24000001,

        /// The error code indicates that the queried Asset can not be found.
        NotFound = 24000002,

        /// The error code indicates that the Asset already exists.
        Duplicated = 24000003,

        /// The error code indicates that the access to Asset is denied.
        AccessDenied = 24000004,

        /// The error code indicates that the screen lock status mismatches.
        StatusMismatch = 24000005,

        /// The error code indicates insufficient memory.
        OutOfMemory = 24000006,

        /// The error code indicates that the Asset is corrupted.
        DataCorrupted = 24000007,

        /// The error code indicates that the database operation is failed.
        DatabaseError = 24000008,

        /// The error code indicates that the cryptography operation is failed.
        CryptoError = 24000009,

        /// The error code indicates that the ipc communication is abnormal.
        IpcError = 24000010,

        /// The error code indicates that the operation of calling Bundle Manager Service is failed.
        BmsError = 24000011,

        /// The error code indicates that the operation of calling OS Account Service is failed.
        AccountError = 24000012,

        /// The error code indicates that the operation of calling Access Token Service is failed.
        AccessTokenError = 24000013,

        /// The error code indicates that the operation of file is failed.
        FileOperationError = 24000014,

        /// The error code indicates that the operation of getting system time failed.
        GetSystemTimeError = 24000015,

        /// The error code indicates that the cache exceeds the limit.
        LimitExceeded = 24000016,

        /// The error code indicates that the capability is not supported.
        Unsupported = 24000017,

        /// The error code indicates that verifying the parameter failed.
        ParamVerificationFailed = 24000018,
    }
}

/// A struct containing the Asset result code and error message.
#[derive(Debug)]
pub struct AssetError {
    /// Error code for error occurred.
    pub code: ErrCode,

    /// Error message for error occurred.
    pub msg: String,
}

/// Alias of the Asset result type.
pub type Result<T> = std::result::Result<T, AssetError>;

impl_enum_trait! {
    /// An enum type indicates when the Asset is accessible.
    #[repr(C)]
    #[derive(Debug)]
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    #[derive(Default)]
    pub enum Accessibility {
        /// The secret value in the Asset can only be accessed after the device power on.
        DevicePowerOn = 0,

        /// The secret value in the Asset can only be accessed after the device is first unlocked.
        #[default]
        DeviceFirstUnlocked = 1,

        /// The secret value in the Asset can only be accessed while the device is unlocked.
        DeviceUnlocked = 2,
    }
}

impl_enum_trait! {
    /// An enum type indicates the user authentication type for Asset access control.
    #[derive(Debug)]
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    #[derive(Default)]
    pub enum AuthType {
        /// The access to an Asset doesn't require user authentication.
        #[default]
        None = 0x00,

        /// The access to an Asset requires user authentication using either PIN/pattern/password or biometric traits.
        Any = 0xFF,
    }
}

impl_enum_trait! {
    /// An enum type indicates the type of Asset synchronization.
    #[derive(Debug)]
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    #[derive(Default)]
    pub enum SyncType {
        /// An Asset with this attribute value is never allowed to be transferred out.
        #[default]
        Never = 0,

        /// An Asset with this attribute value can only be restored to the device from which it was transferred out.
        ThisDevice = 1 << 0,

        /// An Asset with this attribute value can only be transferred out to a trusted device (user authorized).
        TrustedDevice = 1 << 1,

        /// An Asset with this attribute value can only be transferred out to a trusted device (user authorized).
        TrustedAccount = 1 << 2,
    }
}

impl_enum_trait! {
    /// An enum type indicates the type of Asset synchronization.
    #[derive(Debug)]
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    #[derive(Default)]
    pub enum WrapType {
        /// An Asset with this attribute value is never allowed to be wrapped up.
        #[default]
        Never = 0,

        /// An Asset with this attribute value can only be wrapped or unwrapped on devices logged in with trusted accounts.
        TrustedAccount = 1,
    }
}

impl_enum_trait! {
    /// An enum type indicates the strategy for conflict resolution when handling duplicated Asset alias.
    #[derive(Default)]
    pub enum ConflictResolution {
        /// Directly overwrite an Asset with duplicated alias when a conflict is detected.
        Overwrite = 0,

        /// Throw an error so that the caller can take measures when a conflict is detected.
        #[default]
        ThrowError = 1,
    }
}

impl_enum_trait! {
    /// An enum type indicates the return type of the queried Asset.
    #[derive(Debug)]
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    #[derive(Default)]
    pub enum LocalStatus {
        /// Specify that the return data should contain both secret value and attributes.
        #[default]
        Local = 0,

        /// Specify that the return data contains only attributes.
        Cloud = 1 << 0,
    }
}

impl_enum_trait! {
    /// An enum type indicates the return type of the queried Asset.
    #[derive(Debug)]
    #[derive(Clone, Copy)]
    #[derive(PartialEq, Eq)]
    #[derive(Default)]
    pub enum SyncStatus {
        /// Specify that the return data should contain both secret value and attributes.
        #[default]
        NoNeedSync = 0,

        /// Specify that the return data contains only attributes.
        SyncAdd = 1 << 0,

        /// Specify that the return data contains only attributes.
        SyncDel = 1 << 1,

        /// Specify that the return data contains only attributes.
        SyncUpdate = 1 << 2,
    }
}

impl_enum_trait! {
    /// An enum type indicates the return type of the queried Asset.
    #[derive(Default)]
    pub enum ReturnType {
        /// Specify that the return data should contain both secret value and attributes.
        All = 0,

        /// Specify that the return data contains only attributes.
        #[default]
        Attributes = 1,
    }
}

impl_enum_trait! {
    /// An enum type indicates the return type of the queried Asset.
    #[derive(Default)]
    pub enum OperationType {
        /// Trigger Sync.
        #[default]
        NeedSync = 0,

        /// Logout to clean cloud flag.
        NeedLogout = 1,

        /// Delete cloud data.
        NeedDeleteCloudData = 2,
    }
}

/// Expended abililty for HashMap.
pub trait Extension<K> {
    /// Insert an attribute into the collection.
    fn insert_attr(&mut self, key: K, value: impl Conversion);

    /// Get an attribute of bool type from the collection.
    fn get_bool_attr(&self, key: &K) -> Result<bool>;

    /// Get an attribute of enum type from the collection.
    fn get_enum_attr<T: TryFrom<u32, Error = AssetError>>(&self, key: &K) -> Result<T>;

    /// Get an attribute of number type from the collection.
    fn get_num_attr(&self, key: &K) -> Result<u32>;

    /// Get an attribute of bytes type from the collection.
    fn get_bytes_attr(&self, key: &K) -> Result<&Vec<u8>>;
}

/// Conversion between a specific type and the Asset Value type.
pub trait Conversion {
    /// Get the data type of Asset Enum type.
    fn data_type(&self) -> DataType;

    /// Convert the Asset Enum type to the Value variant.
    fn into_value(self) -> Value;
}

/// The error of synchronization.
#[repr(C)]
#[derive(Default)]
pub struct SyncResult {
    /// The result code of synchronization.
    pub result_code: i32,
    /// The total count of synchronized Assets.
    pub total_count: u32,
    /// The count of Assets that fail to synchronize.
    pub failed_count: u32,
}
