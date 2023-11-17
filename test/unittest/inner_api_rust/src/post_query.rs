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
fn post_query_non_exist_with_auth_challenge() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE]);
    assert!(asset_sdk::Manager::build().unwrap().post_query(&query).is_ok());
}

#[test]
fn post_query_with_wrong_auth_challenge() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    let challenge = asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap();

    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE]);
    assert!(asset_sdk::Manager::build().unwrap().post_query(&query).is_ok());

    query.insert_attr(Tag::AuthChallenge, challenge);
    asset_sdk::Manager::build().unwrap().post_query(&query).unwrap();
    remove_by_alias(function_name).unwrap();
}

#[test]
fn post_query_normal() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    let challenge = asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap();

    query.insert_attr(Tag::AuthChallenge, challenge);
    assert!(asset_sdk::Manager::build().unwrap().post_query(&query).is_ok());
    remove_by_alias(function_name).unwrap();
}
