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
use std::fs;

use asset_db_operator::{
    database::*,
    statement::Statement,
    types::{ColumnInfo, SQLITE_OK},
};
use asset_definition::DataType;

#[test]
pub fn test_for_sqlite3_open() {
    let _ = match Database::new("test.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    match Database::new("/root/test.db") {
        Ok(_) => {
            panic!("read root");
        },
        Err(ret) => {
            println!("expected fault {}", ret);
        },
    };
    let _ = fs::create_dir("db");
}

#[test]
pub fn test_for_drop_database() {
    let _ = match Database::new("test1.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    let _ = Database::drop_database("test1.db");
}

#[test]
pub fn test_for_update_version() {
    let db = match Database::new("test0.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    if db.update_version(1) != SQLITE_OK {
        panic!("update version fail");
    }
}

#[test]
pub fn test_for_error_exec() {
    let db = match Database::new("test1.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    let sql = "pragma zzz user_version = {} mmm";
    let statement = Statement::new(sql, &db);
    let ret = statement.exec(None, 0);
    assert_ne!(ret, 0);
}

#[test]
pub fn test_for_open_table() {
    let db = match Database::new("test3.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    let table = db.open_table("table_name");
    match table {
        Ok(_o) => {
            println!("open table succ");
        },
        Err(e) => {
            panic!("expect open table fail {}", e);
        },
    }

    let _ = Database::drop_database("test4.db");

    let db = match Database::new("test4.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let _table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let _ = match db.open_table("table_test") {
        Ok(o) => {
            println!("open table succ");
            o
        },
        Err(e) => {
            panic!("open table fail {}", e)
        },
    };
}

#[test]
pub fn test_for_drop_table() {
    let db = match Database::new("test5.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let _table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let ret = db.drop_table("table_test");
    assert_eq!(ret, SQLITE_OK);
}
