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

//! This module extends the function of Asset data structure.

use std::{collections::HashMap, ffi, fmt::Display, hash::Hash, io};

use asset_log::loge;

use super::{
    impl_from_for_u32, log_throw_error, Accessibility, AssetError, AuthType, Conversion, DataType, ErrCode, Extension,
    Result, Tag, Value,
};

/// The mask used to obtain the data type of Asset attribute value.
const DATA_TYPE_MASK: u32 = 0xF << 28;

impl Conversion for Tag {
    fn data_type(&self) -> DataType {
        let mask = (*self as u32) & DATA_TYPE_MASK;
        match mask {
            _ if DataType::Bool as u32 == mask => DataType::Bool,
            _ if DataType::Number as u32 == mask => DataType::Number,
            _ if DataType::Bytes as u32 == mask => DataType::Bytes,
            _ => {
                panic!("Unexpected data type, it should be bool, uint32 or bytes.");
            },
        }
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl Conversion for Value {
    fn data_type(&self) -> DataType {
        match self {
            Value::Bool(_) => DataType::Bool,
            Value::Number(_) => DataType::Number,
            Value::Bytes(_) => DataType::Bytes,
        }
    }

    fn into_value(self) -> Value {
        self
    }
}

impl Conversion for Vec<u8> {
    fn data_type(&self) -> DataType {
        DataType::Bytes
    }

    fn into_value(self) -> Value {
        Value::Bytes(self)
    }
}

impl Conversion for bool {
    fn data_type(&self) -> DataType {
        DataType::Bool
    }

    fn into_value(self) -> Value {
        Value::Bool(self)
    }
}

impl Conversion for u32 {
    fn data_type(&self) -> DataType {
        DataType::Number
    }

    fn into_value(self) -> Value {
        Value::Number(self)
    }
}

impl<K> Extension<K> for HashMap<K, Value>
where
    K: Eq + PartialEq + Hash + std::fmt::Display,
{
    fn insert_attr(&mut self, key: K, value: impl Conversion) {
        self.insert(key, value.into_value());
    }

    fn get_bool_attr(&self, key: &K) -> Result<bool> {
        if let Some(Value::Bool(b)) = self.get(key) {
            Ok(*b)
        } else {
            log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get attribute of bool type failed, key: {}", key)
        }
    }

    fn get_enum_attr<T: TryFrom<u32, Error = AssetError>>(&self, key: &K) -> Result<T> {
        if let Some(Value::Number(num)) = self.get(key) {
            T::try_from(*num)
        } else {
            log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get attribute of enum type failed, key: {}", key)
        }
    }

    fn get_num_attr(&self, key: &K) -> Result<u32> {
        if let Some(Value::Number(num)) = self.get(key) {
            Ok(*num)
        } else {
            log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get attribute of number type failed, key: {}", key)
        }
    }

    fn get_bytes_attr(&self, key: &K) -> Result<&Vec<u8>> {
        if let Some(Value::Bytes(bytes)) = self.get(key) {
            Ok(bytes)
        } else {
            log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get attribute of bytes type failed, key: {}", key)
        }
    }
}

impl Display for AssetError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "[{}]: {}", self.code, self.msg)
    }
}

impl AssetError {
    /// Create an AssetError instance.
    pub fn new(code: ErrCode, msg: String) -> AssetError {
        loge!("{}", msg);
        AssetError { code, msg }
    }
}

impl From<io::Error> for AssetError {
    fn from(error: io::Error) -> Self {
        AssetError {
            code: (ErrCode::FileOperationError),
            msg: (format!("[FATAL]Backup db failed! error is [{error}]")),
        }
    }
}

impl From<ffi::NulError> for AssetError {
    fn from(error: ffi::NulError) -> Self {
        AssetError {
            code: (ErrCode::FileOperationError),
            msg: (format!("[FATAL] Null character found in string: {}", error)),
        }
    }
}

impl_from_for_u32!(AuthType);
impl_from_for_u32!(Accessibility);
