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

use db_operator::{
    database::*,
    database_table_helper::G_ASSET_TABLE_NAME,
    statement::Statement,
    types::{
        from_result_datatype_to_str, from_resultvalue_to_str_value, ColumnInfo, DataType,
        DataValue, Pair, ResultDataValue,
    },
    SQLITE_DONE, SQLITE_OK, SQLITE_OPEN_CREATE, SQLITE_OPEN_READWRITE, SQLITE_ROW,
};

#[test]
pub fn test_for_selite3_open() {
    let _ = match Database::new("test.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };

    match Database::new("/root/test.db") {
        Ok(_) => {
            panic!("read root");
        }
        Err(ret) => {
            println!("expected fault {}", ret);
        }
    };
}

#[test]
pub fn test_for_selite3_v2_open() {
    let _ = match Database::new_v2(
        "testv2.db",
        SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
        None,
    ) {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };

    match Database::new_v2(
        "/root/testv2.db",
        SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
        Some(b"unix-dotfile"),
    ) {
        Ok(_) => {
            panic!("read root");
        }
        Err(ret) => {
            println!("expected fault {}", ret);
        }
    };
}

#[test]
pub fn test_for_drop_database() {
    let _ = match Database::new("test1.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };
    let _ =  Database::drop_database("test1.db");
}

#[test]
pub fn test_for_update_version() {
    let db = match Database::new("test0.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
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
        }
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
        }
    };
    let table = db.open_table("table_name");
    match table {
        Ok(_o) => {
            panic!("open table succ");
        }
        Err(e) => {
            println!("expect open table fail {}", e);
        }
    }

    let _ = Database::drop_database("test4.db");

    let db = match Database::new("test4.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let _table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };

    let _ = match db.open_table("table_test") {
        Ok(o) => {
            println!("open table succ");
            o
        }
        Err(e) => {
            panic!("open table fail {}", e)
        }
    };
}

#[test]
pub fn test_for_drop_table() {
    let db = match Database::new("test5.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let _table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let _ = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };

    let stmt = Statement::<false>::new("insert into table_test values(1, 'test')", &db);
    let code = stmt.exec(None, 0);
    assert_eq!(code, SQLITE_OK);

    let stmt1 =
        Statement::<true>::prepare("select id,alias from table_test where id < ?", &db).unwrap();
    let ret = stmt1.bind_data(1, &DataValue::Integer(1000));
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
        let mut out_id = ResultDataValue::Integer(0);
        stmt1.query_column(0, &mut out_id);
        let mut out_alias = ResultDataValue::Text(None);
        stmt1.query_column(1, &mut out_alias);
        println!("id is {}", from_result_datatype_to_str(&out_id));
        println!("alias is {}", from_result_datatype_to_str(&out_alias));
        if let (ResultDataValue::Integer(id), ResultDataValue::Text(Some(alias))) =
            (out_id, out_alias)
        {
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
        [DataValue::Integer(2), DataValue::Text(b"test2")],
        [DataValue::Integer(3), DataValue::Text(b"test3")],
        [DataValue::Integer(4), DataValue::Text(b"test4")],
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
    let ret = stmt1.bind_data(1, &DataValue::Integer(1000));
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let dataset = &[
        [DataValue::Integer(2), DataValue::Text(b"test2")],
        [DataValue::Integer(3), DataValue::Text(b"test3")],
        [DataValue::Integer(4), DataValue::Text(b"test4")],
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
    let conditions = &vec![Pair {
        column_name: "id",
        value: DataValue::Integer(2),
    }];
    let datas = &vec![Pair {
        column_name: "alias",
        value: DataValue::Text(b"test_update"),
    }];
    let ret = table.update_row(conditions, datas).unwrap();
    assert_eq!(ret, 1);
    let ret = table
        .update_row_column(conditions, "alias", DataValue::Text(b"test_update1"))
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };

    let datas = &vec![
        Pair {
            column_name: "id",
            value: DataValue::Integer(3),
        },
        Pair {
            column_name: "alias",
            value: DataValue::Text(b"alias1"),
        },
    ];
    let count = table.insert_row(datas).unwrap();
    assert_eq!(count, 1);
    let datas = &vec![Pair {
        column_name: "alias",
        value: DataValue::Text(b"alias1"),
    }];
    let count = table.insert_row(datas).unwrap();
    assert_eq!(count, 1);
}

#[test]
pub fn test_update_datas() {
    let _ = Database::drop_database("test9.db");
    let db = match Database::new("test9.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let dataset = &[
        &vec![
            Pair {
                column_name: "Owner",
                value: DataValue::Text(b"owner1"),
            },
            Pair {
                column_name: "Alias",
                value: DataValue::Text(b"alias1"),
            },
            Pair {
                column_name: "value",
                value: DataValue::Text(b"value1"),
            },
        ],
        &vec![
            Pair {
                column_name: "Owner",
                value: DataValue::Text(b"owner2"),
            },
            Pair {
                column_name: "Alias",
                value: DataValue::Text(b"alias2"),
            },
            Pair {
                column_name: "value",
                value: DataValue::Text(b"value2"),
            },
        ],
        &vec![
            Pair {
                column_name: "Owner",
                value: DataValue::Text(b"owner2"),
            },
            Pair {
                column_name: "Alias",
                value: DataValue::Text(b"alias3"),
            },
            Pair {
                column_name: "value",
                value: DataValue::Text(b"value3"),
            },
        ],
    ];

    for data in dataset {
        let count = table.insert_row(data).unwrap();
        assert_eq!(count, 1);
    }

    // update
    let datas = &vec![Pair {
        column_name: "value",
        value: DataValue::Text(b"value_new"),
    }];
    let count = db
        .update_datas(G_ASSET_TABLE_NAME, "owner2", "alias2", datas)
        .unwrap();
    assert_eq!(count, 1);
    // query
    let stmt = Statement::<true>::prepare(
        "select * from asset_table where Owner='owner2' and Alias='alias2'",
        &db,
    )
    .unwrap();
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let _table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let dataset = vec![Pair {
        column_name: "value",
        value: DataValue::Text(b"value"),
    }];
    let owner = "owner1";
    let alias = "alias1";
    let count = db
        .insert_datas(G_ASSET_TABLE_NAME, owner, alias, dataset)
        .unwrap();
    assert_eq!(count, 1);

    // query
    let stmt = Statement::<true>::prepare(
        "select * from asset_table where Owner='owner1' and Alias='alias1'",
        &db,
    )
    .unwrap();
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let dataset = vec![
        DataValue::Integer(2),
        DataValue::Text(b"owner1"),
        DataValue::Text(b"alias1"),
        DataValue::Text(b"bbbb"),
    ];
    let count = table.insert_row_datas(&dataset).unwrap();
    assert_eq!(count, 1);

    // query
    let stmt = Statement::<true>::prepare(
        "select * from asset_table where Owner='owner1' and Alias='alias1'",
        &db,
    )
    .unwrap();
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let _table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let dataset = vec![Pair {
        column_name: "value",
        value: DataValue::Text(b"value"),
    }];
    let owner = "owner1";
    let alias = "alias1";
    let count = db
        .insert_datas(G_ASSET_TABLE_NAME, owner, alias, dataset)
        .unwrap();
    assert_eq!(count, 1);

    let cond = vec![Pair {
        column_name: "value",
        value: DataValue::Text(b"value"),
    }];
    let count = db
        .delete_datas(G_ASSET_TABLE_NAME, owner, alias, &cond)
        .unwrap();
    assert_eq!(count, 1);

    let cond = vec![Pair {
        column_name: "value",
        value: DataValue::Text(b"value"),
    }];
    let count = db
        .delete_datas(G_ASSET_TABLE_NAME, owner, alias, &cond)
        .unwrap();
    assert_eq!(count, 0); // can not delete any data because no data
    let count = db
        .delete_datas(G_ASSET_TABLE_NAME, owner, alias, &vec![])
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let mut table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let ret = table.add_new_column(
        ColumnInfo {
            name: "nid",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: true,
        },
        Some(DataValue::Integer(0)),
    );
    assert_eq!(ret, SQLITE_OK);

    let ret = table.add_new_column(
        ColumnInfo {
            name: "nnid",
            data_type: DataType::INTEGER,
            is_primary_key: true,
            not_null: true,
        },
        Some(DataValue::Integer(0)),
    );
    assert_ne!(ret, SQLITE_OK);

    let ret = table.add_new_column(
        ColumnInfo {
            name: "nnnid",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: true,
        },
        Some(DataValue::Blob(b"")),
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "alias",
            is_primary_key: false,
            not_null: false,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "blobs",
            is_primary_key: false,
            not_null: false,
            data_type: DataType::BLOB,
        },
    ];
    let table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let dataset = &[
        [
            DataValue::Integer(2),
            DataValue::Text(b"test2"),
            DataValue::Blob(b"blob2"),
        ],
        [
            DataValue::Integer(3),
            DataValue::Text(b"test3"),
            DataValue::Blob(b"blob3"),
        ],
        [
            DataValue::Integer(4),
            DataValue::Text(b"test4"),
            DataValue::Blob(b"blob4"),
        ],
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

    let count = table
        .insert_row(&vec![
            Pair {
                column_name: "alias",
                value: DataValue::NoData,
            },
            Pair {
                column_name: "blobs",
                value: DataValue::Blob(b"blob5"),
            },
        ])
        .unwrap();
    assert_eq!(count, 1);
    let count = table
        .insert_row(&vec![
            Pair {
                column_name: "alias",
                value: DataValue::Text(b"test6"),
            },
            Pair {
                column_name: "blobs",
                value: DataValue::NoData,
            },
        ])
        .unwrap();
    assert_eq!(count, 1);

    let resultset = table.query_row(&vec!["alias", "blobs"], &vec![]).unwrap();
    println!("id alias blobs");
    for dataline in &resultset {
        print!("line: ");
        for data in dataline {
            print!("{} ", from_resultvalue_to_str_value(data));
        }
        println!()
    }
    assert_eq!(resultset.len(), 5);
    let count = table.count_datas(&vec![]).unwrap();
    assert_eq!(count, 5);
    let count = table
        .count_datas(&vec![Pair {
            column_name: "id",
            value: DataValue::Integer(3),
        }])
        .unwrap();
    assert_eq!(count, 1);
    let exits = table
        .is_data_exists(&vec![
            Pair {
                column_name: "id",
                value: DataValue::Integer(3),
            },
            Pair {
                column_name: "alias",
                value: DataValue::Text(b"testtest"),
            },
        ])
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            DataValue::Text(b"owner1"),
            DataValue::Text(b"alias1"),
            DataValue::Text(b"aaaa"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias2"),
            DataValue::Text(b"bbbb"),
        ],
        vec![
            DataValue::Text(b"owner3"),
            DataValue::Text(b"alias3"),
            DataValue::Text(b"cccc"),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let stmt = Statement::<true>::prepare(
        "select * from asset_table where Owner='owner1' and Alias='alias1'",
        &db,
    )
    .unwrap();
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            DataValue::Text(b"owner1"),
            DataValue::Text(b"alias1"),
            DataValue::Text(b"aaaa"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias2"),
            DataValue::Text(b"bbbb"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias3"),
            DataValue::Text(b"cccc"),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let exist = db
        .is_data_exists(G_ASSET_TABLE_NAME, "owner1", "alias1")
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists(G_ASSET_TABLE_NAME, "owner1", "alias2")
        .unwrap();
    assert!(!exist);

    let count = db.select_count(G_ASSET_TABLE_NAME, "owner2").unwrap();
    assert_eq!(count, 2);
}

#[test]
pub fn test_helper() {
    let _ = Database::drop_database("test18.db");
    let db = match Database::new("test18.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            DataValue::Text(b"owner1"),
            DataValue::Text(b"alias1"),
            DataValue::Text(b"aaaa"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias2"),
            DataValue::Text(b"bbbb"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias3"),
            DataValue::Text(b"cccc"),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let exist = db
        .is_data_exists(G_ASSET_TABLE_NAME, "owner1", "alias1")
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists(G_ASSET_TABLE_NAME, "owner1", "alias2")
        .unwrap();
    assert!(!exist);

    let count = db.select_count(G_ASSET_TABLE_NAME, "owner2").unwrap();
    assert_eq!(count, 2);

    let ret = db
        .insert_datas(
            G_ASSET_TABLE_NAME,
            "owner4",
            "alias4",
            vec![Pair {
                column_name: "value",
                value: DataValue::Text(b"value4"),
            }],
        )
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .update_datas(
            G_ASSET_TABLE_NAME,
            "owner4",
            "alias4",
            &vec![Pair {
                column_name: "value",
                value: DataValue::Text(b"value5"),
            }],
        )
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .delete_datas(G_ASSET_TABLE_NAME, "owner4", "alias4", &vec![])
        .unwrap();
    assert_eq!(ret, 1);

    let result = db
        .query_datas(G_ASSET_TABLE_NAME, "owner1", "alias1", &vec![])
        .unwrap();
    assert_eq!(result.len(), 1);
    for data in result {
        print!("line: ");
        for d in data {
            print!("{}, ", from_resultvalue_to_str_value(&d));
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
        }
    };

    let columns = &[
        ColumnInfo {
            name: "Id",
            is_primary_key: true,
            not_null: true,
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![
            DataValue::Text(b"owner1"),
            DataValue::Text(b"alias1"),
            DataValue::Text(b"aaaa"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias2"),
            DataValue::Text(b"bbbb"),
        ],
        vec![
            DataValue::Text(b"owner2"),
            DataValue::Text(b"alias3"),
            DataValue::Text(b"cccc"),
        ],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    let sql = "select Owner,Alias from asset_table where Id>?";
    let stmt = Statement::<true>::prepare(sql, &db).unwrap();
    let ret = stmt.bind_data(1, &DataValue::Integer(1));
    assert_eq!(ret, SQLITE_OK);

    while stmt.step() == SQLITE_ROW {
        print!("line: ");
        let owner = stmt.query_column_text(0);
        let alias = stmt.query_column_text(1);
        print!(
            "{} {}",
            from_resultvalue_to_str_value(&ResultDataValue::Text(Some(Box::new(owner.to_vec())))),
            from_resultvalue_to_str_value(&ResultDataValue::Text(Some(Box::new(alias.to_vec()))))
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
            data_type: DataType::INTEGER,
        },
        ColumnInfo {
            name: "Owner",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "Alias",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
        ColumnInfo {
            name: "value",
            is_primary_key: false,
            not_null: true,
            data_type: DataType::TEXT,
        },
    ];
    let _table = match db.create_table(G_ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        }
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
