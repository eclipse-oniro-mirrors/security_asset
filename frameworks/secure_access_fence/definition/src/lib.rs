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
        /// Success.
        Success = 0,

        /// Evaluate deny.
        EvaluateDeny = 1,

        /// General error.
        GeneralError = 2,

        // ==================== IPC (0x10000) ====================
        /// The error code indicates that ipc write data failed.
        IpcWriteDataFail = 0x10001,

        /// The error code indicates that ipc read data failed.
        IpcReadDataFail = 0x10002,

        /// The error code indicates that ipc send request failed.
        IpcSendRequestFail = 0x10003,

        /// The error code indicates that ipc proxy failed.
        IpcProxyFail = 0x10004,

        /// The error code indicates that ipc error.
        IpcInvalidIpcCode = 0x10005,

        /// The error code indicates that invalid ipc code.
        IpcInvalidIpcCode = 0x10006,

        // ==================== SAMGR (0x12000) ====================
        /// The error code indicates that the SAF service is unavailable.
        ServiceUnavailable = 0x12001,

        /// The error code indicates that the service is stopping.
        ServiceIsStopping = 0x12002,

        // ==================== LIBDL (0x21000) ====================
        /// The error code indicates that dlopen failed.
        DlopenFail = 0x21001,

        /// The error code indicates that dlsym failed.
        DlsymFail = 0x21002,

        // ==================== ARGUMENT (0x30000) ====================
        /// The error code indicates that invalid array length.
        InvalidArrayLen = 0x30003,

        /// The error code indicates that null pointer.
        NullPtr = 0x30004,

        /// The error code indicates that argument is empty.
        ArgEmpty = 0x30019,

        /// The error code indicates that invalid os account id.
        InvalidOsAccountId = 0x3001A,

        // ==================== PERMISSION (0x32000) ====================
        /// The error code indicates that the caller doesn't have the permission.
        PermissionDenied = 0x32001,

        // ==================== COMMON (0x33000) ====================
        /// The error code indicates that data type mismatch.
        DataTypeMismatch = 0x33001,

        /// The error code indicates that hash map key not found.
        HashMapKeyNotFound = 0x33002,

        /// The error code indicates that base64 invalid length.
        Base64InvalidLen = 0x33003,

        /// The error code indicates that base64 invalid character.
        Base64InvalidChar = 0x33004,

        // ==================== CLI_TOOL (0x20000) ====================
        /// The error code indicates that the operation of calling Tool Service is failed.
        ToolError = 0x20001,

        // ==================== CRYPTO (0x18000) ====================
        /// The error code indicates that crypto operation failed.
        CryptoOperation = 0x18001,

        /// The error code indicates that invalid HMAC size.
        InvalidHmacSize = 0x18002,

        // ==================== PLUGIN (0x70000) ====================
        /// The error code indicates that create plugin manager failed.
        CreatePluginMgrFail = 0x70001,

        /// The error code indicates that plugin invalid event type.
        PluginInvalidEventType = 0x70002,

        /// The error code indicates that plugin not support.
        PluginNotSupport = 0x70003,

        // ==================== TICKET_OPERATION (0x71000) ====================
        /// The error code indicates that ticket key manager not support.
        TicketKeyMgrNotSupport = 0x71001,
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
