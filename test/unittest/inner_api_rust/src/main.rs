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

use asset_sdk::{Accessibility, AssetMap, AuthType, ConflictResolution, Insert, ReturnType, SyncType, Tag, Value};

fn add_asset_inner(alias: &[u8]) {
    let mut input = AssetMap::new();
    input.insert_attr(Tag::Secret, alias.to_owned()).unwrap();
    input.insert_attr(Tag::AuthType, AuthType::None).unwrap();
    input.insert_attr(Tag::SyncType, SyncType::Never).unwrap();

    input.insert_attr(Tag::Accessibility, Accessibility::DeviceUnlock).unwrap();
    input.insert_attr(Tag::Alias, alias.to_owned()).unwrap();
    input.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => {
            if let Err(e) = manager.add(&input) {
                panic!("test for add failed {}", e)
            }
        },
        Err(e) => panic!("test for add failed {}", e),
    }
}

fn remove_asset_inner(alias: &[u8]) {
    let mut input = AssetMap::new();
    input.insert_attr(Tag::Alias, alias.to_owned()).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => {
            if let Err(e) = manager.remove(&input) {
                panic!("test for remote failed {}", e)
            }
        },
        Err(e) => panic!("test for add failed {}", e),
    }
}

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
        Err(e) => panic!("test for add failed {}", e),
    }

    remove_asset_inner(&Vec::from("alias".as_bytes()));
}

#[test]
fn test_for_add_conflict_overwrite() {
    let alias = Vec::from("test_for_add_conflict_overwrite".as_bytes());
    add_asset_inner(&alias);

    // panic will occur if fail
    add_asset_inner(&alias);

    remove_asset_inner(&alias);
}

#[test]
fn test_for_precise_query() {
    let alias = Vec::from("test_for_precise_query".as_bytes());
    add_asset_inner(&alias);
    let mut input = AssetMap::new();
    input.insert_attr(Tag::Alias, alias.clone()).unwrap();
    input.insert_attr(Tag::ReturnType, ReturnType::All).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => match manager.query(&input) {
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
            },
        },
        Err(e) => panic!("test for query failed {}", e),
    }

    match asset_sdk::Manager::build() {
        Ok(manager) => match manager.query(&input) {
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
                panic!("test for query 2 failed {}", e)
            },
        },
        Err(e) => panic!("test for query 2 failed {}", e),
    }

    remove_asset_inner(&alias);
}

#[test]
fn test_for_fuzz_query() {
    let alias = Vec::from("test_for_fuzz_query".as_bytes());
    add_asset_inner(&alias);
    let mut input = AssetMap::new();
    input.insert_attr(Tag::SyncType, SyncType::Never).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => match manager.query(&input) {
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
            },
        },
        Err(e) => panic!("test for query failed {}", e),
    }

    match asset_sdk::Manager::build() {
        Ok(manager) => match manager.query(&input) {
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
                panic!("test for query 2 failed {}", e)
            },
        },
        Err(e) => panic!("test for query 2 failed {}", e),
    }
    remove_asset_inner(&alias);
}

#[test]
fn test_for_update_normal_label() {
    let alias = Vec::from("test_for_update_normal_label".as_bytes());
    add_asset_inner(&alias);
    let mut update = AssetMap::new();
    update.insert_attr(Tag::DataLabelNormal1, Vec::from("DataLabelNormal1".as_bytes())).unwrap();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.clone()).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => match manager.update(&query, &update) {
            Ok(_) => {
                print!("update ok");
            },
            Err(e) => {
                panic!("test for update failed {}", e)
            },
        },
        Err(e) => panic!("test for update failed {}", e),
    }
    remove_asset_inner(&alias);
}

#[test]
fn test_for_update_secret_and_normal_label() {
    let alias = Vec::from("test_for_update_secret_and_normal_label".as_bytes());
    add_asset_inner(&alias);
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, alias.clone()).unwrap();

    let mut update = AssetMap::new();
    update.insert_attr(Tag::DataLabelNormal1, Vec::from("DataLabelNormal1".as_bytes())).unwrap();
    update.insert_attr(Tag::Secret, Vec::from("secret_test_for_update_secret_and_normal_label".as_bytes())).unwrap();

    match asset_sdk::Manager::build() {
        Ok(manager) => match manager.update(&query, &update) {
            Ok(_) => {
                print!("update ok");
            },
            Err(e) => {
                panic!("test for update failed {}", e)
            },
        },
        Err(e) => panic!("test for update failed {}", e),
    }
    remove_asset_inner(&alias);
}
