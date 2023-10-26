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

use asset_sdk::{AssetMap, AuthType, ConflictResolution, ErrCode, Insert, Tag, Value};

use crate::common::{get_bytes, get_number, query_attr_by_alias, remove_by_alias};

#[test]
fn add_all_tags() {
    let alias = "add_all_tags".as_bytes();
    let data_label = "add_all_tags".as_bytes();
    let secret = "add_all_tags".as_bytes();
    let auth_type = AuthType::None;
    let mut add = AssetMap::new();
    add.insert_attr(Tag::Alias, alias.to_owned()).unwrap();
    add.insert_attr(Tag::Secret, secret.to_owned()).unwrap();
    add.insert_attr(Tag::DataLabelCritical1, data_label.to_owned()).unwrap();
    add.insert_attr(Tag::AuthType, auth_type).unwrap();
    asset_sdk::Manager::build().unwrap().add(&add).unwrap();

    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(data_label, *get_bytes(&res[0], Tag::DataLabelCritical1).unwrap());
    assert_eq!(auth_type as u32, get_number(&res[0], Tag::AuthType).unwrap());

    remove_by_alias(alias).unwrap();
}

#[test]
fn add_conflict_throw_error() {
    let alias = "add_conflict_throw_error".as_bytes();
    let secret = "add_conflict_throw_error_secret".as_bytes();

    let mut add_throw_error =
        AssetMap::from([(Tag::Alias, Value::Bytes(alias.to_owned())), (Tag::Secret, Value::Bytes(secret.to_owned()))]);
    asset_sdk::Manager::build().unwrap().add(&add_throw_error).unwrap();

    assert_eq!(Err(ErrCode::Duplicated), asset_sdk::Manager::build().unwrap().add(&add_throw_error));

    add_throw_error.insert_attr(Tag::ConflictResolution, ConflictResolution::ThrowError).unwrap();
    assert_eq!(Err(ErrCode::Duplicated), asset_sdk::Manager::build().unwrap().add(&add_throw_error));

    remove_by_alias(alias).unwrap();
}

#[test]
fn add_conflict_over_write() {
    let alias = "add_conflict_over_write".as_bytes();
    let secret = "add_conflict_over_write_secret".as_bytes();
    let label_normal_1 = "add_conflict_over_write_label_normal_1".as_bytes();

    let mut add_over_write = AssetMap::new();
    add_over_write.insert_attr(Tag::Alias, alias.to_owned()).unwrap();
    add_over_write.insert_attr(Tag::Secret, secret.to_owned()).unwrap();

    asset_sdk::Manager::build().unwrap().add(&add_over_write).unwrap();

    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert!(res[0].get(&Tag::DataLabelNormal1).is_none());

    add_over_write.insert_attr(Tag::DataLabelNormal1, label_normal_1.to_owned()).unwrap();
    add_over_write.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite).unwrap();

    asset_sdk::Manager::build().unwrap().add(&add_over_write).unwrap();
    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(label_normal_1, *get_bytes(&res[0], Tag::DataLabelNormal1).unwrap());

    remove_by_alias(alias).unwrap();
}
