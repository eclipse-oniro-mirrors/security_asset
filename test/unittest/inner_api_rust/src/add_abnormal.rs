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

use crate::common::*;
use asset_sdk::*;

#[test]
fn add_empty_attr() {
    let attrs = AssetMap::new();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_without_alias() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_min_len() {
    let function_name = function!().as_bytes();
    let alias = vec![0; 1];
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.clone());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    query_attr_by_alias(&alias).unwrap();
    remove_by_alias(&alias).unwrap();
}

#[test]
fn add_alias_with_max_len() {
    let function_name = function!().as_bytes();
    let alias = vec![0; 256];
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.clone());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    query_attr_by_alias(&alias).unwrap();
    remove_by_alias(&alias).unwrap();
}

#[test]
fn add_alias_with_short_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, vec![]);
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_long_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, vec![0; 257]);
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Alias, 0);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Alias, true);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_without_secret() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_min_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, vec![0; 1]);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_secret_with_max_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, vec![0; 1024]);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_secret_with_short_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, vec![]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_long_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, vec![0; 1025]);
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, 0);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Secret, true);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_accessibility() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, 0);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Accessibility, 3);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_accessibility_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, vec![]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Accessibility, true);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_required_pwd_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::RequirePasswordSet, vec![]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::RequirePasswordSet, 0);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_auth_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::AuthType, 1);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::AuthType, 256);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_auth_type_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::AuthType, vec![]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::AuthType, true);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_sync_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::AuthType, 8);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_sync_type_with_max_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::AuthType, 7);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_sync_type_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::SyncType, vec![]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::SyncType, true);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_delete_type_with_max_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::DeleteType, 3);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_delete_type_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::DeleteType, vec![]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::DeleteType, true);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}
