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
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_without_alias() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_min_len() {
    let function_name = function!().as_bytes();
    let alias = vec![0; MIN_ARRAY_SIZE + 1];
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.clone());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    query_attr_by_alias(&alias).unwrap();
    remove_by_alias(&alias).unwrap();
}

#[test]
fn add_alias_with_max_len() {
    let function_name = function!().as_bytes();
    let alias = vec![0; MAX_ALIAS_SIZE];
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.clone());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    query_attr_by_alias(&alias).unwrap();
    remove_by_alias(&alias).unwrap();
}

#[test]
fn add_invalid_alias() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, vec![]);
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Alias, vec![0; MAX_ALIAS_SIZE + 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_alias_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Alias, 0);
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Alias, true);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_without_secret() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_min_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, vec![0; MIN_ARRAY_SIZE + 1]);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    query_attr_by_alias(function_name).unwrap();
    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_secret_with_max_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::Secret, vec![0; MAX_SECRET_SIZE]);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_invalid_secret() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, vec![]);
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Secret, vec![0; MAX_SECRET_SIZE + 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_secret_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, 0);
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Secret, true);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_accessibility() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, (Accessibility::DeviceUnlocked as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Accessibility, Accessibility::DeviceUnlocked);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::Accessibility, Accessibility::DeviceFirstUnlocked);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_auth_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::AuthType, (AuthType::None as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::AuthType, (AuthType::Any as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_sync_type() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    let sync_type = SyncType::ThisDevice as u32 | SyncType::TrustedDevice as u32;
    attrs.insert_attr(Tag::SyncType, sync_type + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_sync_type_with_max_len() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    let sync_type = SyncType::ThisDevice as u32 | SyncType::TrustedDevice as u32;
    attrs.insert_attr(Tag::SyncType, sync_type);
    assert!(asset_sdk::Manager::build().unwrap().add(&attrs).is_ok());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_invalid_conflict_resolution() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::ConflictResolution, (ConflictResolution::ThrowError as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}

#[test]
fn add_invalid_label() {
    let function_name = function!().as_bytes();
    let labels = &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    for &label in labels {
        let mut attrs = AssetMap::new();
        attrs.insert_attr(Tag::Alias, function_name.to_owned());
        attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
        attrs.insert_attr(Tag::Secret, function_name.to_owned());
        attrs.insert_attr(label, vec![]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

        attrs.insert_attr(label, vec![0; MAX_LABEL_SIZE + 1]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
    }
}

#[test]
fn add_bool_tag_with_unmatched_type() {
    let tags = [Tag::RequirePasswordSet, Tag::IsPersistent];
    let function_name = function!().as_bytes();
    for tag in tags {
        let mut attrs = AssetMap::new();
        attrs.insert_attr(Tag::Alias, function_name.to_owned());
        attrs.insert_attr(Tag::Secret, function_name.to_owned());
        attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
        attrs.insert_attr(tag, vec![]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

        attrs.insert_attr(tag, 0);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
    }
}

#[test]
fn add_bytes_tag_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let labels = &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    for &label in labels {
        let mut attrs = AssetMap::new();
        attrs.insert_attr(Tag::Alias, function_name.to_owned());
        attrs.insert_attr(Tag::Secret, function_name.to_owned());
        attrs.insert_attr(label, 0);
        attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

        attrs.insert_attr(label, true);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
    }
}

#[test]
fn add_number_tag_with_unmatched_type() {
    let tags_num = [Tag::Accessibility, Tag::AuthType, Tag::SyncType, Tag::ConflictResolution];
    for tag in tags_num {
        let mut attrs = AssetMap::new();
        attrs.insert_attr(tag, vec![]);
        attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

        attrs.insert_attr(tag, true);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
    }
}

#[test]
fn add_unsupported_tags() {
    let function_name = function!().as_bytes();
    let tags_bytes = [Tag::AuthChallenge, Tag::AuthToken];
    for tag in tags_bytes {
        let mut attrs = AssetMap::new();
        attrs.insert_attr(Tag::Alias, function_name.to_owned());
        attrs.insert_attr(Tag::Secret, function_name.to_owned());
        attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
        attrs.insert_attr(tag, vec![0; MIN_ARRAY_SIZE + 1]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
    }

    let tags_num =
        [Tag::AuthValidityPeriod, Tag::ReturnLimit, Tag::ReturnOffset, Tag::ReturnOrderedBy, Tag::ReturnType];
    for tag in tags_num {
        let mut attrs = AssetMap::new();
        attrs.insert_attr(Tag::Alias, function_name.to_owned());
        attrs.insert_attr(Tag::Secret, function_name.to_owned());
        attrs.insert_attr(tag, 1);
        attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
    }
}
