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
    path::Path,
};

use asset_common::CallingInfo;
use asset_definition::{ErrCode, Extension, Value};

use crate::{
    database::Database,
    table::Table,
    types::{column, DbMap, QueryOptions, TABLE_NAME},
};

const DB_DATA: [(&str, Value); 9] = [
    (column::OWNER_TYPE, Value::Number(1)),
    (column::SYNC_TYPE, Value::Number(1)),
    (column::ACCESSIBILITY, Value::Number(1)),
    (column::AUTH_TYPE, Value::Number(1)),
    (column::IS_PERSISTENT, Value::Bool(true)),
    (column::VERSION, Value::Number(1)),
    (column::REQUIRE_PASSWORD_SET, Value::Bool(false)),
    (column::LOCAL_STATUS, Value::Number(0)),
    (column::SYNC_STATUS, Value::Number(0)),
];

const TEST_FILE: &str = "/data/asset_test/0";

fn create_dir() {
    let path = Path::new(TEST_FILE);
    if !path.exists() {
        fs::create_dir_all(path).unwrap();
    }
}

fn remove_dir() {
    let path = Path::new(TEST_FILE);
    if path.exists() {
        fs::remove_dir_all(path).unwrap();
    }
}

fn open_db_and_insert_data() -> Database {
    create_dir();
    let mut def = DbMap::from(DB_DATA);
    add_bytes_column(&mut def);
    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);
    db
}

fn add_bytes_column(db_data: &mut DbMap) {
    db_data.insert(column::SECRET, Value::Bytes(column::SECRET.as_bytes().to_vec()));
    db_data.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    db_data.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    db_data.insert(column::CREATE_TIME, Value::Bytes(column::CREATE_TIME.as_bytes().to_vec()));
    db_data.insert(column::UPDATE_TIME, Value::Bytes(column::UPDATE_TIME.as_bytes().to_vec()));
}

fn backup_db(db: &Database) {
    fs::copy(&db.path, &db.backup_path).unwrap();
}

#[test]
fn create_and_drop_database() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    backup_db(&db);
    db.close_db();
    assert!(Database::delete(0).is_ok());
}

#[test]
fn database_version() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let calling_info = CallingInfo::new_self();
    let db = Database::build(&calling_info).unwrap();
    assert_eq!(1, db.get_version().unwrap());
    assert!(db.set_version(2).is_ok());
    assert_eq!(2, db.get_version().unwrap());
    let _ = Database::delete(0);
}

#[test]
fn error_sql() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let calling_info = CallingInfo::new_self();
    let db = Database::build(&calling_info).unwrap();
    let sql = "pragma zzz user_version = {} mmm";
    assert!(db.exec(sql).is_err());
    let _ = Database::delete(0);
}

#[test]
fn create_delete_asset_table() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    let table = Table::new(TABLE_NAME, &db);
    assert!(table.exist().unwrap());
    assert!(table.delete().is_ok());
    assert!(!table.exist().unwrap());
    db.close_db();
    let _ = Database::delete(0);
}

#[test]
fn insert_data_with_different_alias() {
    create_dir();
    let mut def = DbMap::from(DB_DATA);
    add_bytes_column(&mut def);

    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);

    def.insert(column::ALIAS, Value::Bytes(b"Alias2".to_vec()));
    let count = db.insert_datas(&def).unwrap();
    assert_eq!(count, 1);

    let ret = db
        .query_datas(
            &vec![],
            &DbMap::from([(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()))]),
            None,
            false,
        )
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

    let ret = db.is_data_exists(&datas, false).unwrap();
    assert!(ret);

    let count = db.delete_datas(&datas, None, false).unwrap();
    assert_eq!(count, 1);

    let ret = db.is_data_exists(&datas, false).unwrap();
    assert!(!ret);

    remove_dir();
}

#[test]
fn update_data() {
    let mut db = open_db_and_insert_data();

    let mut datas = DbMap::new();
    datas.insert(column::OWNER, Value::Bytes(column::OWNER.as_bytes().to_vec()));
    datas.insert(column::ALIAS, Value::Bytes(column::ALIAS.as_bytes().to_vec()));
    let update_time: Vec<u8> = vec![2];
    let count = db
        .update_datas(&datas, true, &DbMap::from([(column::UPDATE_TIME, Value::Bytes(update_time.clone()))]))
        .unwrap();
    assert_eq!(count, 1);

    let res = db.query_datas(&vec![], &datas, None, false).unwrap();
    assert_eq!(res.len(), 1);
    let query_update_time = res[0].get_bytes_attr(&column::UPDATE_TIME).unwrap();
    assert_eq!(update_time.len(), query_update_time.len());
    for (ins, qy) in update_time.iter().zip(query_update_time.iter()) {
        assert_eq!(*ins, *qy);
    }

    remove_dir();
}

#[test]
fn query_ordered_data() {
    // insert two data
    create_dir();
    let mut def = DbMap::from(DB_DATA);
    add_bytes_column(&mut def);

    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
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
            false,
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
    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    assert!(db.insert_datas(&datas).is_err());
    remove_dir();
}

#[test]
fn backup_and_restore() {
    let db = open_db_and_insert_data();
    backup_db(&db);
    drop(db);

    // Destroy the main database.
    let mut db_file = OpenOptions::new().read(true).write(true).open("/data/asset_test/0/asset.db").unwrap();
    let _ = db_file.write(b"buffer buffer buffer").unwrap();

    // Recovery the main database.
    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    let mut def = DbMap::from(DB_DATA);
    add_bytes_column(&mut def);

    db.query_datas(&vec![], &def, None, false).unwrap();
    drop(db);
    remove_dir();
}

#[test]
fn insert_duplicated_data() {
    let mut db = open_db_and_insert_data();

    let mut def = DbMap::from(DB_DATA);
    add_bytes_column(&mut def);
    assert_eq!(ErrCode::Duplicated, db.insert_datas(&def).unwrap_err().code);

    drop(db);
    remove_dir();
}

#[test]
fn query_mismatch_type_data() {
    create_dir();
    let mut data = DbMap::from(DB_DATA);
    add_bytes_column(&mut data);
    data.insert(column::CREATE_TIME, Value::Number(1));
    let calling_info = CallingInfo::new_self();
    let mut db = Database::build(&calling_info).unwrap();
    db.insert_datas(&data).unwrap();

    assert_eq!(ErrCode::FileOperationError, db.query_datas(&vec![], &data, None, false).unwrap_err().code);
    drop(db);
    remove_dir();
}
