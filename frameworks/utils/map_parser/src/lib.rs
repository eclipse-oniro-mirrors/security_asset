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

//! This crate implements the sha256.

use asset_definition::{AssetMap, Tag, Result, Value, ErrCode};

/// Parse bytes from Asset map.
pub fn get_bytes(attrs: &AssetMap, tag: Tag) -> Result<&Vec<u8>> {
    if let Some(Value::Bytes(bytes)) = attrs.get(&tag) {
        Ok(bytes)
    } else {
        Err(ErrCode::InvalidArgument)
    }
}

/// Parse number from Asset map.
pub fn get_number(attrs: &AssetMap, tag: Tag) -> Result<u32> {
    if let Some(Value::Number(num)) = attrs.get(&tag) {
        Ok(*num)
    } else {
        Err(ErrCode::InvalidArgument)
    }
}

/// Parse enum from Asset map.
pub fn get_enum_variant<T: TryFrom<u32, Error = ErrCode>>(attrs: &AssetMap, tag: Tag) -> Result<T> {
    if let Some(Value::Number(num)) = attrs.get(&tag) {
        T::try_from(*num)
    } else {
        Err(ErrCode::InvalidArgument)
    }
}

/// Parse bool from Asset map.
pub fn get_bool(attrs: &AssetMap, tag: Tag) -> Result<bool> {
    if let Some(Value::Bool(b)) = attrs.get(&tag) {
        Ok(*b)
    } else {
        Err(ErrCode::InvalidArgument)
    }
}