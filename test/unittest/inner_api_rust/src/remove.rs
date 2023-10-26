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

use asset_sdk::{AssetMap, ErrCode, Tag, Value};

use crate::common::remove_by_alias;

#[test]
fn remove_alias_non_exist() {
    assert_eq!(Err(ErrCode::NotFound), remove_by_alias("remove_alias_non_exist".as_bytes()));
}

#[test]
fn remove_condition_non_exist() {
    let delete_condition =
        AssetMap::from([(Tag::DataLabelCritical1, Value::Bytes("remove_condition_non_exist".as_bytes().to_vec()))]);
    assert_eq!(Err(ErrCode::NotFound), asset_sdk::Manager::build().unwrap().remove(&delete_condition));
}

#[test]
fn remove_condition_exist_and_query() {
    let alias = std::any::type_name::<()>().as_bytes();
    let secret = std::any::type_name::<()>().as_bytes();
    let label_vritial_2 = "remove_condition_exist_and_query".as_bytes();
    let mut condition = AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_owned())),
        (Tag::Secret, Value::Bytes(secret.to_owned())),
        (Tag::DataLabelCritical2, Value::Bytes(label_vritial_2.to_owned())),
    ]);
    asset_sdk::Manager::build().unwrap().add(&condition).unwrap();
    condition.remove(&Tag::Alias);
    condition.remove(&Tag::Secret);
    asset_sdk::Manager::build().unwrap().remove(&condition).unwrap();
    assert_eq!(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&condition).unwrap_err());
}

#[test]
fn remove_condition_with_secret() {
    let alias = std::any::type_name::<()>().as_bytes();
    let secret = std::any::type_name::<()>().as_bytes();
    let condition =
        AssetMap::from([(Tag::Alias, Value::Bytes(alias.to_owned())), (Tag::Secret, Value::Bytes(secret.to_owned()))]);
    asset_sdk::Manager::build().unwrap().add(&condition).unwrap();
    assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().remove(&condition).unwrap_err());
    remove_by_alias(alias).unwrap();
}
