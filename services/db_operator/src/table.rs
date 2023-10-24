/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! sqlite table impl, build most of the sql.
//! all the column name should be static str, do NOT pass column name from user interface, it's not safe
//! the data can be from user input, we will prepare and bind data.
//! table is auto drop by RAII

use core::ffi::c_void;
use std::cmp::Ordering;

use crate::{
    database::Database,
    statement::Statement,
    types::{
        from_data_type_to_str, from_data_value_to_str_value, ColumnInfo, Condition, DbMap, QueryOptions, ResultSet,
        SqliteErrCode, SQLITE_DONE, SQLITE_ERROR, SQLITE_OK, SQLITE_ROW,
    },
};

use asset_definition::{DataType, Value};

extern "C" {
    fn SqliteChanges(db: *mut c_void) -> i32;
}

/// a database table
#[repr(C)]
pub struct Table<'a> {
    /// table name
    pub(crate) table_name: String,
    /// point to db
    pub(crate) db: &'a Database<'a>,
}

/// prepare statement with test output
#[inline(always)]
fn prepare_statement<'a>(table: &'a Table, sql: &mut str) -> Result<Statement<'a, true>, SqliteErrCode> {
    asset_log::loge!("{}", sql);
    let stmt = match Statement::<true>::prepare(sql, table.db) {
        Ok(s) => s,
        Err(e) => {
            let msg = table.db.get_err_msg().unwrap();
            asset_log::loge!("prepare stmt fail ret {}, info: {}", e, msg.s);
            return Err(e);
        },
    };
    Ok(stmt)
}

/// bind conditions for statement
#[inline(always)]
fn bind_conditions(conditions: &Condition, stmt: &Statement<true>, index: &mut i32) -> Result<(), SqliteErrCode> {
    bind_datas(conditions, stmt, index)
}

/// bind datas
#[inline(always)]
fn bind_datas(datas: &DbMap, stmt: &Statement<true>, index: &mut i32) -> Result<(), SqliteErrCode> {
    for (_, value) in datas.iter() {
        let ret = stmt.bind_data(*index, value);
        if ret != SQLITE_OK {
            return Err(ret);
        }
        *index += 1;
    }
    Ok(())
}

/// bind data values
#[inline(always)]
fn bind_data_values(datas: &Vec<Value>, stmt: &Statement<true>, index: &mut i32) -> Result<(), SqliteErrCode> {
    for data in datas {
        let ret = stmt.bind_data(*index, data);
        if ret != SQLITE_OK {
            return Err(ret);
        }
        *index += 1;
    }
    Ok(())
}

/// build sql columns not empty
#[inline(always)]
fn build_sql_columns_not_empty(columns: &Vec<&str>, sql: &mut String) {
    for i in 0..columns.len() {
        let column = &columns[i];
        sql.push_str(column);
        if i != columns.len() - 1 {
            sql.push(',');
        }
    }
}

/// build sql columns
#[inline(always)]
fn build_sql_columns(columns: &Vec<&str>, sql: &mut String) {
    if !columns.is_empty() {
        build_sql_columns_not_empty(columns, sql);
    } else {
        sql.push('*');
    }
}

/// build sql where
#[inline(always)]
fn build_sql_where(conditions: &Condition, sql: &mut String) {
    if !conditions.is_empty() {
        sql.push_str(" where ");
        for (i, column_name) in conditions.keys().enumerate() {
            sql.push_str(column_name);
            sql.push_str("=?");
            if i != conditions.len() - 1 {
                sql.push_str(" and ")
            }
        }
    }
}

/// build sql values
#[inline(always)]
fn build_sql_values(len: usize, sql: &mut String) {
    for i in 0..len {
        sql.push('?');
        if i != len - 1 {
            sql.push(',');
        }
    }
}

/// build sql options
fn build_sql_query_options(query_options: Option<&QueryOptions>, sql: &mut String) {
    if let Some(option) = query_options {
        if let Some(order_by) = &option.order_by {
            if !order_by.is_empty() {
                sql.push_str(" order by ");
                build_sql_columns_not_empty(order_by, sql);
            }
        }
        if let Some(order) = option.order {
            let str = if order == Ordering::Greater {
                "ASC"
            } else if order == Ordering::Less {
                "DESC"
            } else {
                ""
            };
            sql.push_str(format!(" {}", str).as_str());
        }
        if let Some(limit) = option.limit {
            sql.push_str(format!(" limit {}", limit).as_str());
        }
        if let Some(offset) = option.offset {
            sql.push_str(format!(" offset {}", offset).as_str());
        }
    }
}

fn get_column_info(columns: &'static [ColumnInfo], db_column: &str) -> &'static ColumnInfo {
    for column in columns.iter() {
        if column.name.eq(db_column) {
            return column;
        }
    }
    panic!()
}

impl<'a> Table<'a> {
    pub(crate) fn new(table_name: &str, db: &'a Database) -> Table<'a> {
        Table { table_name: table_name.to_string(), db }
    }

    /// the param conditions is to build sql after 'where'
    /// the param datas is to build sql between 'set' and 'where'
    /// like this sql: update table_test set alias='test_update' where id=2
    /// the code like:
    ///
    /// let conditions = &DbMap::from([("id", Value::Number(2)]);
    ///
    /// let datas = &DbMap::from([("alias", Value::Bytes(b"test_update")]);
    ///
    /// let ret = table.update_row(conditions, datas);
    pub fn update_row(&self, conditions: &Condition, datas: &DbMap) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("update {} set ", self.table_name);
        for (i, column_name) in datas.keys().enumerate() {
            sql.push_str(column_name);
            sql.push_str("=?");
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }
        build_sql_where(conditions, &mut sql);
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_datas(datas, &stmt, &mut index)?;
        bind_conditions(conditions, &stmt, &mut index)?;
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// update single data
    /// sql like: update table_name set column_name=data_new [where conditions]
    pub fn update_row_column(
        &self,
        conditions: &Condition,
        column_name: &'static str,
        data_new: Value,
    ) -> Result<i32, SqliteErrCode> {
        let mut datas = DbMap::new();
        datas.insert(column_name, data_new);
        self.update_row(conditions, &datas)
    }

    /// delete row from table
    /// like this sql: delete from table_test where id=2
    /// the code like:
    ///
    /// let conditions = &DbMap::from([("id", Value::Number(2)]);
    ///
    /// let ret = table.delete_row(conditions);
    pub fn delete_row(&self, conditions: &Condition) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("delete from {}", self.table_name);
        build_sql_where(conditions, &mut sql);
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_conditions(conditions, &stmt, &mut index)?;
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// insert into table, datas is the value to be insert.
    /// sql like: insert into table_test (id,alias) values (3,'alias1')
    /// code like this:
    ///
    /// let datas = &DbMap::from([("id", Value::Number(3), ("alias", Value::Bytes(b"alias1"))]);
    /// let ret = table.insert_row(datas);
    pub fn insert_row(&self, datas: &DbMap) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("insert into {} (", self.table_name);
        for (i, column_name) in datas.keys().enumerate() {
            sql.push_str(column_name);
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }

        sql.push_str(") values (");
        build_sql_values(datas.len(), &mut sql);
        sql.push(')');
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_datas(datas, &stmt, &mut index)?;
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// insert into table, datas is the value to be insert.
    /// sql like: insert into table_test values (3,'alias1')
    /// code like this:
    ///
    /// let datas = &vec![Value::Number(3), Value::Bytes(b"alias1")];
    /// let ret = table.insert_row_datas(datas);
    pub fn insert_row_datas(&self, datas: &Vec<Value>) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("insert into {} ", self.table_name);
        sql.push_str("values (");
        build_sql_values(datas.len(), &mut sql);
        sql.push(')');
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_data_values(datas, &stmt, &mut index)?;
        let ret = stmt.step();
        if ret != SQLITE_DONE {
            return Err(ret);
        }
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// insert into table, dataset is the value to be insert.
    /// sql like: insert into table_test values (3,'alias1')
    /// code like this:
    ///
    /// let columns = &vec!["AppId", "Alias", "value"];
    /// let dataset = vec![
    ///     vec![
    ///         Value::Bytes(b"appid1"),
    ///         Value::Bytes(b"alias1"),
    ///         Value::Bytes(b"a"),
    ///     ],
    ///     vec![
    ///         Value::Bytes(b"appid2"),
    ///         Value::Bytes(b"alias2"),
    ///         Value::Bytes(b"b"),
    ///     ],
    ///     vec![
    ///         Value::Bytes(b"appid3"),
    ///         Value::Bytes(b"alias3"),
    ///         Value::Bytes(b"c"),
    ///     ],
    /// ];
    /// let count = table.insert_multi_row_datas(columns, &dataset);
    pub fn insert_multi_row_datas(
        &self,
        columns: &Vec<&'static str>,
        dataset: &Vec<Vec<Value>>,
    ) -> Result<i32, SqliteErrCode> {
        let mut sql = format!("insert into {} (", self.table_name);
        build_sql_columns_not_empty(columns, &mut sql);
        sql.push_str(") values (");
        build_sql_values(columns.len(), &mut sql);
        sql.push(')');
        let stmt = prepare_statement(self, &mut sql)?;
        let mut count = 0;
        for datas in dataset {
            let ret = stmt.reset();
            if ret != SQLITE_OK {
                return Err(ret);
            }
            let mut index = 1;
            bind_data_values(datas, &stmt, &mut index)?;
            let ret = stmt.step();
            if ret != SQLITE_DONE {
                return Err(ret);
            }
            count += 1;
        }
        Ok(count)
    }

    /// rename table name
    pub fn rename(&mut self, name: &str) -> SqliteErrCode {
        let sql = format!("ALTER TABLE {} RENAME TO {}", self.table_name, name);
        asset_log::loge!("{}", sql);
        let stmt = &Statement::<false>::new(sql.as_str(), self.db);
        let ret = stmt.exec(None, 0);
        if ret == SQLITE_OK {
            self.table_name = name.to_string();
        }
        ret
    }

    /// add new column for table
    /// 1. can not add primary key
    /// 2. can not add not null key if no default value
    /// 3. only int/blob support default value, blob data default value is always null
    ///
    /// code like:
    /// let ret = table.add_new_column(
    ///     ColumnInfo {
    ///         name: "id",
    ///         data_type: DataType::INTEGER,
    ///         is_primary_key: false,
    ///         not_null: true,
    ///     },
    ///     Some(Value::Number(0)),
    /// );
    pub fn add_new_column(&self, column: ColumnInfo, default_value: Option<Value>) -> SqliteErrCode {
        if column.is_primary_key {
            return SQLITE_ERROR;
        }
        if column.not_null && default_value.is_none() {
            return SQLITE_ERROR;
        }
        let data_type = from_data_type_to_str(&column.data_type);
        let mut sql = format!("ALTER TABLE {} ADD COLUMN {} {}", self.table_name, column.name, data_type);
        if let Some(data) = default_value {
            sql.push_str(" DEFAULT ");
            sql.push_str(&from_data_value_to_str_value(&data));
        }
        if column.not_null {
            sql.push_str(" NOT NULL");
        }
        asset_log::loge!("{}", sql);
        let stmt = Statement::<false>::new(sql.as_str(), self.db);

        stmt.exec(None, 0)
    }

    /// query datas from table,
    /// if length of columns is 0, will select *.
    /// if length of conditions is 0, will select all data.
    ///
    /// code like:
    /// let result_set = table.query_row(&vec!["alias", "blobs"], &vec![]);
    ///
    /// means sql like: select alias,blobs from table_name
    pub fn query_row(
        &self,
        columns: &Vec<&str>,
        conditions: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<ResultSet, SqliteErrCode> {
        let mut sql = String::from("select distinct ");
        build_sql_columns(columns, &mut sql);
        sql.push_str(" from ");
        sql.push_str(self.table_name.as_str());
        build_sql_where(conditions, &mut sql);
        build_sql_query_options(query_options, &mut sql);
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_conditions(conditions, &stmt, &mut index)?;
        let mut result = vec![];
        while stmt.step() == SQLITE_ROW {
            let mut data_line = Vec::<Value>::new();
            let n = stmt.data_count();
            for i in 0..n {
                let data = stmt.query_columns_auto_type(i)?;
                if let Some(data) = data {
                    data_line.push(data);
                }
            }
            result.push(data_line);
        }
        Ok(result)
    }

    /// query datas from table,
    /// if length of columns is 0, will select *.
    /// if length of conditions is 0, will select all data.
    /// the return value will construct into HashMap
    ///
    /// code like:
    /// let result_set = table.query_datas_with_key_value(&vec!["alias", "blobs"], &vec![]);
    ///
    /// means sql like: select alias,blobs from table_name
    pub fn query_datas_advanced(
        &self,
        columns: &Vec<&'static str>,
        conditions: &Condition,
        query_options: Option<&QueryOptions>,
        column_info: &'static [ColumnInfo],
    ) -> Result<Vec<DbMap>, SqliteErrCode> {
        let mut sql = String::from("select distinct ");
        build_sql_columns(columns, &mut sql);
        sql.push_str(" from ");
        sql.push_str(self.table_name.as_str());
        build_sql_where(conditions, &mut sql);
        build_sql_query_options(query_options, &mut sql);
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_conditions(conditions, &stmt, &mut index)?;
        let mut result = vec![];
        while stmt.step() == SQLITE_ROW {
            let mut record = DbMap::new();
            let n = stmt.data_count();
            for i in 0..n {
                let column_name = stmt.query_column_name(i)?;
                let column_info = get_column_info(column_info, column_name);
                match stmt.query_columns_auto_type(i)? {
                    Some(Value::Number(n)) if column_info.data_type == DataType::Bool => {
                        record.insert(column_info.name, Value::Bool(n != 0))
                    },
                    Some(n) => record.insert(column_info.name, n),
                    None => continue,
                };
            }
            result.push(record);
        }
        Ok(result)
    }

    /// return the count of datas with conditions, the length of conditions may be 0.
    ///
    /// code like:
    /// let count = table
    ///     .count_datas(&DbMap::from([("id", Value::Number(3))]));
    ///
    /// the sql is like : select count(*) as count from table_name where id=3
    pub fn count_datas(&self, conditions: &Condition) -> Result<u32, SqliteErrCode> {
        let mut sql = format!("select distinct count(*) as count from {}", self.table_name);
        build_sql_where(conditions, &mut sql);
        let stmt = prepare_statement(self, &mut sql)?;
        let mut index = 1;
        bind_conditions(conditions, &stmt, &mut index)?;
        let ret = stmt.step();
        if ret != SQLITE_ROW {
            return Err(ret);
        }
        let count = stmt.query_column_int(0);
        Ok(count)
    }

    /// return if the data exists
    ///
    /// code like:
    /// let exits = table
    ///     .is_data_exists(&DbMap::from([("id", Value::Number(3)), ("alias", Value::Bytes(b"test test"))]));
    ///
    /// the sql is like: select count(*) as count from table_name where id=3 and alias='test test'
    /// if count > 0, data exists
    pub fn is_data_exists(&self, cond: &Condition) -> Result<bool, SqliteErrCode> {
        let ret = self.count_datas(cond);
        match ret {
            Ok(count) => Ok(count > 0),
            Err(e) => Err(e),
        }
    }
}
