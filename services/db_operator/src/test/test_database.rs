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

use crate::{
    database::Database,
    table::Table,
    types::{column, DbMap, QueryOptions, TABLE_NAME},
};
use asset_definition::{Extension, Value};

const DB_DATA: [(&str, Value); 9] = [
    (column::OWNER_TYPE, Value::Number(1)),
    (column::SYNC_TYPE, Value::Number(1)),
    (column::ACCESSIBILITY, Value::Number(1)),
    (column::AUTH_TYPE, Value::Number(1)),
    (column::DELETE_TYPE, Value::Number(1)),
    (column::VERSION, Value::Number(1)),
    (column::CREATE_TIME, Value::Number(1)),
    (column::UPDATE_TIME, Value::Number(1)),
    (column::REQUIRE_PASSWORD_SET, Value::Number(0)),
];

fn create_dir() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
}

fn remove_dir() {
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

fn open_db_and_insert_data() -> Database {
    create_dir();
    let mut def = DbMap::from(DB_DATA);
    def.insert(column::SECRET, Value::Bytes(column::SECRET.as_bytes().to_vec()));
    def.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    def.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    let mut db = Database::build(0).unwrap();
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);
    db
}

#[test]
fn create_and_drop_database() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let mut db = Database::build(0).unwrap();
    db.close();
    assert!(Database::delete(0).is_ok());
}

#[test]
fn database_version() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    assert_eq!(0, db.get_version().unwrap());
    assert!(db.set_version(1).is_ok());
    assert_eq!(1, db.get_version().unwrap());
    let _ = Database::delete(0);
}

#[test]
fn error_sql() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let sql = "pragma zzz user_version = {} mmm";
    assert!(db.exec(sql).is_err());
    let _ = Database::delete(0);
}

#[test]
fn create_delete_asset_table() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let mut db = Database::build(0).unwrap();
    let table = Table::new(TABLE_NAME, &db);
    assert!(table.exist().unwrap());
    assert!(table.delete().is_ok());
    assert!(!table.exist().unwrap());
    db.close();
    let _ = Database::delete(0);
}

#[test]
fn insert_data_with_different_alias() {
    create_dir();
    let mut def = DbMap::from(DB_DATA);
    def.insert(column::SECRET, Value::Bytes(column::SECRET.as_bytes().to_vec()));
    def.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    def.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));

    let mut db = Database::build(0).unwrap();
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);

    def.insert(column::ALIAS, Value::Bytes(b"Alias2".to_vec()));
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);

    let ret = db
        .query_datas(&vec![], &DbMap::from([(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()))]), None)
        .unwrap();
    assert_eq!(ret.len(), 2);
    remove_dir();
}

#[test]
fn delete_data() {
    let mut db = open_db_and_insert_data();

    let mut datas = DbMap::new();
    datas.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    datas.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));

    let ret = db.is_data_exists(&datas).unwrap();
    assert!(ret);

    let count = db.delete_datas(&datas).unwrap();
    assert_eq!(count, 1);

    let ret = db.is_data_exists(&datas).unwrap();
    assert!(!ret);

    remove_dir();
}

#[test]
fn update_data() {
    let mut db = open_db_and_insert_data();

    let mut datas = DbMap::new();
    datas.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    datas.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    let update_time = 2;
    let count = db.update_datas(&datas, &DbMap::from([(column::UPDATE_TIME, Value::Number(update_time))])).unwrap();
    assert_eq!(count, 1);

    let res = db.query_datas(&vec![], &datas, None).unwrap();
    assert_eq!(res.len(), 1);
    assert_eq!(update_time, res[0].get_num_attr(&column::UPDATE_TIME).unwrap());

    remove_dir();
}

#[test]
fn query_ordered_data() {
    // insert two data
    create_dir();
    let mut def = DbMap::from(DB_DATA);
    def.insert(column::SECRET, Value::Bytes(column::SECRET.as_bytes().to_vec()));
    def.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    def.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    let mut db = Database::build(0).unwrap();
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);

    def.insert(column::ALIAS, Value::Bytes(b"AAA".to_vec()));
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);

    // query data by order
    let query = QueryOptions {
        limit: Some(100),
        offset: Some(0),
        order: Some(Ordering::Greater),
        order_by: Some(vec![column::ALIAS]),
    };
    let res = db
        .query_datas(
            &vec![column::ID, column::ALIAS],
            &DbMap::from([(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()))]),
            Some(&query),
        )
        .unwrap();
    assert_eq!(res.len(), 2);
    assert_eq!(&(b"AAA".to_vec()), res[0].get_bytes_attr(&column::ALIAS).unwrap());

    remove_dir();
}

#[test]
fn insert_error_data() {
    create_dir();
    let mut datas = DbMap::new();
    datas.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    let mut db = Database::build(0).unwrap();
    assert!(db.insert_datas(&datas).is_err());
    remove_dir();
}

#[test]
fn backup_and_restore() {
    let db = open_db_and_insert_data();
    drop(db);

    // Destroy the main database.
    let mut db_file = OpenOptions::new().read(true).write(true).open("/data/asset_test/0/asset.db").unwrap();
    let _ = db_file.write(b"buffer buffer buffer").unwrap();

    // Recovery the main database.
    let mut db = Database::build(0).unwrap();
    let mut def = DbMap::from(DB_DATA);
    def.insert(column::SECRET, Value::Bytes(column::SECRET.as_bytes().to_vec()));
    def.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    def.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    db.insert_datas(&def).unwrap();
    drop(db);

    // Destroy the backup database.
    let mut backup_file = OpenOptions::new().read(true).write(true).open("/data/asset_test/0/asset.db.backup").unwrap();
    let _ = backup_file.write(b"bad message info").unwrap();

    // Recovery the backup database.
    let mut db = Database::build(0).unwrap();
    db.insert_datas(&def).unwrap();
    let ret = db
        .query_datas(&vec![], &DbMap::from([(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()))]), None)
        .unwrap();
    assert_eq!(ret.len(), 3);
    remove_dir();
}
