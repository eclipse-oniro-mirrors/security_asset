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

use std::{
    cmp::Ordering,
    fs::{self, OpenOptions},
    io::Write,
};

use asset_db_operator::{
    database::*,
    database_table_helper::{do_transaction, DatabaseHelper},
    types::{column, from_data_value_to_str_value, DbMap, QueryOptions},
};
use asset_definition::Value;

pub fn test_for_default_asset(user_id: i32) {
    fs::create_dir_all(format!("/data/service/el1/public/asset_service/{}/", user_id)).unwrap();
    // let _ = Database::drop_default_database(user_id);
    let mut def = DbMap::from([
        ("Secret", Value::Bytes(b"blob".to_vec())),
        ("OwnerType", Value::Number(1)),
        ("SyncType", Value::Number(1)),
        ("Accessibility", Value::Number(1)),
        ("AuthType", Value::Number(1)),
        ("DeleteType", Value::Number(1)),
        ("Version", Value::Number(1)),
        ("CreateTime", Value::Number(1)),
        ("UpdateTime", Value::Number(1)),
        ("RequirePasswordSet", Value::Number(0)),
        (column::OWNER, Value::Bytes(b"owner1".to_vec())),
        (column::ALIAS, Value::Bytes(b"Alias1".to_vec())),
    ]);

    let count = DatabaseHelper::insert_datas(user_id, &def).unwrap();
    assert_eq!(count, 1);

    def.remove(column::ALIAS);
    def.insert(column::ALIAS, Value::Bytes(b"Alias2".to_vec()));

    let count = DatabaseHelper::insert_datas(user_id, &def).unwrap();
    assert_eq!(count, 1);

    let count = DatabaseHelper::update_datas(
        user_id,
        &DbMap::from([("Owner", Value::Bytes(b"owner1".to_vec())), ("Alias", Value::Bytes(b"Alias1".to_vec()))]),
        &DbMap::from([("UpdateTime", Value::Number(1))]),
    )
    .unwrap();
    assert!(count >= 0);

    let _count =
        DatabaseHelper::select_count(user_id, &DbMap::from([(column::OWNER, Value::Bytes(b"owner1".to_vec()))]))
            .unwrap();

    let _ret = DatabaseHelper::is_data_exists(
        user_id,
        &DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"Alias2".to_vec())),
        ]),
    )
    .unwrap();

    let count = DatabaseHelper::delete_datas(
        user_id,
        &DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"Alias1".to_vec())),
        ]),
    )
    .unwrap();
    assert!(count >= 0);

    default_asset_fun(user_id);
}

fn default_asset_fun(user_id: i32) {
    let query = QueryOptions {
        limit: Some(100),
        offset: Some(0),
        order: Some(Ordering::Greater),
        order_by: Some(vec![column::OWNER, column::ALIAS]),
    };

    let result = DatabaseHelper::query_datas(
        user_id,
        &DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"Alias2".to_vec())),
        ]),
        Some(&query),
    )
    .unwrap();
    for line in result {
        print!("line: ");
        for r in line {
            print!("{}, ", from_data_value_to_str_value(&r));
        }
        println!();
    }

    let result = DatabaseHelper::query_columns(
        user_id,
        &vec!["Id", "Alias"],
        &DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"Alias2".to_vec())),
        ]),
        None,
    )
    .unwrap();
    for line in result {
        for (k, v) in line {
            print!("{}:{}, ", k, from_data_value_to_str_value(&v));
        }
        println!();
    }
    // let db = DatabaseHelper::open_default_database_table(user_id).unwrap();
    // let _ = db.drop_database_and_backup();
}

#[test]
pub fn test_for_default_asset_multi() {
    test_for_default_asset(1);
}

#[test]
pub fn test_for_default_asset_multi1() {
    test_for_default_asset(1);
}

#[test]
pub fn test_for_default_asset_multi2() {
    test_for_default_asset(2);
}

/// trans callback
fn trans_call(db: &Database) -> bool {
    let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner1".to_vec()))])).unwrap();
    assert_eq!(count, 0);
    true
}

#[test]
pub fn test_for_transaction3() {
    fs::create_dir_all(format!("/data/service/el1/public/asset_service/{}/", 6)).unwrap();
    let ret = do_transaction(6, trans_call).unwrap();
    assert!(ret);
    let trans = |db: &Database| -> bool {
        let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner1".to_vec()))])).unwrap();
        assert_eq!(count, 0);
        true
    };
    let ret = do_transaction(6, trans).unwrap();
    assert!(ret);
}

#[test]
pub fn test_for_error() {
    fs::create_dir_all(format!("/data/service/el1/public/asset_service/{}/", 1)).unwrap();
    let stmt = DatabaseHelper::insert_datas(
        1,
        &DbMap::from([
            (column::OWNER, Value::Bytes(b"owner".to_vec())),
            (column::OWNER, Value::Bytes(b"alias".to_vec())),
        ]),
    );
    assert!(stmt.is_err());
}

#[test]
pub fn test_for_master_backup() {
    fs::create_dir_all(format!("/data/service/el1/public/asset_service/{}/", 5)).unwrap();
    let _ = Database::drop_default_database_and_backup(5);
    let db = DatabaseHelper::open_default_database_table(5).unwrap();
    let def = DbMap::from([
        ("Secret", Value::Bytes(b"blob".to_vec())),
        ("OwnerType", Value::Number(1)),
        ("SyncType", Value::Number(1)),
        ("Accessibility", Value::Number(1)),
        ("AuthType", Value::Number(1)),
        ("DeleteType", Value::Number(1)),
        ("Version", Value::Number(1)),
        ("CreateTime", Value::Number(1)),
        ("UpdateTime", Value::Number(1)),
        ("RequirePasswordSet", Value::Number(0)),
        (column::OWNER, Value::Bytes(b"owner".to_vec())),
        (column::ALIAS, Value::Bytes(b"Alias".to_vec())),
    ]);

    db.insert_datas(&def).unwrap();
    drop(db);
    let mut db_file =
        OpenOptions::new().read(true).write(true).open("/data/service/el1/public/asset_service/5/asset.db").unwrap(); // write master db
    let _ = db_file.write(b"buffer buffer buffer").unwrap();
    let db = DatabaseHelper::open_default_database_table(5).unwrap(); // will recovery master db
    db.insert_datas(&def).unwrap();
    drop(db);
    let mut back_file = OpenOptions::new()
        .read(true)
        .write(true)
        .open("/data/service/el1/public/asset_service/5/asset.db.backup")
        .unwrap(); // write backup db
    let _ = back_file.write(b"bad message info").unwrap();
    let db = DatabaseHelper::open_default_database_table(5).unwrap(); // will recovery backup db
    db.insert_datas(&def).unwrap();
    let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner".to_vec()))])).unwrap();
    assert_eq!(count, 3);
    drop(db);
}
