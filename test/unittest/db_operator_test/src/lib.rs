//!
//! Copyright (C) 2023 Huawei Device Co., Ltd.
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//! http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
use core::panic;
use std::{
    cmp::Ordering,
    fs::{self, OpenOptions},
    io::Write,
};

use asset_common::definition::{DataType, Value};
use db_operator::{
    database::*,
    database_table_helper::{
        do_transaction, DefaultDatabaseHelper, ASSET_TABLE_NAME, COLUMN_ALIAS, COLUMN_OWNER,
    },
    statement::Statement,
    types::{
        from_data_value_to_str_value, ColumnInfo,
        QueryOptions, DbMap,
    },
    SQLITE_DONE, SQLITE_OK, SQLITE_OPEN_CREATE, SQLITE_OPEN_READWRITE, SQLITE_ROW,
};

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

    let _ = match Database::new_v2("db/test.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, None) {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
}

#[test]
pub fn test_for_sqlite3_v2_open() {
    let _ = match Database::new_v2("test_v2.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, None) {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    match Database::new_v2(
        "/root/test_v2.db",
        SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
        Some(b"unix-dotfile"),
    ) {
        Ok(_) => {
            panic!("read root");
        },
        Err(ret) => {
            println!("expected fault {}", ret);
        },
    };
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
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
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
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
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

#[test]
pub fn test_for_statement_column() {
    let _ = Database::drop_database("test6.db");
    let db = match Database::new("test6.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let _ = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let stmt = Statement::<false>::new("insert into table_test values(1, 'test')", &db);
    let code = stmt.exec(None, 0);
    assert_eq!(code, SQLITE_OK);

    let stmt1 =
        Statement::<true>::prepare("select id,alias from table_test where id < ?", &db).unwrap();
    let ret = stmt1.bind_data(1, &Value::Number(1000));
    assert_eq!(ret, SQLITE_OK);
    let count = stmt1.column_count();
    assert_eq!(count, 2);
    for i in 0..count {
        let s = stmt1.query_column_name(i).unwrap();
        println!("column {} = {}", i, s);
        //sqlite3_free_func(s.as_ptr() as _);
    }
    // query by query_column
    while stmt1.step() == SQLITE_ROW {
        let out_id = stmt1.query_column(0, &DataType::Uint32).unwrap();
        let out_alias = stmt1.query_column(1, &DataType::Bytes).unwrap();
        println!("id is {}", from_data_value_to_str_value(&out_id));
        println!("alias is {}", from_data_value_to_str_value(&out_alias));
        if let (Value::Number(id), Value::Bytes(alias)) = (out_id, out_alias) {
            let alias_str = unsafe { String::from_utf8_unchecked(alias.to_vec()) };
            println!("line 0 : id {} alias {}", id, alias_str);
        }
        let data_count = stmt1.data_count();
        assert_eq!(data_count, 2);
        println!("data count {}", data_count);
    }
    println!("first");

    // multi insert
    let dataset = &[
        [Value::Number(2), Value::Bytes(b"test2".to_vec())],
        [Value::Number(3), Value::Bytes(b"test3".to_vec())],
        [Value::Number(4), Value::Bytes(b"test4".to_vec())],
    ];

    let stmt2 = Statement::<true>::prepare("insert into table_test values(?, ?)", &db).unwrap();
    for data in dataset {
        let ret = stmt2.reset();
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(1, &data[0]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(2, &data[1]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.step();
        assert_eq!(ret, SQLITE_DONE);
    }

    let stmt1 =
        Statement::<true>::prepare("select id,alias from table_test where id < ?", &db).unwrap();
    let ret = stmt1.bind_data(1, &Value::Number(1000));
    assert_eq!(ret, SQLITE_OK);
    let count = stmt1.column_count();
    assert_eq!(count, 2);
    for i in 0..count {
        let s = stmt1.query_column_name(i).unwrap();
        println!("column {} = {}", i, s);
        //sqlite3_free_func(s.as_ptr() as _);
    }
    // multi query
    while stmt1.step() == SQLITE_ROW {
        let id = stmt1.query_column_int(0);
        let alias = stmt1.query_column_text(1);
        let alias_str = unsafe { String::from_utf8_unchecked(alias.to_vec()) };
        println!("line 0 : id = {} , alias = {}", id, alias_str);
        let data_count = stmt1.data_count();
        assert_eq!(data_count, 2);
        println!("data count {}", data_count);
    }

    let ret = db.drop_table("table_test");
    assert_eq!(ret, SQLITE_OK);
}

#[test]
pub fn test_update_row() {
    let _ = Database::drop_database("test7.db");
    let db = match Database::new("test7.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let dataset = &[
        [Value::Number(2), Value::Bytes(b"test2".to_vec())],
        [Value::Number(3), Value::Bytes(b"test3".to_vec())],
        [Value::Number(4), Value::Bytes(b"test4".to_vec())],
    ];

    let stmt2 = Statement::<true>::prepare("insert into table_test values(?, ?)", &db).unwrap();
    for data in dataset {
        let ret = stmt2.reset();
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(1, &data[0]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(2, &data[1]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.step();
        assert_eq!(ret, SQLITE_DONE);
    }

    // update
    let conditions = DbMap::from([
        ("id", Value::Number(2)),
    ]);
    let datas = DbMap::from([
        ("alias", Value::Bytes(b"test_update".to_vec()))
    ]);

    let ret = table.update_row(&conditions, &datas).unwrap();
    assert_eq!(ret, 1);
    let ret = table
        .update_row_column(&conditions, COLUMN_ALIAS, Value::Bytes(b"test_update1".to_vec()))
        .unwrap();
    assert_eq!(ret, 1);
    let stmt = Statement::<true>::prepare("select * from table_test where id=2", &db).unwrap();
    let ret = stmt.step();
    assert_eq!(ret, SQLITE_ROW);
    let alias = stmt.query_column_text(1);
    assert_eq!(alias, b"test_update1");
}

#[test]
pub fn test_for_insert_row() {
    let _ = Database::drop_database("test8.db");
    let db = match Database::new("test8.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let datas = DbMap::from([
        ("id", Value::Number(3)),
        ("alias", Value::Bytes(b"alias1".to_vec()))
    ]);

    let count = table.insert_row(&datas).unwrap();
    assert_eq!(count, 1);
    let datas = DbMap::from([
        ("alias", Value::Bytes(b"alias1".to_vec()))
    ]);

    let count = table.insert_row(&datas).unwrap();
    assert_eq!(count, 1);
}

#[test]
pub fn test_update_datas() {
    let _ = Database::drop_database("test9.db");
    let db = match Database::new("test9.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let dataset = &[
        DbMap::from([
            ("Owner", Value::Bytes(b"owner1".to_vec())),
            ("Alias", Value::Bytes(b"alias1".to_vec())),
            ("value", Value::Bytes(b"value1".to_vec())),
        ]),
        DbMap::from([
            ("Owner", Value::Bytes(b"owner2".to_vec())),
            ("Alias", Value::Bytes(b"alias2".to_vec())),
            ("value", Value::Bytes(b"value2".to_vec())),
        ]),
        DbMap::from([
            ("Owner", Value::Bytes(b"owner3".to_vec())),
            ("Alias", Value::Bytes(b"alias3".to_vec())),
            ("value", Value::Bytes(b"value3".to_vec())),
        ]),
    ];

    for data in dataset {
        let count = table.insert_row(data).unwrap();
        assert_eq!(count, 1);
    }

    // update
    let datas = DbMap::from([
        ("value", Value::Bytes(b"value_new".to_vec())),
    ]);

    let count = db
        .update_datas_default(
            &DbMap::from([
                ("Owner", Value::Bytes(b"owner2".to_vec())),
                ("Alias", Value::Bytes(b"alias2".to_vec())),
            ]),
            &datas,
        )
        .unwrap();
    assert_eq!(count, 1);
    // query
    let stmt =
        Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db)
            .unwrap();
    let od = Value::Bytes(b"owner2".to_vec());
    let ad = Value::Bytes(b"alias2".to_vec());
    let ret = stmt.bind_data(1, &od);
    assert_eq!(ret, 0);
    let ret = stmt.bind_data(2, &ad);
    assert_eq!(ret, 0);
    let ret = stmt.step();
    assert_eq!(ret, SQLITE_ROW);
    let alias = stmt.query_column_text(3);
    assert_eq!(alias, b"value_new");
}

#[test]
pub fn test_insert_datas() {
    let _ = Database::drop_database("test10.db");
    let db = match Database::new("test10.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let _table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let dataset = DbMap::from([
        ("value", Value::Bytes(b"value".to_vec())),
        (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
    ]);

    let count = db.insert_datas_default(&dataset).unwrap();
    assert_eq!(count, 1);

    // query
    let stmt =
        Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db)
            .unwrap();
    let ownerd = Value::Bytes(b"owner1".to_vec());
    let aliasd = Value::Bytes(b"alias1".to_vec());
    let ret = stmt.bind_data(1, &ownerd);
    assert_eq!(ret, 0);
    let ret = stmt.bind_data(2, &aliasd);
    assert_eq!(ret, 0);
    let ret = stmt.step();
    assert_eq!(ret, SQLITE_ROW);
    let alias = stmt.query_column_text(3);
    assert_eq!(alias, b"value");
}

#[test]
pub fn test_insert_row_datas() {
    let _ = Database::drop_database("test15.db");
    let db = match Database::new("test15.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let dataset = vec![
        Value::Number(2),
        Value::Bytes(b"owner1".to_vec()),
        Value::Bytes(b"alias1".to_vec()),
        Value::Bytes(b"bbbb".to_vec()),
    ];
    let count = table.insert_row_datas(&dataset).unwrap();
    assert_eq!(count, 1);

    // query
    let stmt =
        Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db)
            .unwrap();
    let od = Value::Bytes(b"owner1".to_vec());
    let ad = Value::Bytes(b"alias1".to_vec());
    let ret = stmt.bind_data(1, &od);
    assert_eq!(ret, 0);
    let ret = stmt.bind_data(2, &ad);
    assert_eq!(ret, 0);
    let ret = stmt.step();
    assert_eq!(ret, SQLITE_ROW);
    let alias = stmt.query_column_text(3);
    assert_eq!(alias, b"bbbb");
}

#[test]
pub fn test_delete_datas() {
    let _ = Database::drop_database("test11.db");
    let db = match Database::new("test11.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let _table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let dataset = DbMap::from([
        ("value", Value::Bytes(b"value".to_vec())),
        (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
    ]);

    let count = db.insert_datas_default(&dataset).unwrap();
    assert_eq!(count, 1);

    let cond = DbMap::from([
        ("value", Value::Bytes(b"value".to_vec())),
        (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
    ]);

    let count = db.delete_datas_default(&cond).unwrap();
    assert_eq!(count, 1);

    let cond = DbMap::from([
        ("value", Value::Bytes(b"value".to_vec())),
    ]);

    let count = db.delete_datas_default(&cond).unwrap();
    assert_eq!(count, 0); // can not delete any data because no data
    let count = db
        .delete_datas_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert_eq!(count, 0); // can not delete any data because no data

    // query
    let stmt = Statement::<true>::prepare("select * from asset_table", &db).unwrap();
    let ret = stmt.step();
    assert_eq!(ret, SQLITE_DONE); // no data select
}

#[test]
pub fn test_for_rename() {
    let _ = Database::drop_database("test12.db");
    let db = match Database::new("test12.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let mut table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let ret = table.rename("name");
    assert_eq!(ret, SQLITE_OK);
}

#[test]
pub fn test_for_add_column() {
    let _ = Database::drop_database("test13.db");
    let db = match Database::new("test13.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let ret = table.add_new_column(
        ColumnInfo {
            name: "nid",
            data_type: DataType::Uint32,
            is_primary_key: false,
            not_null: true,
        },
        Some(Value::Number(0)),
    );
    assert_eq!(ret, SQLITE_OK);

    let ret = table.add_new_column(
        ColumnInfo {
            name: "n_n_id",
            data_type: DataType::Uint32,
            is_primary_key: true,
            not_null: true,
        },
        Some(Value::Number(0)),
    );
    assert_ne!(ret, SQLITE_OK);

    let ret = table.add_new_column(
        ColumnInfo {
            name: "n_n_n_id",
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: true,
        },
        Some(Value::Bytes(b"".to_vec())),
    );
    assert_ne!(ret, SQLITE_OK);
}

#[test]
pub fn test_query() {
    let _ = Database::drop_database("test14.db");
    let db = match Database::new("test14.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: false,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "blobs",
            is_primary_key: false,
            not_null: false,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let dataset = &[
        [Value::Number(2), Value::Bytes(b"test2".to_vec()), Value::Bytes(b"blob2".to_vec())],
        [Value::Number(3), Value::Bytes(b"test3".to_vec()), Value::Bytes(b"blob3".to_vec())],
        [Value::Number(4), Value::Bytes(b"test4".to_vec()), Value::Bytes(b"blob4".to_vec())],
    ];

    let stmt2 = Statement::<true>::prepare("insert into table_test values(?, ?, ?)", &db).unwrap();
    for data in dataset {
        let ret = stmt2.reset();
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(1, &data[0]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(2, &data[1]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.bind_data(3, &data[2]);
        assert_eq!(ret, SQLITE_OK);
        let ret = stmt2.step();
        assert_eq!(ret, SQLITE_DONE);
    }

    let result_set = table.query_row(&vec!["alias", "blobs"], &DbMap::new(), None).unwrap();
    println!("id alias blobs");
    for data_line in &result_set {
        print!("line: ");
        for data in data_line {
            print!("{} ", from_data_value_to_str_value(data));
        }
        println!()
    }
    assert_eq!(result_set.len(), 5);
    let count = table.count_datas(&DbMap::new()).unwrap();
    assert_eq!(count, 5);
    let count =
        table.count_datas(&DbMap::from([("id", Value::Number(3))])).unwrap();
    assert_eq!(count, 1);
    let exits = table
        .is_data_exists(&DbMap::from([
            ("id", Value::Number(3)),
            ("alias", Value::Bytes(b"testtest".to_vec())),
        ]))
        .unwrap();
    assert!(!exits);
}

#[test]
pub fn test_multi_insert_row_datas() {
    let _ = Database::drop_database("test16.db");
    let db = match Database::new("test16.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            Value::Bytes(b"owner1".to_vec()),
            Value::Bytes(b"alias1".to_vec()),
            Value::Bytes(b"aaaa".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias2".to_vec()),
            Value::Bytes(b"bbbb".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner3".to_vec()),
            Value::Bytes(b"alias3".to_vec()),
            Value::Bytes(b"cccc".to_vec()),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    let dataset = vec![
        vec![
            Value::Number(5),
            Value::Bytes(b"owner1".to_vec()),
            Value::Bytes(b"alias1".to_vec()),
            Value::Bytes(b"aaaa".to_vec()),
        ],
        vec![
            Value::Number(6),
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias2".to_vec()),
            Value::Bytes(b"bbbb".to_vec()),
        ],
        vec![
            Value::Number(7),
            Value::Bytes(b"owner3".to_vec()),
            Value::Bytes(b"alias3".to_vec()),
            Value::Bytes(b"cccc".to_vec()),
        ],
    ];
    let count =
        table.insert_multi_row_datas(&vec!["Id", "Owner", "Alias", "value"], &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let stmt =
        Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db)
            .unwrap();
    let od = Value::Bytes(b"owner1".to_vec());
    let ad = Value::Bytes(b"alias1".to_vec());
    let ret = stmt.bind_data(1, &od);
    assert_eq!(ret, 0);
    let ret = stmt.bind_data(2, &ad);
    assert_eq!(ret, 0);
    let ret = stmt.step();
    assert_eq!(ret, SQLITE_ROW);
    let alias = stmt.query_column_text(3);
    assert_eq!(alias, b"aaaa");
}

#[test]
pub fn test_data_exists_and_data_count() {
    let _ = Database::drop_database("test17.db");
    let db = match Database::new("test17.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            Value::Bytes(b"owner1".to_vec()),
            Value::Bytes(b"alias1".to_vec()),
            Value::Bytes(b"aaaa".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias2".to_vec()),
            Value::Bytes(b"bbbb".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias3".to_vec()),
            Value::Bytes(b"cccc".to_vec()),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let exist = db
        .is_data_exists_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(!exist);

    let count = db
        .select_count_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner2".to_vec())),
        ])).unwrap();
    assert_eq!(count, 2);
}

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
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            Value::Bytes(b"owner1".to_vec()),
            Value::Bytes(b"alias1".to_vec()),
            Value::Bytes(b"aaaa".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias2".to_vec()),
            Value::Bytes(b"bbbb".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias3".to_vec()),
            Value::Bytes(b"cccc".to_vec()),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let exist = db
        .is_data_exists_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(!exist);

    let count = db
        .select_count_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner2".to_vec())),
        ]))
        .unwrap();
    assert_eq!(count, 2);

    let ret = db
        .insert_datas_default(&DbMap::from([
                ("value", Value::Bytes(b"value4".to_vec())),
                (COLUMN_OWNER, Value::Bytes(b"owner4".to_vec())),
                (COLUMN_ALIAS, Value::Bytes(b"alias4".to_vec())),
            ]))
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .update_datas_default(
            &DbMap::from([
                (COLUMN_OWNER, Value::Bytes(b"owner4".to_vec())),
                (COLUMN_ALIAS, Value::Bytes(b"alias4".to_vec())),
            ]),
            &DbMap::from([
                ("value", Value::Bytes(b"value5".to_vec())),
            ])
        )
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .delete_datas_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner4".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias4".to_vec())),
        ]))
        .unwrap();
    assert_eq!(ret, 1);

    let result = db
        .query_datas_default(
            &DbMap::from([
                (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
                (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
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
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            Value::Bytes(b"owner1".to_vec()),
            Value::Bytes(b"alias1".to_vec()),
            Value::Bytes(b"aaaa".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias2".to_vec()),
            Value::Bytes(b"bbbb".to_vec()),
        ],
        vec![
            Value::Bytes(b"owner2".to_vec()),
            Value::Bytes(b"alias3".to_vec()),
            Value::Bytes(b"cccc".to_vec()),
        ],
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
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::Uint32,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::Bytes,
        },
    ];
    let _ = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    drop(db);
    let db2 =
        Database::new_with_version_update("test20.db", 0, default_update_database_func).unwrap();
    drop(db2);

    let db3 =
        Database::new_with_version_update("test20.db", 1, default_update_database_func).unwrap();
    drop(db3);

    let db4 = Database::new_with_version_update("test20.db", 0, default_update_database_func);
    assert!(db4.is_err());
}

pub fn test_for_default_asset(userid: i32) {
    // let _ = Database::drop_default_database(userid);
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
        (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        (COLUMN_ALIAS, Value::Bytes(b"Alias1".to_vec())),
    ]);

    let count = DefaultDatabaseHelper::insert_datas_default_once(userid, &def).unwrap();
    assert_eq!(count, 1);

    def.remove(COLUMN_ALIAS);
    def.insert(COLUMN_ALIAS, Value::Bytes(b"Alias2".to_vec()));

    let count = DefaultDatabaseHelper::insert_datas_default_once(userid, &def).unwrap();
    assert_eq!(count, 1);

    let count = DefaultDatabaseHelper::update_datas_default_once(
        userid,
        &DbMap::from([
            ("Owner", Value::Bytes(b"owner1".to_vec())),
            ("Alias", Value::Bytes(b"Alias1".to_vec())),
        ]),
        &DbMap::from([
            ("UpdateTime", Value::Number(1)),
        ])
    )
    .unwrap();
    assert!(count >= 0);

    let _count = DefaultDatabaseHelper::select_count_default_once(
        userid,
        &DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        ])
    )
    .unwrap();

    let _ret = DefaultDatabaseHelper::is_data_exists_default_once(
        userid,
        &DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"Alias2".to_vec())),
        ]),
    )
    .unwrap();

    let count = DefaultDatabaseHelper::delete_datas_default_once(
        userid,
        &DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"Alias1".to_vec())),
        ]),
    )
    .unwrap();
    assert!(count >= 0);

    let query = QueryOptions {
        limit: Some(100),
        offset: Some(0),
        order: Some(Ordering::Greater),
        order_by: Some(vec![COLUMN_OWNER, COLUMN_ALIAS]),
    };

    let result = DefaultDatabaseHelper::query_datas_default_once(
        userid,
        &DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"Alias2".to_vec())),
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

    let result = DefaultDatabaseHelper::query_columns_default_once(
        userid,
        &vec!["Id", "Alias"],
        &DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"Alias2".to_vec())),
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
    // let db = DefaultDatabaseHelper::open_default_database_table(userid).unwrap();
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
            &[ColumnInfo {
                name: "Id",
                data_type: DataType::Uint32,
                is_primary_key: true,
                not_null: true,
            }],
        )
        .unwrap();
    let count =
        table.insert_row(&DbMap::from([
            ("Id", Value::Number(1)),
        ])).unwrap();
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
    let count = db
        .select_count_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        ])).unwrap();
    assert_eq!(count, 0);
    true
}

#[test]
pub fn test_for_transaction3() {
    let ret = do_transaction(6, trans_call).unwrap();
    assert!(ret);
    let trans = |db: &Database| -> bool {
        let count = db
            .select_count_default(&DbMap::from([
                (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            ]))
            .unwrap();
        assert_eq!(count, 0);
        true
    };
    let ret = do_transaction(6, trans).unwrap();
    assert!(ret);
}

#[test]
pub fn test_for_error() {
    let stmt = DefaultDatabaseHelper::insert_datas_default_once(
        1,
        &DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner".to_vec())),
            (COLUMN_OWNER, Value::Bytes(b"alias".to_vec())),
        ]),
    );
    assert!(stmt.is_err());
}

#[test]
pub fn test_for_master_backup() {
    let _ = Database::drop_default_database_and_backup(5);
    let db = DefaultDatabaseHelper::open_default_database_table(5).unwrap();
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
        (COLUMN_OWNER, Value::Bytes(b"owner".to_vec())),
        (COLUMN_ALIAS, Value::Bytes(b"Alias".to_vec())),
    ]);

    db.insert_datas_default(&def).unwrap();
    drop(db);
    let mut db_file = OpenOptions::new()
        .read(true)
        .write(true)
        .open("/data/service/el1/public/asset_service/5/asset.db")
        .unwrap(); // write master db
    let _ = db_file.write(b"buffer buffer buffer").unwrap();
    let db = DefaultDatabaseHelper::open_default_database_table(5).unwrap(); // will recovery master db
    db.insert_datas_default(&def).unwrap();
    drop(db);
    let mut back_file = OpenOptions::new()
        .read(true)
        .write(true)
        .open("/data/service/el1/public/asset_service/5/asset.db.backup")
        .unwrap(); // write backup db
    let _ = back_file.write(b"bad message info").unwrap();
    let db = DefaultDatabaseHelper::open_default_database_table(5).unwrap(); // will recovery backup db
    db.insert_datas_default(&def).unwrap();
    let count = db
        .select_count_default(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner".to_vec())),
        ])).unwrap();
    assert_eq!(count, 3);
    drop(db);
}
