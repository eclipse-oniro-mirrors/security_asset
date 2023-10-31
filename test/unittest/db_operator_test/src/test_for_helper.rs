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
use std::{
    cmp::Ordering,
    fs::{self, OpenOptions},
    io::Write,
};

use asset_db_operator::{
    database::*,
    database_table_helper::{do_transaction, DatabaseHelper},
    statement::Statement,
    types::{
        column, from_data_value_to_str_value, ColumnInfo, DbMap, QueryOptions, ASSET_TABLE_NAME, SQLITE_OK, SQLITE_ROW,
    },
};
use asset_definition::{DataType, Value};

#[test]
pub fn test_helper() {
    let _ = Database::drop_database("test18.db");
    let db = match Database::new("test18.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![Value::Bytes(b"owner1".to_vec()), Value::Bytes(b"alias1".to_vec()), Value::Bytes(b"aaaa".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias2".to_vec()), Value::Bytes(b"bbbb".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias3".to_vec()), Value::Bytes(b"cccc".to_vec())],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let exist = db
        .is_data_exists(&DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists(&DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias2".to_vec())),
        ]))
        .unwrap();
    assert!(!exist);

    helper_fun(db);
}

fn helper_fun(db: Database<'_>) {
    let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner2".to_vec()))])).unwrap();
    assert_eq!(count, 2);

    let ret = db
        .insert_datas(&DbMap::from([
            ("value", Value::Bytes(b"value4".to_vec())),
            (column::OWNER, Value::Bytes(b"owner4".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias4".to_vec())),
        ]))
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .update_datas(
            &DbMap::from([
                (column::OWNER, Value::Bytes(b"owner4".to_vec())),
                (column::ALIAS, Value::Bytes(b"alias4".to_vec())),
            ]),
            &DbMap::from([("value", Value::Bytes(b"value5".to_vec()))]),
        )
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .delete_datas(&DbMap::from([
            (column::OWNER, Value::Bytes(b"owner4".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias4".to_vec())),
        ]))
        .unwrap();
    assert_eq!(ret, 1);

    let result = db
        .query_datas(
            &DbMap::from([
                (column::OWNER, Value::Bytes(b"owner1".to_vec())),
                (column::ALIAS, Value::Bytes(b"alias1".to_vec())),
            ]),
            None,
        )
        .unwrap();
    assert_eq!(result.len(), 1);
    for data in result {
        print!("line: ");
        for d in data {
            print!("{}, ", from_data_value_to_str_value(&d));
        }
        println!();
    }
}

#[test]
pub fn test_for_special_sql() {
    let _ = Database::drop_database("test19.db");
    let db = match Database::new("test19.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![Value::Bytes(b"owner1".to_vec()), Value::Bytes(b"alias1".to_vec()), Value::Bytes(b"aaaa".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias2".to_vec()), Value::Bytes(b"bbbb".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias3".to_vec()), Value::Bytes(b"cccc".to_vec())],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    let sql = "select Owner,Alias from asset_table where Id>?";
    let stmt = Statement::<true>::prepare(sql, &db).unwrap();
    let ret = stmt.bind_data(1, &Value::Number(1));
    assert_eq!(ret, SQLITE_OK);

    while stmt.step() == SQLITE_ROW {
        print!("line: ");
        let owner = stmt.query_column_text(0);
        let alias = stmt.query_column_text(1);
        print!(
            "{} {}",
            from_data_value_to_str_value(&Value::Bytes(owner.to_vec())),
            from_data_value_to_str_value(&Value::Bytes(alias.to_vec()))
        );
        println!();
    }
}

#[test]
pub fn test_for_update_ver() {
    let _ = Database::drop_database("test20.db");
    let db = Database::new("test20.db").unwrap();
    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let _ = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    drop(db);
    let db2 = Database::new_with_version_update("test20.db", 0, default_update_database_func).unwrap();
    drop(db2);

    let db3 = Database::new_with_version_update("test20.db", 1, default_update_database_func).unwrap();
    drop(db3);

    let db4 = Database::new_with_version_update("test20.db", 0, default_update_database_func);
    assert!(db4.is_err());
}

pub fn test_for_default_asset(user_id: i32) {
    fs::create_dir_all(format!("/data/asset_test/{}/", user_id)).unwrap();
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

#[test]
pub fn test_for_recovery() {
    let db = Database::new("test111.db").unwrap();
    let table = db
        .create_table(
            "tt",
            &[ColumnInfo { name: "Id", data_type: DataType::Number, is_primary_key: true, not_null: true }],
        )
        .unwrap();
    let count = table.insert_row(&DbMap::from([("Id", Value::Number(1))])).unwrap();
    assert_eq!(count, 1);
    fs::copy("test111.db", "test111.db.backup").unwrap();
    fs::remove_file("test111.db").unwrap();
    fs::copy("test111.db.backup", "test111.db").unwrap();
    let count = table.count_datas(&DbMap::new()).unwrap();
    assert_eq!(count, 1);
    let _ = Database::drop_database_and_backup(db);
}

/// trans callback
fn trans_call(db: &Database) -> bool {
    let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner1".to_vec()))])).unwrap();
    assert_eq!(count, 0);
    true
}

#[test]
pub fn test_for_transaction3() {
    fs::create_dir_all(format!("/data/asset_test/{}/", 6)).unwrap();
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
    fs::create_dir_all(format!("/data/asset_test/{}/", 1)).unwrap();
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
    fs::create_dir_all(format!("/data/asset_test/{}/", 5)).unwrap();
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
    let mut db_file = OpenOptions::new().read(true).write(true).open("/data/asset_test/5/asset.db").unwrap(); // write master db
    let _ = db_file.write(b"buffer buffer buffer").unwrap();
    let db = DatabaseHelper::open_default_database_table(5).unwrap(); // will recovery master db
    db.insert_datas(&def).unwrap();
    drop(db);
    let mut back_file = OpenOptions::new().read(true).write(true).open("/data/asset_test/5/asset.db.backup").unwrap(); // write backup db
    let _ = back_file.write(b"bad message info").unwrap();
    let db = DatabaseHelper::open_default_database_table(5).unwrap(); // will recovery backup db
    db.insert_datas(&def).unwrap();
    let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner".to_vec()))])).unwrap();
    assert_eq!(count, 3);
    drop(db);
}
