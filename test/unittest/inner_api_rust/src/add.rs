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

use asset_sdk::{
    AssetMap, AuthType, ConflictResolution, Insert, Tag, Value, ErrCode,
};

use crate::common::{get_bytes, get_number, delete_by_alias, query_by_alias};

#[test]
fn add_values_match_query() {
    let alias = "add_values_match_query".as_bytes().to_vec();
    let data_label = "add_values_match_query_data_label".as_bytes().to_vec();
    let secret = "add_values_match_query_secret".as_bytes().to_vec();
    let auth_type = AuthType::None;
    let mut add = AssetMap::new();
    add.insert_attr(Tag::Alias, alias.clone()).unwrap();
    add.insert_attr(Tag::Secret, secret).unwrap();
    add.insert_attr(Tag::DataLabelCritical1, data_label.clone()).unwrap();
    add.insert_attr(Tag::AuthType, auth_type).unwrap();
    asset_sdk::Manager::build().unwrap().add(&add).unwrap();

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.clone()).unwrap();
    let res = query_by_alias(&alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(data_label, *get_bytes(&res[0], Tag::DataLabelCritical1).unwrap());
    assert_eq!(auth_type as u32, get_number(&res[0], Tag::AuthType).unwrap());

    delete_by_alias(&alias).unwrap();
}

#[test]
fn add_conflict_throw_error() {
    let alias = "add_conflict_throw_error".as_bytes().to_vec();
    let secret = "add_conflict_throw_error_secret".as_bytes().to_vec();

    let mut add_throw_error = AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.clone())),
        (Tag::Secret, Value::Bytes(secret)),
    ]);
    asset_sdk::Manager::build().unwrap().add(&add_throw_error).unwrap();

    assert_eq!(Err(ErrCode::Duplicated), asset_sdk::Manager::build().unwrap().add(&add_throw_error));

    add_throw_error.insert_attr(Tag::ConflictResolution, ConflictResolution::ThrowError).unwrap();
    assert_eq!(Err(ErrCode::Duplicated), asset_sdk::Manager::build().unwrap().add(&add_throw_error));

    delete_by_alias(&alias).unwrap();
}

#[test]
fn add_conflict_over_write() {
    let alias = "add_conflict_over_write".as_bytes().to_vec();
    let secret = "add_conflict_over_write_secret".as_bytes().to_vec();
    let label_normal_1 = "add_conflict_over_write_label_normal_1".as_bytes().to_vec();

    let mut add_over_write = AssetMap::new();
    add_over_write.insert_attr(Tag::Alias, alias.clone()).unwrap();
    add_over_write.insert_attr(Tag::Secret, secret).unwrap();

    asset_sdk::Manager::build().unwrap().add(&add_over_write).unwrap();

    let res = query_by_alias(&alias).unwrap();
    assert_eq!(1, res.len());
    assert!(res[0].get(&Tag::DataLabelNormal1).is_none());

    add_over_write.insert_attr(Tag::DataLabelNormal1, label_normal_1.clone()).unwrap();
    add_over_write.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite).unwrap();

    asset_sdk::Manager::build().unwrap().add(&add_over_write).unwrap();
    let res = query_by_alias(&alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(label_normal_1, *get_bytes(&res[0], Tag::DataLabelNormal1).unwrap());

    delete_by_alias(&alias).unwrap();
}