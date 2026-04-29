/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! This module defines SAF-related data structures.

use std::collections::HashMap;

mod extension;
#[macro_use]
pub mod macros;
pub mod macros_lib;

impl_enum_trait! {
    /// An enum type containing the data type definitions for SAF attribute value.
    #[derive(Clone, Copy)]
    #[derive(Eq, PartialEq)]
    pub enum DataType {
        /// The data type of SAF attribute value is bool.
        Bool = 1 << 28,

        /// The data type of SAF attribute value is uint32.
        Number = 2 << 28,

        /// The data type of SAF attribute value is byte array.
        Bytes = 3 << 28,
    }
}

impl_tag_trait! {
    /// An emum type that indicates the tag of the SAF attribute.
    #[derive(Clone, Copy)]
    #[derive(Debug)]
    #[derive(Eq, Hash, PartialEq)]
    pub enum Tag {
        /// A tag whose value is a byte array indicating the authentication token after a user is verified.
        AuthToken = DataType::Bytes as isize | 0x01,

        /// A tag whose value is a byte array indicating the compation device id.
        CompationDeviceId = DataType::Bytes as isize | 0x02,

        /// A tag whose value is a number indicating the authentication trust level.
        AuthTrustLevel = DataType::Number as isize | 0x03,

        /// A tag whose value is a byte array indicating the device id.
        DeviceId = DataType::Bytes as isize | 0x04,

        /// A tag whose value is a byte array indicating the access bundle name.
        AccessBundleName = DataType::Bytes as isize | 0x05,

        /// A tag whose value is a byte array indicating the caller bundle name.
        CallerBundleName = DataType::Bytes as isize | 0x06,
    }
}

/// A type that indicates the secret or attribute value of an SAF tag.
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

/// A Map type containing tag-value pairs that describe the attributes of an SAF.
pub type SAFMap = HashMap<Tag, Value>;

impl_enum_trait! {
    /// An enum type containing the SAF error codes.
    #[derive(Clone, Copy)]
    #[derive(Debug)]
    #[derive(Eq, Hash, PartialEq)]
    pub enum ErrCode {
        /// The error code indicates that the caller doesn't have the permission.
        PermissionDenied = 201,

        /// The error code indicates that the caller is not system application.
        NotSystemApplication = 202,

        /// The error code indicates that the SAF service is unavailable.
        ServiceUnavailable = 1023900001,

        /// The error code indicates that the ipc communication is abnormal.
        IpcError = 1023900002,

        /// The error code indicates that the operation of calling Bundle Manager Service is failed.
        BmsError = 1023900003,

        /// The error code indicates that the operation of calling OS Account Service is failed.
        AccountError = 1023900004,

        /// The error code indicates that the operation of calling userIAM Service is failed.
        UserIAMError = 1023900005,

        /// The error code indicates that verifying the parameter failed.
        ParamVerificationFailed = 1023900006,

        /// The error code indicates that file operation failed.
        FileOperationError = 1023900007,

        /// The error code indicates that the operation of calling Tool Service is failed.
        ToolError = 1023900008,
    }
}

/// A struct containing the SAF result code and error message.
#[derive(Clone, Debug)]
pub struct SAFError {
    /// Error code for error occurred.
    pub code: ErrCode,

    /// Error message for error occurred.
    pub msg: String,
}

/// Alias of the SAF result type.
pub type Result<T> = std::result::Result<T, SAFError>;

/// Expended abililty for HashMap.
pub trait Extension<K> {
    /// Insert an attribute into the collection.
    fn insert_attr(&mut self, key: K, value: impl Conversion);

    /// Get an attribute of bool type from the collection.
    fn get_bool_attr(&self, key: &K) -> Result<bool>;

    /// Get an attribute of enum type from the collection.
    fn get_enum_attr<T: TryFrom<u32, Error = SAFError>>(&self, key: &K) -> Result<T>;

    /// Get an attribute of number type from the collection.
    fn get_num_attr(&self, key: &K) -> Result<u32>;

    /// Get an attribute of bytes type from the collection.
    fn get_bytes_attr(&self, key: &K) -> Result<&Vec<u8>>;
}

/// Conversion between a specific type and the SAF Value type.
pub trait Conversion {
    /// Get the data type of SAF Enum type.
    fn data_type(&self) -> DataType;

    /// Convert the SAF Enum type to the Value variant.
    fn into_value(self) -> Value;
}
