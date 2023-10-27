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

use asset_sdk::*;
use crate::common::*;

#[test]
fn add_empty_attr() {
    let attrs = AssetMap::new();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_without_alias() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_min_len() {
    let function_name = function!().as_bytes();
    let alias = vec![0; 1];
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.clone()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

   remove_by_alias(&alias).unwrap();
}

#[test]
fn add_alias_with_max_len() {
    let function_name = function!().as_bytes();
    let alias = vec![0; 256];
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.clone()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

   remove_by_alias(&alias).unwrap();
}

#[test]
fn add_alias_with_short_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, vec![]).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_long_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, vec![0; 257]).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::Alias, 0).unwrap_err());
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::Alias, true).unwrap_err());

    attrs.insert(Tag::Alias, Value::Number(0));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert(Tag::Alias, Value::Bool(true));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_without_secret() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_min_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, vec![0; 1]).unwrap();
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

   remove_by_alias(function_name).unwrap();
}

#[test]
fn add_secret_with_max_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, vec![0; 1024]).unwrap();
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

   remove_by_alias(function_name).unwrap();
}

#[test]
fn add_secret_with_short_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, vec![]).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_long_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, vec![0; 1025]).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::Secret, 0).unwrap_err());
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::Secret, true).unwrap_err());

    attrs.insert(Tag::Secret, Value::Number(0));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert(Tag::Secret, Value::Bool(true));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_accessibility() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Accessibility, 0).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Accessibility, 3).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_accessibility_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::Accessibility, vec![]).unwrap_err());
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::Accessibility, true).unwrap_err());

    attrs.insert(Tag::Accessibility, Value::Bytes(vec![]));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert(Tag::Accessibility, Value::Bool(true));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_required_pwd_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::RequirePasswordSet, vec![]).unwrap_err());
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::RequirePasswordSet, 0).unwrap_err());

    attrs.insert(Tag::RequirePasswordSet, Value::Bytes(vec![]));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert(Tag::RequirePasswordSet, Value::Number(0));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_auth_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::AuthType, 1).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::AuthType, 256).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_auth_type_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, function_name.to_owned()).unwrap();
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::AuthType, vec![]).unwrap_err());
    assert_eq!(ErrCode::InvalidArgument, attrs.insert_attr(Tag::AuthType, true).unwrap_err());

    attrs.insert(Tag::AuthType, Value::Bytes(vec![]));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert(Tag::AuthType, Value::Bool(true));
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

