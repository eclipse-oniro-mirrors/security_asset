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

use std::fs;

use asset_definition::{DataType, Extension, Value};

use crate::{
    database::Database,
    statement::Statement,
    table::Table,
    types::{ColumnInfo, DbMap, SQLITE_DONE, SQLITE_ROW},
};

#[test]
fn create_delete_table() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let mut db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);
    assert!(!table.exist().unwrap());
    assert!(table.create(columns).is_ok());
    assert!(table.exist().unwrap());
    table.delete().unwrap();
    db.close();
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn table_restore() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let mut db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);
    table
        .create(&[ColumnInfo { name: "Id", data_type: DataType::Number, is_primary_key: true, not_null: true }])
        .unwrap();
    let count = table.insert_row(&DbMap::from([("Id", Value::Number(1))])).unwrap();
    assert_eq!(count, 1);
    fs::copy("/data/asset_test/0/asset.db", "/data/asset_test/0/asset.db.backup").unwrap();
    db.close();

    fs::remove_file("/data/asset_test/0/asset.db").unwrap();
    fs::copy("/data/asset_test/0/asset.db.backup", "/data/asset_test/0/asset.db").unwrap();
    db.open().unwrap();
    let table = Table::new("table_name", &db);
    let count = table.count_datas(&DbMap::new(), false).unwrap();
    assert_eq!(count, 1);
    db.close();
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[cfg(test)]
fn insert_test_data() -> Database {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let table = Table::new("table_name", &db);
    table.create(columns).unwrap();

    let mut datas = DbMap::new();
    datas.insert_attr("Owner", b"owner1".to_vec());
    datas.insert_attr("Alias", b"alias1".to_vec());
    datas.insert_attr("value", b"aaaa".to_vec());
    assert_eq!(1, table.insert_row(&datas).unwrap());

    datas.insert_attr("Owner", b"owner2".to_vec());
    datas.insert_attr("Alias", b"alias2".to_vec());
    datas.insert_attr("value", b"bbbb".to_vec());
    assert_eq!(1, table.insert_row(&datas).unwrap());

    datas.insert_attr("Owner", b"owner2".to_vec());
    datas.insert_attr("Alias", b"alias3".to_vec());
    datas.insert_attr("value", b"cccc".to_vec());
    assert_eq!(1, table.insert_row(&datas).unwrap());
    db
}

#[test]
fn execute_sql() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = insert_test_data();
    let sql = "select Owner,Alias from table_name where Id>?";
    let stmt = Statement::prepare(sql, &db).unwrap();
    assert!(stmt.bind_data(1, &Value::Number(1)).is_ok());

    let mut count = 0;
    while stmt.step().unwrap() == SQLITE_ROW {
        count += 1;
    }
    assert_eq!(count, 2);
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn data_life_circle() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = insert_test_data();
    let mut datas = DbMap::new();
    datas.insert_attr("Owner", b"owner1".to_vec());
    datas.insert_attr("Alias", b"alias1".to_vec());
    let table = Table::new("table_name", &db);
    assert!(table.is_data_exists(&datas, false).unwrap());

    datas.insert_attr("Owner", b"owner1".to_vec());
    datas.insert_attr("Alias", b"alias2".to_vec());
    assert!(!table.is_data_exists(&datas, false).unwrap());

    datas.insert_attr("Owner", b"owner2".to_vec());
    datas.insert_attr("Alias", b"alias3".to_vec());
    assert_eq!(1, table.update_row(&datas, false, &DbMap::from([("value", Value::Bytes(b"dddd".to_vec()))])).unwrap());
    assert_eq!(1, table.delete_row(&datas, None, false).unwrap());
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn single_data() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);
    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    table.create(columns).unwrap();
    db.exec("insert into table_name values(1, 'test')").unwrap();

    let stmt = Statement::prepare("select id,alias from table_name where id < ?", &db).unwrap();
    stmt.bind_data(1, &Value::Number(1000)).unwrap();

    while stmt.step().unwrap() == SQLITE_ROW {
        let count = stmt.data_count();
        assert_eq!(2, count);

        assert_eq!("id", stmt.query_column_name(0).unwrap());
        assert_eq!("alias", stmt.query_column_name(1).unwrap());

        let id = stmt.query_column_int(0);
        let alias = stmt.query_column_blob(1);
        assert_eq!(1, id);
        assert_eq!("test".as_bytes(), alias);
    }
    drop(stmt);
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn multiple_data() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);
    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    table.create(columns).unwrap();
    let data_set = &[
        [Value::Number(2), Value::Bytes(b"test2".to_vec())],
        [Value::Number(3), Value::Bytes(b"test3".to_vec())],
        [Value::Number(4), Value::Bytes(b"test4".to_vec())],
    ];
    let stmt = Statement::prepare("insert into table_name values(?, ?)", &db).unwrap();
    for data in data_set {
        stmt.reset().unwrap();
        stmt.bind_data(1, &data[0]).unwrap();
        stmt.bind_data(2, &data[1]).unwrap();
        assert_eq!(SQLITE_DONE, stmt.step().unwrap());
    }

    let stmt = Statement::prepare("select id,alias from table_name where id < ?", &db).unwrap();
    stmt.bind_data(1, &Value::Number(1000)).unwrap();
    let mut index = 0;
    while stmt.step().unwrap() == SQLITE_ROW {
        let data_count = stmt.data_count();
        assert_eq!(data_count, 2);

        let id = stmt.query_column_int(0);
        let alias = stmt.query_column_blob(1);
        assert_eq!(data_set[index][0], Value::Number(id));
        assert_eq!(data_set[index][1], Value::Bytes(alias.to_vec()));
        index += 1;
    }
    assert!(table.delete().is_ok());
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn insert_query_row() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    table.create(columns).unwrap();

    let datas = DbMap::from([("id", Value::Number(3)), ("alias", Value::Bytes(b"alias1".to_vec()))]);
    assert_eq!(table.insert_row(&datas).unwrap(), 1);
    let datas = DbMap::from([("alias", Value::Bytes(b"alias1".to_vec()))]);
    assert_eq!(table.insert_row(&datas).unwrap(), 1);

    let result_set = table.query_row(&vec![], &DbMap::new(), None, false, columns).unwrap();
    assert_eq!(result_set.len(), 2);

    let count = table.count_datas(&DbMap::new(), false).unwrap();
    assert_eq!(count, 2);
    let count = table.count_datas(&DbMap::from([("id", Value::Number(3))]), false).unwrap();
    assert_eq!(count, 1);

    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn update_delete_row() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    table.create(columns).unwrap();
    let datas = DbMap::from([("id", Value::Number(1)), ("alias", Value::Bytes(b"alias1".to_vec()))]);
    assert_eq!(table.insert_row(&datas).unwrap(), 1);
    let datas = DbMap::from([("id", Value::Number(2)), ("alias", Value::Bytes(b"alias2".to_vec()))]);
    assert_eq!(table.insert_row(&datas).unwrap(), 1);

    let conditions = DbMap::from([("id", Value::Number(2))]);
    let datas = DbMap::from([("alias", Value::Bytes(b"test_update".to_vec()))]);
    assert_eq!(table.update_row(&conditions, false, &datas).unwrap(), 1);
    assert!(table.is_data_exists(&datas, false).unwrap());
    assert_eq!(table.delete_row(&conditions, None, false).unwrap(), 1);
    assert!(!table.is_data_exists(&conditions, false).unwrap());

    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn upgrade_table() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    table.create(columns).unwrap();
    assert!(table
        .add_column(
            &ColumnInfo { name: "value", is_primary_key: false, not_null: false, data_type: DataType::Bytes },
            &None
        )
        .is_ok());
    assert!(table
        .add_column(
            &ColumnInfo { name: "value1", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
            &None
        )
        .is_err());
    assert!(table
        .add_column(
            &ColumnInfo { name: "value2", is_primary_key: true, not_null: true, data_type: DataType::Number },
            &None
        )
        .is_err());
    assert!(table
        .add_column(
            &ColumnInfo { name: "value3", is_primary_key: false, not_null: true, data_type: DataType::Number },
            &Some(Value::Number(1))
        )
        .is_ok());
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}

#[test]
fn replace_datas() {
    fs::create_dir_all("/data/asset_test/0").unwrap();
    let db = Database::build(0).unwrap();
    let table = Table::new("table_name", &db);

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    table.create(columns).unwrap();
    let datas = DbMap::from([("id", Value::Number(1)), ("alias", Value::Bytes(b"alias1".to_vec()))]);
    assert_eq!(table.insert_row(&datas).unwrap(), 1);
    let datas = DbMap::from([("id", Value::Number(2)), ("alias", Value::Bytes(b"alias2".to_vec()))]);
    assert_eq!(table.insert_row(&datas).unwrap(), 1);

    let conditions = DbMap::from([("id", Value::Number(2))]);
    let datas = DbMap::from([("id", Value::Number(3)), ("alias", Value::Bytes(b"alias3".to_vec()))]);
    table.replace_row(&conditions, false, &datas).unwrap();
    assert!(table.is_data_exists(&datas, false).unwrap());

    assert_eq!(table.count_datas(&conditions, false).unwrap(), 0);
    fs::remove_dir_all("/data/asset_test/0").unwrap();
}
