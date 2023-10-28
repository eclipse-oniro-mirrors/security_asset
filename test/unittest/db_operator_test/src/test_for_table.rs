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

use asset_db_operator::{
    database::*,
    database_table_helper::{ASSET_TABLE_NAME, COLUMN_ALIAS, COLUMN_OWNER},
    statement::Statement,
    types::{from_data_value_to_str_value, ColumnInfo, DbMap, SQLITE_DONE, SQLITE_OK, SQLITE_ROW},
};
use asset_definition::{DataType, Value};

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
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
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

    let stmt1 = Statement::<true>::prepare("select id,alias from table_test where id < ?", &db).unwrap();
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
        let out_id = stmt1.query_column(0, &DataType::Number).unwrap();
        let out_alias = stmt1.query_column(1, &DataType::Bytes).unwrap();
        println!("id is {}", from_data_value_to_str_value(&out_id));
        println!("alias is {}", from_data_value_to_str_value(&out_alias));
        if let (Value::Number(id), Value::Bytes(ref alias)) = (out_id, out_alias) {
            let alias_str = unsafe { String::from_utf8_unchecked(alias.to_vec()) };
            println!("line 0 : id {} alias {}", id, alias_str);
        }
        let data_count = stmt1.data_count();
        assert_eq!(data_count, 2);
        println!("data count {}", data_count);
    }
    println!("first");
    drop(stmt);
    drop(stmt1);

    statement_func(db);
}

fn statement_func(db: Database<'_>) {
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

    let stmt1 = Statement::<true>::prepare("select id,alias from table_test where id < ?", &db).unwrap();
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
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
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
    let conditions = DbMap::from([("id", Value::Number(2))]);
    let datas = DbMap::from([("alias", Value::Bytes(b"test_update".to_vec()))]);

    let ret = table.update_row(&conditions, &datas).unwrap();
    assert_eq!(ret, 1);
    let ret = table.update_row_column(&conditions, COLUMN_ALIAS, Value::Bytes(b"test_update1".to_vec())).unwrap();
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
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let datas = DbMap::from([("id", Value::Number(3)), ("alias", Value::Bytes(b"alias1".to_vec()))]);

    let count = table.insert_row(&datas).unwrap();
    assert_eq!(count, 1);
    let datas = DbMap::from([("alias", Value::Bytes(b"alias1".to_vec()))]);

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
    let datas = DbMap::from([("value", Value::Bytes(b"value_new".to_vec()))]);

    let count = db
        .update_datas(
            &DbMap::from([("Owner", Value::Bytes(b"owner2".to_vec())), ("Alias", Value::Bytes(b"alias2".to_vec()))]),
            &datas,
        )
        .unwrap();
    assert_eq!(count, 1);
    // query
    let stmt = Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db).unwrap();
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
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
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

    let count = db.insert_datas(&dataset).unwrap();
    assert_eq!(count, 1);

    // query
    let stmt = Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db).unwrap();
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
    let dataset = vec![
        Value::Number(2),
        Value::Bytes(b"owner1".to_vec()),
        Value::Bytes(b"alias1".to_vec()),
        Value::Bytes(b"bbbb".to_vec()),
    ];
    let count = table.insert_row_datas(&dataset).unwrap();
    assert_eq!(count, 1);

    // query
    let stmt = Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db).unwrap();
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
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
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

    let count = db.insert_datas(&dataset).unwrap();
    assert_eq!(count, 1);

    let cond = DbMap::from([
        ("value", Value::Bytes(b"value".to_vec())),
        (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
        (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
    ]);

    let count = db.delete_datas(&cond).unwrap();
    assert_eq!(count, 1);

    let cond = DbMap::from([("value", Value::Bytes(b"value".to_vec()))]);

    let count = db.delete_datas(&cond).unwrap();
    assert_eq!(count, 0); // can not delete any data because no data
    let count = db
        .delete_datas(&DbMap::from([
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
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
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
    let ret = table.add_new_column(
        ColumnInfo { name: "nid", data_type: DataType::Number, is_primary_key: false, not_null: true },
        Some(Value::Number(0)),
    );
    assert_eq!(ret, SQLITE_OK);

    let ret = table.add_new_column(
        ColumnInfo { name: "n_n_id", data_type: DataType::Number, is_primary_key: true, not_null: true },
        Some(Value::Number(0)),
    );
    assert_ne!(ret, SQLITE_OK);

    let ret = table.add_new_column(
        ColumnInfo { name: "n_n_n_id", data_type: DataType::Bytes, is_primary_key: false, not_null: true },
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
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: false, data_type: DataType::Bytes },
        ColumnInfo { name: "blobs", is_primary_key: false, not_null: false, data_type: DataType::Bytes },
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
    assert_eq!(result_set.len(), 3);
    let count = table.count_datas(&DbMap::new()).unwrap();
    assert_eq!(count, 3);
    let count = table.count_datas(&DbMap::from([("id", Value::Number(3))])).unwrap();
    assert_eq!(count, 1);
    let exits = table
        .is_data_exists(&DbMap::from([("id", Value::Number(3)), ("alias", Value::Bytes(b"testtest".to_vec()))]))
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
        vec![Value::Bytes(b"owner3".to_vec()), Value::Bytes(b"alias3".to_vec()), Value::Bytes(b"cccc".to_vec())],
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
    let count = table.insert_multi_row_datas(&vec!["Id", "Owner", "Alias", "value"], &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let stmt = Statement::<true>::prepare("select * from asset_table where Owner=? and Alias=?", &db).unwrap();
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
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists(&DbMap::from([
            (COLUMN_OWNER, Value::Bytes(b"owner1".to_vec())),
            (COLUMN_ALIAS, Value::Bytes(b"alias2".to_vec())),
        ]))
        .unwrap();
    assert!(!exist);

    let count = db.select_count(&DbMap::from([(COLUMN_OWNER, Value::Bytes(b"owner2".to_vec()))])).unwrap();
    assert_eq!(count, 2);
}
