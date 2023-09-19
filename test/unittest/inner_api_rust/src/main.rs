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

use asset_sdk::definition::{AssetMap, Accessibility, Tag, Insert, AuthType, SyncType, Value};

#[test]
fn test_for_add() {
    let mut input = AssetMap::new();
    input.insert_attr(Tag::Secret, Vec::from("alias".as_bytes())).unwrap();
    input.insert_attr(Tag::AuthType, AuthType::None).unwrap();
    input.insert_attr(Tag::SyncType, SyncType::Never).unwrap();

    input.insert_attr(Tag::Accessibility, Accessibility::DeviceUnlock).unwrap();
    input.insert_attr(Tag::Alias, Vec::from("alias".as_bytes())).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => {
            if let Err(e) = manager.add(&input) {
                panic!("test for add failed {}", e)
            }
        },
        Err(e) => panic!("test for add failed {}", e)
    }
}

#[test]
fn test_for_query() {
    test_for_add();
    let mut input = AssetMap::new();
    input.insert_attr(Tag::Alias, Vec::from("alias".as_bytes())).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => {
            match manager.query(&input) {
                Ok(res) => {
                    for map in res.iter() {
                        for (tag, value) in map.iter() {
                            match value {
                                Value::Bool(boolean) => println!("get tag:[{}] value:[{}]", tag, boolean),
                                Value::Number(num) => println!("get tag:[{}] value:[{}]", tag, num),
                                Value::Bytes(bytes) => println!("get tag:[{}] value_len:[{}]", tag, bytes.len()),
                            }
                        }
                    }
                },
                Err(e) => {
                    panic!("test for query failed {}", e)
                }
            }
        },
        Err(e) => panic!("test for add failed {}", e)
    }
}
