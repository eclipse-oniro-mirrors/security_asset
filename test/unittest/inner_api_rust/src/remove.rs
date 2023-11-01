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
fn remove_alias_non_exist() {
    expect_error_eq(ErrCode::NotFound, remove_by_alias("remove_alias_non_exist".as_bytes()).unwrap_err());
}

#[test]
fn remove_condition_non_exist() {
    let delete_condition =
        AssetMap::from([(Tag::DataLabelCritical1, Value::Bytes("remove_condition_non_exist".as_bytes().to_vec()))]);
        expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().remove(&delete_condition).unwrap_err());
}

#[test]
fn remove_condition_exist_and_query() {
    let function_name = function!().as_bytes();
    let critical_label = "remove_condition_exist_and_query".as_bytes();
    let mut condition = AssetMap::from([
        (Tag::Alias, Value::Bytes(function_name.to_owned())),
        (Tag::Secret, Value::Bytes(function_name.to_owned())),
        (Tag::DataLabelCritical2, Value::Bytes(critical_label.to_owned())),
    ]);
    asset_sdk::Manager::build().unwrap().add(&condition).unwrap();
    condition.remove(&Tag::Alias);
    condition.remove(&Tag::Secret);
    asset_sdk::Manager::build().unwrap().remove(&condition).unwrap();
    expect_error_eq(ErrCode::NotFound, asset_sdk::Manager::build().unwrap().query(&condition).unwrap_err());
}

#[test]
fn remove_condition_with_secret() {
    let function_name = function!().as_bytes();
    let condition = AssetMap::from([
        (Tag::Alias, Value::Bytes(function_name.to_owned())),
        (Tag::Secret, Value::Bytes(function_name.to_owned())),
    ]);
    asset_sdk::Manager::build().unwrap().add(&condition).unwrap();
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().remove(&condition).unwrap_err());
    remove_by_alias(function_name).unwrap();
}
