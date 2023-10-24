/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use asset_sdk::{
    AssetMap, Tag, Value, ErrCode, Result, ReturnType
};

pub(crate) fn get_bytes(input: &AssetMap, tag: Tag) -> Result<&Vec<u8>> {
    if let Some(Value::Bytes(bytes)) = input.get(&tag) {
        return Ok(bytes)
    }
    Err(ErrCode::NotFound)
}

pub(crate) fn get_number(input: &AssetMap, tag: Tag) -> Result<u32> {
    if let Some(Value::Number(num)) = input.get(&tag) {
        return Ok(*num)
    }
    Err(ErrCode::NotFound)
}

// pub(crate) fn get_bool(input: &AssetMap, tag: Tag) -> Result<bool> {
//     if let Some(Value::Bool(b)) = input.get(&tag) {
//         return Ok(*b)
//     }
//     Err(ErrCode::NotFound)
// }

pub(crate) fn remove_by_alias(alias: &[u8]) -> Result<()> {
    asset_sdk::Manager::build()?.remove(&AssetMap::from([(Tag::Alias, Value::Bytes(alias.to_vec()))]))
}

pub(crate) fn query_all_by_alias(alias: &[u8]) -> Result<Vec<AssetMap>> {
    asset_sdk::Manager::build()?.query(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::ReturnType, Value::Number(ReturnType::All as u32)),
    ]))
}

pub(crate) fn query_attr_by_alias(alias: &[u8]) -> Result<Vec<AssetMap>> {
    asset_sdk::Manager::build()?.query(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::ReturnType, Value::Number(ReturnType::Attributes as u32)),
    ]))
}

pub(crate) fn add_default_asset(alias: &[u8], secret: &[u8]) -> Result<()> {
    asset_sdk::Manager::build()?.add(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::Secret, Value::Bytes(secret.to_vec())),
    ]))
}