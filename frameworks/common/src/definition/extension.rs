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

//! This module extends the function of Asset data structure.

use super::{
    AssetMap, DataType, ErrCode, IntoValue, Insert,
    Result, Tag, Value, Accessibility, AuthType,
    ConflictResolution, ReturnType, SyncType
};
use crate::loge;

/// The mask used to obtain the data type of Asset attribute value.
const DATA_TYPE_MASK: u32 = 0xF << 28;

impl IntoValue for Tag {
    fn data_type(&self) -> DataType {
        let mask = (*self as u32) & DATA_TYPE_MASK;
        match mask {
            _ if DataType::Bool as u32 == mask => DataType::Bool,
            _ if DataType::Uint32 as u32 == mask => DataType::Uint32,
            _ if DataType::Bytes as u32 == mask => DataType::Bytes,
            _ => {
                panic!("Unexpected action, data type should be uint32 or bytes");
            },
        }
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl IntoValue for Accessibility {
    fn data_type(&self) -> DataType {
        DataType::Uint32
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl IntoValue for SyncType {
    fn data_type(&self) -> DataType {
        DataType::Uint32
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl IntoValue for ConflictResolution {
    fn data_type(&self) -> DataType {
        DataType::Uint32
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl IntoValue for ReturnType {
    fn data_type(&self) -> DataType {
        DataType::Uint32
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl IntoValue for AuthType {
    fn data_type(&self) -> DataType {
        DataType::Uint32
    }

    fn into_value(self) -> Value {
        Value::Number(self as u32)
    }
}

impl IntoValue for Vec<u8> {
    fn data_type(&self) -> DataType {
        DataType::Bytes
    }

    fn into_value(self) -> Value {
        Value::Bytes(self)
    }
}

impl IntoValue for bool {
    fn data_type(&self) -> DataType {
        DataType::Bool
    }

    fn into_value(self) -> Value {
        Value::Bool(self)
    }
}

impl Insert for AssetMap {
    fn insert_attr(&mut self, key: Tag, value: impl IntoValue) -> Result<()> {
        match value.data_type() {
            DataType::Bool => {
                if let Value::Bool(real) = value.into_value() {
                    self.insert(key, Value::Bool(real));
                    return Ok(());
                }
                loge!("Insert bool failed!");
                Err(ErrCode::InvalidArgument)
            }
            DataType::Uint32 => {
                if let Value::Number(real) = value.into_value() {
                    self.insert(key, Value::Number(real));
                    return Ok(());
                }
                loge!("Insert u32 failed!");
                Err(ErrCode::InvalidArgument)
            },
            DataType::Bytes => {
                if let Value::Bytes(real) = value.into_value() {
                    self.insert(key, Value::Bytes(real));
                    return Ok(());
                }
                loge!("Insert byte failed!");
                Err(ErrCode::InvalidArgument)
            },
        }
    }
}
