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
fn pre_query_non_exist_with_alias() {
    let alias = function!().as_bytes();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.to_owned());
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
}

#[test]
fn pre_query_with_wrong_auth_type() {
    let function_name = function!().as_bytes();
    add_default_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_with_wrong_accessibility() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Accessibility, Accessibility::DeviceUnlocked);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_with_unsupported_auth_type() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthType, AuthType::None);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_with_wrong_persistent() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::IsPersistent, true);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_with_wrong_sync_type() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::SyncType, SyncType::TrustedDevice);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_batch_data() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    let challenge = asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap();
    assert_eq!(CHALLENGE_SIZE, challenge.len());

    query.insert_attr(Tag::AuthChallenge, challenge);
    asset_sdk::Manager::build().unwrap().post_query(&query).unwrap();

    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_single_data() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    query.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    query.insert_attr(Tag::AuthType, AuthType::Any);
    query.insert_attr(Tag::RequirePasswordSet, false);
    let challenge = asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap();
    assert_eq!(CHALLENGE_SIZE, challenge.len());

    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthChallenge, challenge);
    asset_sdk::Manager::build().unwrap().post_query(&query).unwrap();

    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_max_times() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let query = AssetMap::new();
    let mut challenges = vec![];
    for _i in 0..CRYPTO_CAPACITY {
        let challenge = asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap();
        assert_eq!(CHALLENGE_SIZE, challenge.len());
        challenges.push(challenge);
    }
    expect_error_eq(ErrCode::LimitExceeded, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());

    for challenge in challenges.into_iter() {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::AuthChallenge, challenge);
        asset_sdk::Manager::build().unwrap().post_query(&query).unwrap();
    }
    remove_by_alias(function_name).unwrap();
}

#[test]
fn pre_query_multiple_data_type() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let new_alias = "test_alias_c".as_bytes();
    asset_sdk::Manager::build()
        .unwrap()
        .add(&AssetMap::from([
            (Tag::Alias, Value::Bytes(new_alias.to_vec())),
            (Tag::Secret, Value::Bytes(new_alias.to_vec())),
            (Tag::Accessibility, Value::Number(Accessibility::DevicePowerOn as u32)),
            (Tag::AuthType, Value::Number(AuthType::Any as u32)),
            (Tag::RequirePasswordSet, Value::Bool(true)),
        ]))
        .unwrap();
    let query = AssetMap::new();
    expect_error_eq(ErrCode::NotSupport, asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
    remove_by_alias(new_alias).unwrap();
}
