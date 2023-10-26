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

use asset_sdk::{AssetMap, ErrCode, Insert, ReturnType, Tag, Value};

use crate::common::{
    add_default_asset, get_bytes, query_all_by_alias, query_attr_by_alias, remove_all, remove_by_alias,
};

#[test]
fn query_non_exist_with_alias() {
    let alias = "query_non_exist_with_alias".as_bytes();
    assert_eq!(ErrCode::NotFound, query_attr_by_alias(alias).unwrap_err());
    assert_eq!(ErrCode::NotFound, query_all_by_alias(alias).unwrap_err());
}

#[test]
fn query_with_wrong_alias() {
    let alias = "query_with_wrong_alias".as_bytes();
    let secret = "query_with_wrong_alias".as_bytes();
    add_default_asset(alias, secret).unwrap();

    let alias_new = "query_with_wrong_alias_wrong_alias".as_bytes();
    assert_eq!(ErrCode::NotFound, query_attr_by_alias(alias_new).unwrap_err());
    assert_eq!(ErrCode::NotFound, query_all_by_alias(alias_new).unwrap_err());
    remove_by_alias(alias).unwrap();
}

#[test]
fn query_non_exist_without_alias() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::RequirePasswordSet, true).unwrap();
    assert_eq!(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_without_alias_with_wrong_condition() {
    let alias = "query_non_exist_without_alias".as_bytes();
    let secret = "query_non_exist_without_alias".as_bytes();
    let mut add = AssetMap::new();
    add.insert_attr(Tag::RequirePasswordSet, false).unwrap();
    add.insert_attr(Tag::Alias, alias.to_owned()).unwrap();
    add.insert_attr(Tag::Secret, secret.to_owned()).unwrap();
    asset_sdk::Manager::build().unwrap().add(&add).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::RequirePasswordSet, true).unwrap();
    assert_eq!(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(alias).unwrap();
}

#[test]
fn query_without_limit() {
    let _ = remove_all();
    let alias = "query_without_limit".as_bytes();
    let secret = "query_without_limit".as_bytes();
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
    let alias = "query_with_limit_without_offset".as_bytes();
    let secret = "query_with_limit_without_offset".as_bytes();
    let asset_num = 10;
    for i in 0..asset_num {
        let alias_new = format!("{:?}{}", alias, i);
        let secret_new = format!("{:?}{}", secret, i);
        add_default_asset(alias_new.as_bytes(), secret_new.as_bytes()).unwrap();
    }

    let mut query = AssetMap::new();
    let limit = 3u32;
    query.insert_attr(Tag::ReturnLimit, limit).unwrap();
    let assets = asset_sdk::Manager::build().unwrap().query(&query).unwrap();
    assert_eq!(limit, assets.len() as u32);
    for (i, asset) in assets.iter().enumerate() {
        assert!(get_bytes(asset, Tag::Alias).unwrap().eq(format!("{:?}{}", alias, i).as_bytes()));
    }

    let offset = 2u32;
    query.insert_attr(Tag::ReturnOffset, offset).unwrap();
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
    let alias = "query_with_without_return_type".as_bytes();
    let secret = "query_with_without_return_type".as_bytes();
    add_default_asset(alias, secret).unwrap();

    assert!(!query_attr_by_alias(alias).unwrap()[0].contains_key(&Tag::Secret));
    assert!(query_all_by_alias(alias).unwrap()[0].contains_key(&Tag::Secret));

    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_secret() {
    let alias = "query_with_secret".as_bytes();
    let secret = "query_with_secret".as_bytes();
    add_default_asset(alias, secret).unwrap();

    let query = AssetMap::from([(Tag::Secret, Value::Bytes(secret.to_vec()))]);
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(alias).unwrap();
}

#[test]
fn query_with_return_all_without_alias() {
    let alias = "query_with_return_all_without_alias".as_bytes();
    let secret = "query_with_return_all_without_alias".as_bytes();
    add_default_asset(alias, secret).unwrap();

    let query = AssetMap::from([(Tag::ReturnType, Value::Number(ReturnType::All as u32))]);

    assert_eq!(ErrCode::NotSupport, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    remove_by_alias(alias).unwrap();
}
