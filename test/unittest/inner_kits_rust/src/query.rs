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

use std::collections::HashMap;

use crate::common::*;
use asset_sdk::*;

#[test]
fn query_non_exist_with_alias() {
    let alias = function!().as_bytes();
    expect_error_eq(ErrCode::NotFound, query_attr_by_alias(alias).unwrap_err());
    expect_error_eq(ErrCode::NotFound, query_all_by_alias(alias).unwrap_err());
}

#[test]
fn query_with_wrong_alias() {
    let function_name = function!().as_bytes();
    add_default_asset(function_name, function_name).unwrap();

    let alias_new = "query_with_wrong_alias_wrong_alias".as_bytes();
    expect_error_eq(ErrCode::NotFound, query_attr_by_alias(alias_new).unwrap_err());
    expect_error_eq(ErrCode::NotFound, query_all_by_alias(alias_new).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_non_exist_without_alias() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::RequirePasswordSet, true);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_without_alias_with_wrong_condition() {
    let function_name = function!().as_bytes();
    let mut add = AssetMap::new();
    add.insert_attr(Tag::RequirePasswordSet, false);
    add.insert_attr(Tag::Alias, function_name.to_owned());
    add.insert_attr(Tag::Secret, function_name.to_owned());
    add.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    asset_sdk::Manager::build().unwrap().add(&add).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::RequirePasswordSet, true);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_without_limit() {
    let _ = remove_all();
    let function_name = function!().as_bytes();
    let asset_num = 10;
    for i in 0..asset_num {
        let new_name = format!("{:?}{}", function_name, i);
        add_default_asset(new_name.as_bytes(), new_name.as_bytes()).unwrap();
    }

    let query = AssetMap::new();

    assert_eq!(asset_num, asset_sdk::Manager::build().unwrap().query(&query).unwrap().len() as u32);

    for i in 0..asset_num {
        let new_name = format!("{:?}{}", function_name, i);
        remove_by_alias(new_name.as_bytes()).unwrap();
    }
}

#[test]
fn query_with_offset_without_limit() {
    let _ = remove_all();
    let function_name = function!().as_bytes();
    let asset_num = 10;
    for i in 0..asset_num {
        let new_name = format!("{:?}{}", function_name, i);
        add_default_asset(new_name.as_bytes(), new_name.as_bytes()).unwrap();
    }

    let offset = 15;
    let query = AssetMap::from([(Tag::ReturnOffset, Value::Number(offset))]);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    let offset = 3;
    let query = AssetMap::from([(Tag::ReturnOffset, Value::Number(offset))]);
    assert_eq!(asset_num - offset, asset_sdk::Manager::build().unwrap().query(&query).unwrap().len() as u32);

    for i in 0..(asset_num - offset) {
        let new_name = format!("{:?}{}", function_name, i + offset);
        remove_by_alias(new_name.as_bytes()).unwrap();
    }
}

#[test]
fn query_with_limit_with_without_offset() {
    let _ = remove_all();
    let function_name = function!().as_bytes();
    let asset_num = 10;
    for i in 0..asset_num {
        let new_name = format!("{:?}{}", function_name, i);
        add_default_asset(new_name.as_bytes(), new_name.as_bytes()).unwrap();
    }

    let mut query = AssetMap::new();
    let limit = 15u32;
    query.insert_attr(Tag::ReturnLimit, limit);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(asset_num, assets.len() as u32);

    let limit = 3u32;
    query.insert_attr(Tag::ReturnLimit, limit);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);
    for (i, asset) in assets.iter().enumerate() {
        assert!(asset.get_bytes_attr(&Tag::Alias).unwrap().eq(format!("{:?}{}", function_name, i).as_bytes()));
    }

    let offset = 2u32;
    query.insert_attr(Tag::ReturnOffset, offset);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);
    for (i, asset) in assets.iter().enumerate() {
        assert!(asset
            .get_bytes_attr(&Tag::Alias)
            .unwrap()
            .eq(format!("{:?}{}", function_name, i + offset as usize).as_bytes()));
    }

    for i in 0..asset_num {
        let new_name = format!("{:?}{}", function_name, i);
        remove_by_alias(new_name.as_bytes()).unwrap();
    }
}

#[test]
fn query_with_without_return_type() {
    let function_name = function!().as_bytes();
    add_default_asset(function_name, function_name).unwrap();

    assert!(!query_attr_by_alias(function_name).unwrap()[0].contains_key(&Tag::Secret));
    assert!(query_all_by_alias(function_name).unwrap()[0].contains_key(&Tag::Secret));

    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_with_secret() {
    let function_name = function!().as_bytes();
    add_default_asset(function_name, function_name).unwrap();

    let query = AssetMap::from([(Tag::Secret, Value::Bytes(function_name.to_vec()))]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_with_return_all_without_alias() {
    let function_name = function!().as_bytes();
    add_default_asset(function_name, function_name).unwrap();

    let query = AssetMap::from([(Tag::ReturnType, Value::Number(ReturnType::All as u32))]);
    expect_error_eq(ErrCode::Unsupported, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(function_name).unwrap();
}

fn query_with_order(order: &[u8], suffix: &[&[u8]]) {
    let _ = remove_all();
    let function_name = function!();
    let asset_num = 4;
    let mut add = AssetMap::new();
    add.insert_attr(Tag::Secret, function_name.as_bytes().to_vec());
    for item in suffix.iter().take(asset_num) {
        let mut alias_new: Vec<u8> = Vec::new();
        alias_new.extend_from_slice(function_name.as_bytes());
        alias_new.extend_from_slice(item);
        let mut order_new: Vec<u8> = Vec::new();
        order_new.extend_from_slice(order);
        order_new.extend_from_slice(item);
        add_default_asset(&alias_new, function_name.as_bytes()).unwrap();
    }

    let mut query = AssetMap::new();
    let limit = 3u32;
    query.insert_attr(Tag::ReturnLimit, limit);
    query.insert_attr(Tag::ReturnOrderedBy, Tag::DataLabelNormal1 as u32);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);

    for (i, asset) in assets.iter().enumerate() {
        let get_alias = asset.get_bytes_attr(&Tag::Alias).unwrap();

        let mut alias_new: Vec<u8> = Vec::new();
        alias_new.extend_from_slice(function_name.as_bytes());
        alias_new.extend_from_slice(suffix[i]);
        assert_eq!(&alias_new, get_alias);
    }

    for item in suffix.iter().take(asset_num) {
        let mut alias_new: Vec<u8> = Vec::new();
        alias_new.extend_from_slice(function_name.as_bytes());
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

#[test]
fn query_with_wrong_auth_token() {
    let function_name = function!().as_bytes();
    add_default_auth_asset(function_name, function_name).unwrap();

    let mut query = AssetMap::new();
    let challenge = asset_sdk::Manager::build().unwrap().pre_query(&query).unwrap();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    query.insert_attr(Tag::AuthType, AuthType::Any);
    query.insert_attr(Tag::ReturnType, ReturnType::All);
    query.insert_attr(Tag::AuthToken, vec![0; MAX_AUTH_TOKEN_SIZE]);
    query.insert_attr(Tag::AuthChallenge, challenge.clone());
    expect_error_eq(ErrCode::AccessDenied, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthChallenge, challenge);
    asset_sdk::Manager::build().unwrap().post_query(&query).unwrap();
    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_with_bytes_tag() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();
    let alias = "default_alias".as_bytes();
    add_default_asset(alias, alias).unwrap();

    let mut bytes_tags: HashMap<Tag, &[u8]> = HashMap::new();
    bytes_tags.insert(Tag::Alias, function_name);
    bytes_tags.insert(Tag::DataLabelCritical1, CRITICAL_LABEL1);
    bytes_tags.insert(Tag::DataLabelCritical2, CRITICAL_LABEL2);
    bytes_tags.insert(Tag::DataLabelCritical3, CRITICAL_LABEL3);
    bytes_tags.insert(Tag::DataLabelCritical4, CRITICAL_LABEL4);
    bytes_tags.insert(Tag::DataLabelNormal1, NORMAL_LABEL1);
    bytes_tags.insert(Tag::DataLabelNormal2, NORMAL_LABEL2);
    bytes_tags.insert(Tag::DataLabelNormal3, NORMAL_LABEL3);
    bytes_tags.insert(Tag::DataLabelNormal4, NORMAL_LABEL4);
    bytes_tags.insert(Tag::DataLabelNormalLocal1, NORMAL_LOCAL_LABEL1);
    bytes_tags.insert(Tag::DataLabelNormalLocal2, NORMAL_LOCAL_LABEL2);
    bytes_tags.insert(Tag::DataLabelNormalLocal3, NORMAL_LOCAL_LABEL3);
    bytes_tags.insert(Tag::DataLabelNormalLocal4, NORMAL_LOCAL_LABEL4);

    for (tag, val) in bytes_tags.into_iter() {
        let mut query = AssetMap::new();
        query.insert_attr(tag, val.to_vec());
        let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
        assert_eq!(1, assets.len() as u32);
    }
    remove_by_alias(function_name).unwrap();
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_accessibility() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(1, assets.len() as u32);

    query.insert_attr(Tag::Accessibility, Accessibility::DeviceUnlocked);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.insert_attr(Tag::Accessibility, Accessibility::DeviceFirstUnlocked);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_with_return_order_by() {
    let alias = "return_order_by_alias".as_bytes();
    let mut attrs = AssetMap::new();
    let normal_label = "return_order_by_label1".as_bytes();
    attrs.insert_attr(Tag::Alias, alias.to_vec());
    attrs.insert_attr(Tag::Secret, alias.to_vec());
    attrs.insert_attr(Tag::DataLabelNormal1, normal_label.to_vec());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn as u32);
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::ReturnOrderedBy, Tag::DataLabelNormal1 as u32);
    let mut assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    let expect_query_len = 2;
    assert_eq!(expect_query_len, assets.len() as u32);
    assert!(assets[0].get_bytes_attr(&Tag::DataLabelNormal1).unwrap().eq(NORMAL_LABEL1));

    query.remove(&Tag::ReturnOrderedBy);
    assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(expect_query_len, assets.len() as u32);
    assert!(!assets[0].get_bytes_attr(&Tag::DataLabelNormal1).unwrap().eq(NORMAL_LABEL1));

    remove_by_alias(function_name).unwrap();
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_return_limit() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();
    let alias = "default_return_limit".as_bytes();
    add_default_asset(alias, alias).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::ReturnLimit, 1);
    let mut assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(1, assets.len() as u32);

    query.insert_attr(Tag::ReturnLimit, 0);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.remove(&Tag::ReturnLimit);
    assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    let expect_query_len = 2;
    assert_eq!(expect_query_len, assets.len() as u32);
    remove_by_alias(function_name).unwrap();
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_return_offset() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();
    let alias = "default_return_offset".as_bytes();
    add_default_asset(alias, alias).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::ReturnOffset, 1);
    let mut assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(1, assets.len() as u32);

    query.remove(&Tag::ReturnOffset);
    assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    let expect_query_len = 2;
    assert_eq!(expect_query_len, assets.len() as u32);

    let offset_start = 2;
    query.insert_attr(Tag::ReturnOffset, offset_start);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_return_type() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::ReturnType, ReturnType::Attributes as u32);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(1, assets.len() as u32);

    query.insert_attr(Tag::ReturnType, ReturnType::All as u32);
    expect_error_eq(ErrCode::Unsupported, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    remove_by_alias(function_name).unwrap();
}

#[test]
fn query_with_auth_type() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();
    let alias = "default_number_alias".as_bytes();
    add_default_asset(alias, alias).unwrap();

    for auth_type in vec![AuthType::Any as u32, AuthType::None as u32].into_iter() {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::AuthType, auth_type);
        let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
        assert_eq!(1, assets.len() as u32);
    }
    remove_by_alias(function_name).unwrap();
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_sync_type() {
    let function_name = function!().as_bytes();
    add_all_tags_asset(function_name).unwrap();

    let mut query = AssetMap::new();
    let mut sync_type = SyncType::ThisDevice as u32 | SyncType::TrustedDevice as u32 | SyncType::TrustedAccount as u32;
    query.insert_attr(Tag::SyncType, sync_type);
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    sync_type = SyncType::ThisDevice as u32;
    query.insert_attr(Tag::SyncType, sync_type);
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(1, assets.len() as u32);

    remove_by_alias(function_name).unwrap();
}
