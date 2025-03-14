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
fn add_all_tags() {
    let alias = function!().as_bytes();
    add_all_tags_asset(alias).unwrap();

    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(20, res[0].len());
    assert_eq!(alias, *res[0].get_bytes_attr(&Tag::Alias).unwrap());
    assert_eq!(NORMAL_LABEL1, *res[0].get_bytes_attr(&Tag::DataLabelNormal1).unwrap());
    assert_eq!(NORMAL_LABEL2, *res[0].get_bytes_attr(&Tag::DataLabelNormal2).unwrap());
    assert_eq!(NORMAL_LABEL3, *res[0].get_bytes_attr(&Tag::DataLabelNormal3).unwrap());
    assert_eq!(NORMAL_LABEL4, *res[0].get_bytes_attr(&Tag::DataLabelNormal4).unwrap());
    assert_eq!(NORMAL_LOCAL_LABEL1, *res[0].get_bytes_attr(&Tag::DataLabelNormalLocal1).unwrap());
    assert_eq!(NORMAL_LOCAL_LABEL2, *res[0].get_bytes_attr(&Tag::DataLabelNormalLocal2).unwrap());
    assert_eq!(NORMAL_LOCAL_LABEL3, *res[0].get_bytes_attr(&Tag::DataLabelNormalLocal3).unwrap());
    assert_eq!(NORMAL_LOCAL_LABEL4, *res[0].get_bytes_attr(&Tag::DataLabelNormalLocal4).unwrap());
    assert_eq!(CRITICAL_LABEL1, *res[0].get_bytes_attr(&Tag::DataLabelCritical1).unwrap());
    assert_eq!(CRITICAL_LABEL2, *res[0].get_bytes_attr(&Tag::DataLabelCritical2).unwrap());
    assert_eq!(CRITICAL_LABEL3, *res[0].get_bytes_attr(&Tag::DataLabelCritical3).unwrap());
    assert_eq!(CRITICAL_LABEL4, *res[0].get_bytes_attr(&Tag::DataLabelCritical4).unwrap());
    assert_eq!(Accessibility::DevicePowerOn, res[0].get_enum_attr::<Accessibility>(&Tag::Accessibility).unwrap());
    assert_eq!(AuthType::Any, res[0].get_enum_attr::<AuthType>(&Tag::AuthType).unwrap());
    assert_eq!(SyncType::ThisDevice, res[0].get_enum_attr::<SyncType>(&Tag::SyncType).unwrap());
    assert!(!res[0].get_bool_attr(&Tag::IsPersistent).unwrap());
    assert!(!res[0].get_bool_attr(&Tag::RequirePasswordSet).unwrap());

    remove_by_alias(alias).unwrap();
}

#[test]
fn add_required_tags() {
    let func_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, func_name.to_owned());
    attrs.insert_attr(Tag::Secret, func_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_all_by_alias(func_name).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(9, res[0].len());
    assert_eq!(func_name, *res[0].get_bytes_attr(&Tag::Alias).unwrap());
    assert_eq!(func_name, *res[0].get_bytes_attr(&Tag::Secret).unwrap());
    assert_eq!(Accessibility::DevicePowerOn, res[0].get_enum_attr::<Accessibility>(&Tag::Accessibility).unwrap());
    assert_eq!(AuthType::None, res[0].get_enum_attr::<AuthType>(&Tag::AuthType).unwrap());
    assert_eq!(SyncType::Never, res[0].get_enum_attr::<SyncType>(&Tag::SyncType).unwrap());
    assert!(!res[0].get_bool_attr(&Tag::IsPersistent).unwrap());
    assert!(!res[0].get_bool_attr(&Tag::RequirePasswordSet).unwrap());
    remove_by_alias(func_name).unwrap();
}

#[test]
fn add_english_secret() {
    let func_name = function!();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, func_name.as_bytes().to_owned());
    attrs.insert_attr(Tag::Secret, func_name.as_bytes().to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_all_by_alias(func_name.as_bytes()).unwrap();
    assert_eq!(1, res.len());
    let bytes = res[0].get_bytes_attr(&Tag::Secret).unwrap();
    assert_eq!(func_name, String::from_utf8(bytes.to_owned()).unwrap());
    remove_by_alias(func_name.as_bytes()).unwrap();
}

#[test]
fn add_chinese_secret() {
    let alias = "Здравствуйте";
    let secret = "中文";
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.as_bytes().to_owned());
    attrs.insert_attr(Tag::Secret, secret.as_bytes().to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_all_by_alias(alias.as_bytes()).unwrap();
    assert_eq!(1, res.len());
    let bytes = res[0].get_bytes_attr(&Tag::Secret).unwrap();
    assert_eq!(secret, String::from_utf8(bytes.to_owned()).unwrap());
    let bytes = res[0].get_bytes_attr(&Tag::Alias).unwrap();
    assert_eq!(alias, String::from_utf8(bytes.to_owned()).unwrap());
    remove_by_alias(alias.as_bytes()).unwrap();
}

#[test]
fn add_same_alias_throw_error() {
    let function_name = function!().as_bytes();

    // step1. insert data
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    // step2. insert data with the same alias, default resolution: throw error
    expect_error_eq(ErrCode::Duplicated, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    // step3. insert data with the same alias, specified resolution: throw error
    attrs.insert_attr(Tag::ConflictResolution, ConflictResolution::ThrowError);
    expect_error_eq(ErrCode::Duplicated, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_same_alias_overwrite() {
    let function_name = function!().as_bytes();

    // step1. insert data
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    // step2. query data with no label
    let res = query_attr_by_alias(function_name).unwrap();
    assert_eq!(1, res.len());
    assert!(res[0].get(&Tag::DataLabelCritical1).is_none());

    // step3. insert data with the same alias, specified resolution: overwrite
    let critical_label = "add_same_alias_overwrite".as_bytes();
    attrs.insert_attr(Tag::DataLabelCritical1, critical_label.to_owned());
    attrs.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite);
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    // step4. query new data with critical label
    let res = query_attr_by_alias(function_name).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(critical_label, *res[0].get_bytes_attr(&Tag::DataLabelCritical1).unwrap());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_multiple_sync_types() {
    let function_name = function!().as_bytes();
    let sync_type = (SyncType::ThisDevice as u32) | (SyncType::TrustedDevice as u32);
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::SyncType, sync_type);
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_attr_by_alias(function_name).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(sync_type, res[0].get_num_attr(&Tag::SyncType).unwrap());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn add_is_persistent_auth_wrong() {
    let function_name = function!().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, function_name.to_owned());
    attrs.insert_attr(Tag::Secret, function_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::IsPersistent, true);
    expect_error_eq(ErrCode::PermissionDenied, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());

    attrs.insert_attr(Tag::IsPersistent, false);
    expect_error_eq(ErrCode::PermissionDenied, asset_sdk::Manager::build().unwrap().add(&attrs).unwrap_err());
}
