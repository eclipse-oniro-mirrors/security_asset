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
fn query_non_exist_with_alias() {
    let alias = function!().as_bytes();
    assert_eq!(ErrCode::NotFound, query_attr_by_alias(alias).unwrap_err());
    assert_eq!(ErrCode::NotFound, query_all_by_alias(alias).unwrap_err());
}

#[test]
fn query_with_wrong_alias() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let alias_new = "query_with_wrong_alias_wrong_alias".as_bytes();
    assert_eq!(ErrCode::NotFound, query_attr_by_alias(alias_new).unwrap_err());
    assert_eq!(ErrCode::NotFound, query_all_by_alias(alias_new).unwrap_err());
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_non_exist_without_alias() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::RequirePasswordSet, true);
    assert_eq!(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_without_alias_with_wrong_condition() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    let mut add = AssetMap::new();
    add.insert_attr(Tag::RequirePasswordSet, false);
    add.insert_attr(Tag::Alias, alias.to_owned());
    add.insert_attr(Tag::Secret, secret.to_owned());
    asset_sdk::Manager::build().unwrap().add(&add).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::RequirePasswordSet, true);
    assert_eq!(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(alias).unwrap();
}

#[test]
fn query_without_limit() {
    let _ = remove_all();
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    let asset_num = 10;
    for i in 0..asset_num {
        let alias_new = format!("{:?}{}", alias, i);
        let secret_new = format!("{:?}{}", secret, i);
        add_default_asset(alias_new.as_bytes(), secret_new.as_bytes()).unwrap();
    }

    let query = AssetMap::new();

    assert_eq!(asset_num, asset_sdk::Manager::build().unwrap().query(&query).unwrap().len() as u32);

    for i in 0..asset_num {
        let alias_new = format!("{:?}{}", alias, i);
        remove_by_alias(alias_new.as_bytes()).unwrap();
    }
}

#[test]
fn query_with_limit_with_without_offset() {
    let _ = remove_all();
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    let asset_num = 10;
    for i in 0..asset_num {
        let alias_new = format!("{:?}{}", alias, i);
        let secret_new = format!("{:?}{}", secret, i);
        add_default_asset(alias_new.as_bytes(), secret_new.as_bytes()).unwrap();
    }

    let mut query = AssetMap::new();
    let limit = 3u32;
    query.insert_attr(Tag::ReturnLimit, limit);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);
    for (i, asset) in assets.iter().enumerate() {
        assert!(get_bytes(asset, Tag::Alias).unwrap().eq(format!("{:?}{}", alias, i).as_bytes()));
    }

    let offset = 2u32;
    query.insert_attr(Tag::ReturnOffset, offset);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);
    for (i, asset) in assets.iter().enumerate() {
        assert!(get_bytes(asset, Tag::Alias).unwrap().eq(format!("{:?}{}", alias, i + offset as usize).as_bytes()));
    }

    for i in 0..asset_num {
        let alias_new = format!("{:?}{}", alias, i);
        remove_by_alias(alias_new.as_bytes()).unwrap();
    }
}

#[test]
fn query_with_without_return_type() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    assert!(!query_attr_by_alias(alias).unwrap()[0].contains_key(&Tag::Secret));
    assert!(query_all_by_alias(alias).unwrap()[0].contains_key(&Tag::Secret));

    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_secret() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let query = AssetMap::from([(Tag::Secret, Value::Bytes(secret.to_vec()))]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_return_all_without_alias() {
    let alias = function!().as_bytes();
    let secret = function!().as_bytes();
    add_default_asset(alias, secret).unwrap();

    let query = AssetMap::from([(Tag::ReturnType, Value::Number(ReturnType::All as u32))]);

    assert_eq!(ErrCode::NotSupport, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(alias).unwrap();
}

fn query_with_order(order: &[u8], suffix: &[&[u8]]) {
    let _ = remove_all();
    let alias = function!();
    let secret = function!();
    let asset_num = 4;
    let mut add = AssetMap::new();
    add.insert_attr(Tag::Secret, secret.as_bytes().to_vec());
    for item in suffix.iter().take(asset_num) {
        let mut alias_new: Vec<u8> = Vec::new();
        alias_new.extend_from_slice(alias.as_bytes());
        alias_new.extend_from_slice(item);
        let mut order_new: Vec<u8> = Vec::new();
        order_new.extend_from_slice(order);
        order_new.extend_from_slice(item);
        add_default_asset(&alias_new, secret.as_bytes()).unwrap();
    }

    let mut query = AssetMap::new();
    let limit = 3u32;
    query.insert_attr(Tag::ReturnLimit, limit);
    query.insert_attr(Tag::ReturnOrderedBy, Tag::DataLabelNormal1 as u32);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);

    for (i, asset) in assets.iter().enumerate() {
        let get_alias = get_bytes(asset, Tag::Alias).unwrap();

        let mut alias_new: Vec<u8> = Vec::new();
        alias_new.extend_from_slice(alias.as_bytes());
        alias_new.extend_from_slice(suffix[i]);
        assert_eq!(&alias_new, get_alias);
    }

    for item in suffix.iter().take(asset_num) {
        let mut alias_new: Vec<u8> = Vec::new();
        alias_new.extend_from_slice(alias.as_bytes());
        alias_new.extend_from_slice(item);

        remove_by_alias(&alias_new).unwrap();
    }
}

#[test]
fn query_with_order_with_english() {
    let order = "query_with_order_with_english".as_bytes();
    let mut suffix: Vec<&[u8]> = ["one", "two", "three", "four"].iter().map(|s| s.as_bytes()).collect();
    suffix.sort_by(|a, b| b.cmp(a));
    query_with_order(order, &suffix);
}

#[test]
fn query_with_order_with_chinese() {
    let order = "排序".as_bytes();
    let mut suffix: Vec<&[u8]> = ["一", "二", "三", "四"].iter().map(|s| s.as_bytes()).collect();
    suffix.sort_by(|a, b| b.cmp(a));
    query_with_order(order, &suffix);
}

#[test]
fn query_with_order_with_number() {
    let order = "123".as_bytes();
    let mut suffix: Vec<&[u8]> = ["11", "22", "33", "44"].iter().map(|s| s.as_bytes()).collect();
    suffix.sort_by(|a, b| b.cmp(a));
    query_with_order(order, &suffix);
}

#[test]
fn query_with_order_with_unreadible() {
    let order = [11u8, 22u8, 33u8];
    let suffix: Vec<[u8; 4]> = [11u32, 22u32, 33u32, 44u32].iter().map(|s| s.to_le_bytes()).collect();

    let mut suffix: Vec<&[u8]> = suffix.iter().map(|s| s as &[u8]).collect();
    suffix.sort_by(|a, b| b.cmp(a));
    query_with_order(&order, &suffix);
}