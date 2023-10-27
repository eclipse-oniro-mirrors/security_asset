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

use core::panic;

use asset_sdk::*;
use crate::common::*;

#[test]
fn update_same_secret() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.to_owned());

    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, secret.to_owned());

    asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap();

    remove_by_alias(alias).unwrap();
}

#[test]
fn update_different_secret() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.to_owned());

    let secret_new = "update_different_secret_new".as_bytes();

    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, secret_new.to_owned());

    asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap();

    let res = query_all_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    let Value::Bytes(ref secret_query) = res[0][&Tag::Secret] else { panic!() };
    assert_eq!(secret_new, secret_query);

    remove_by_alias(alias).unwrap();
}

#[test]
fn update_attr_normal() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.to_owned());

    let label_normal = "update_attr_normal".as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::DataLabelNormal1, label_normal.to_owned());

    asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap();
    let query_res = &query_attr_by_alias(alias).unwrap()[0];
    let Value::Bytes(label_normal_query) = query_res.get(&Tag::DataLabelNormal1).unwrap() else { panic!() };
    assert_eq!(label_normal, label_normal_query);

    remove_by_alias(alias).unwrap();
}

#[test]
fn update_non_exist() {
    let alias = function!().as_bytes();
    let label_normal = function!().as_bytes();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.to_owned());

    let mut update = AssetMap::new();
    update.insert_attr(Tag::DataLabelNormal1, label_normal.to_owned());

    assert_eq!(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err());
}

#[test]
fn update_query_with_secret() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.to_owned());
    query.insert_attr(Tag::Secret, secret.to_owned());

    let label_normal = "update_query_with_secret".as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::DataLabelNormal1, label_normal.to_owned());

    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err());

    remove_by_alias(alias).unwrap();
}

#[test]
fn update_secret_without_query_alias() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let query = AssetMap::new();

    let label_normal = "update_secret_without_query_alias".as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::DataLabelNormal1, label_normal.to_owned());
    update.insert_attr(Tag::Secret, secret.to_owned());

    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err());

    remove_by_alias(alias).unwrap();
}

#[test]
fn update_alias() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let query = AssetMap::new();

    let alias_new = "update_alias_new".as_bytes();

    let mut update = AssetMap::new();
    update.insert_attr(Tag::Alias, alias_new.to_owned());

    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err());

    remove_by_alias(alias).unwrap();
}
