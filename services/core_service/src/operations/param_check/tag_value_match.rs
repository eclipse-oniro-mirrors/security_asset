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

//! This file implement the asset param check

use asset_common::{
    definition::{AssetMap, DataType, ErrCode, Result, Tag, Value, IntoValue},
    loge};

fn tag_value_match(tag: &Tag, value: &Value) -> bool
{
    match tag.data_type() {
        DataType::Bytes => {
            if let Value::Bytes(_) = value {
                return true;
            }
        },
        DataType::Uint32 => {
            if let Value::Number(_) = value {
                return true;
            }
        },
    }
    false
}

pub(crate) fn check_tag_value_match(params: &AssetMap) -> Result<()>
{
    for (tag, value) in params.iter() {
        if !tag_value_match(tag, value) {
            loge!("tag [{}] and its value type match failed!", tag);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}
