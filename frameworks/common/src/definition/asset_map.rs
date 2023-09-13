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

//! map的各类操作

// use ipc_rust::BorrowedMsgParcel;

use super::asset_type_transform::GetType;
use crate::definition::{Tag, Value, Result, ErrCode, DataType, AssetMap};
use crate::loge;

/// xxx
pub trait InsertAttribute {
    ///
    fn insert_attr(&mut self, key: Tag, value: impl GetType) -> Result<()>;
}

impl InsertAttribute for AssetMap {
    fn insert_attr(&mut self, key: Tag, value: impl GetType) -> Result<()> {
        match value.data_type() {
            DataType::Uint32 => {
                if let Value::NUMBER(real) = value.into_value() {
                    self.insert(key, Value::NUMBER(real));
                    return Ok(());
                }
                loge!("InsertAttribute u32 failed!");
                Err(ErrCode::InvalidArgument)
            },
            DataType::Bytes => {
                if let Value::Bytes(real) = value.into_value() {
                    self.insert(key, Value::Bytes(real));
                    return Ok(());
                }
                loge!("InsertAttribute byte failed!");
                Err(ErrCode::InvalidArgument)
            },
        }
    }
}
