/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

//! This module provides data operation management on database tables.
//! The managed data can be user input. Because we will prepare and bind data.

use core::ffi::c_void;
use std::cmp::Ordering;

use asset_definition::{log_throw_error, Conversion, DataType, ErrCode, Result, Value};
use asset_log::logi;

use crate::{
    database::Database,
    statement::Statement,
    transaction::Transaction,
    types::{ColumnInfo, DbMap, QueryOptions, UpgradeColumnInfo, DB_UPGRADE_VERSION, SQLITE_ROW},
};

extern "C" {
    fn SqliteChanges(db: *mut c_void) -> i32;
}

#[repr(C)]
pub(crate) struct Table<'a> {
    pub(crate) table_name: String,
    pub(crate) db: &'a Database,
}

#[inline(always)]
fn bind_datas(datas: &DbMap, stmt: &Statement, index: &mut i32) -> Result<()> {
    for (_, value) in datas.iter() {
        stmt.bind_data(*index, value)?;
        *index += 1;
    }
    Ok(())
}

fn bind_where_datas(datas: &DbMap, stmt: &Statement, index: &mut i32) -> Result<()> {
    for (key, value) in datas.iter() {
        if *key == "SyncType" {
            stmt.bind_data(*index, value)?;
            *index += 1;
        }
        stmt.bind_data(*index, value)?;
        *index += 1;
    }
    Ok(())
}

fn bind_where_with_specific_condifion(datas: &[Value], stmt: &Statement, index: &mut i32) -> Result<()> {
    for value in datas.iter() {
        stmt.bind_data(*index, value)?;
        *index += 1;
    }
    Ok(())
}

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

#[inline(always)]
fn build_sql_columns(columns: &Vec<&str>, sql: &mut String) {
    if !columns.is_empty() {
        build_sql_columns_not_empty(columns, sql);
    } else {
        sql.push('*');
    }
}

#[inline(always)]
fn build_sql_where(conditions: &DbMap, filter: bool, sql: &mut String) {
    if !conditions.is_empty() || filter {
        sql.push_str(" where ");
        if filter {
            sql.push_str("SyncStatus <> 2");
            if !conditions.is_empty() {
                sql.push_str(" and ");
            }
        }
        if !conditions.is_empty() {
            for (i, column_name) in conditions.keys().enumerate() {
                if *column_name == "SyncType" {
                    sql.push_str("(SyncType & ?) = ?");
                } else {
                    sql.push_str(column_name);
                    sql.push_str("=?");
                }
                if i != conditions.len() - 1 {
                    sql.push_str(" and ")
                }
            }
        }
    }
}

#[inline(always)]
fn build_sql_values(len: usize, sql: &mut String) {
    for i in 0..len {
        sql.push('?');
        if i != len - 1 {
            sql.push(',');
        }
    }
}

fn from_data_type_to_str(value: &DataType) -> &'static str {
    match *value {
        DataType::Bytes => "BLOB",
        DataType::Number => "INTEGER",
        DataType::Bool => "INTEGER",
    }
}

fn from_data_value_to_str_value(value: &Value) -> String {
    match *value {
        Value::Number(i) => format!("{}", i),
        Value::Bytes(_) => String::from("NOT SUPPORTED"),
        Value::Bool(b) => format!("{}", b),
    }
}

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
            if let Some(offset) = option.offset {
                sql.push_str(format!(" offset {}", offset).as_str());
            }
        } else if let Some(offset) = option.offset {
            sql.push_str(format!(" limit -1 offset {}", offset).as_str());
        }
    }
}

fn build_sql_reverse_condition(reverse_condition: Option<&DbMap>, sql: &mut String) {
    if let Some(conditions) = reverse_condition {
        if !conditions.is_empty() {
            sql.push_str(" and ");
            for (i, column_name) in conditions.keys().enumerate() {
                if *column_name == "SyncType" {
                    sql.push_str("(SyncType & ?) == 0");
                } else {
                    sql.push_str(column_name);
                    sql.push_str("<>?");
                }
                if i != conditions.len() - 1 {
                    sql.push_str(" and ")
                }
            }
        }
    }
}

fn get_column_info(columns: &'static [ColumnInfo], db_column: &str) -> Result<&'static ColumnInfo> {
    for column in columns.iter() {
        if column.name.eq(db_column) {
            return Ok(column);
        }
    }
    log_throw_error!(ErrCode::DataCorrupted, "Database is corrupted.")
}

impl<'a> Table<'a> {
    pub(crate) fn new(table_name: &str, db: &'a Database) -> Table<'a> {
        Table { table_name: table_name.to_string(), db }
    }

    pub(crate) fn exist(&self) -> Result<bool> {
        let sql = format!("select * from sqlite_master where type ='table' and name = '{}'", self.table_name);
        let stmt = Statement::prepare(sql.as_str(), self.db)?;
        let ret = stmt.step()?;
        if ret == SQLITE_ROW {
            Ok(true)
        } else {
            Ok(false)
        }
    }

    #[allow(dead_code)]
    pub(crate) fn delete(&self) -> Result<()> {
        let sql = format!("DROP TABLE {}", self.table_name);
        self.db.exec(&sql)
    }

    /// Create a table with name 'table_name' at specific version.
    /// The columns is descriptions for each column.
    pub(crate) fn create_with_version(&self, columns: &[ColumnInfo], version: u32) -> Result<()> {
        let is_exist = self.exist()?;
        if is_exist {
            return Ok(());
        }
        let mut sql = format!("CREATE TABLE IF NOT EXISTS {}(", self.table_name);
        for i in 0..columns.len() {
            let column = &columns[i];
            sql.push_str(column.name);
            sql.push(' ');
            sql.push_str(from_data_type_to_str(&column.data_type));
            if column.is_primary_key {
                sql.push_str(" PRIMARY KEY");
            }
            if column.not_null {
                sql.push_str(" NOT NULL");
            }
            if i != columns.len() - 1 {
                sql.push(',')
            };
        }
        sql.push_str(");");
        let mut trans = Transaction::new(self.db);
        trans.begin()?;
        if self.db.exec(sql.as_str()).is_ok() && self.db.set_version(version).is_ok() {
            trans.commit()
        } else {
            trans.rollback()
        }
    }

    /// Create a table with name 'table_name'.
    /// The columns is descriptions for each column.
    pub(crate) fn create(&self, columns: &[ColumnInfo]) -> Result<()> {
        self.create_with_version(columns, DB_UPGRADE_VERSION)
    }

    pub(crate) fn upgrade(&self, ver: u32, columns: &[UpgradeColumnInfo]) -> Result<()> {
        let is_exist = self.exist()?;
        if !is_exist {
            return Ok(());
        }
        logi!("upgrade table!");
        let mut trans = Transaction::new(self.db);
        trans.begin()?;
        for item in columns {
            if self.add_column(&item.base_info, &item.default_value).is_err() {
                return trans.rollback();
            }
        }
        if self.db.set_version(ver).is_err() {
            trans.rollback()
        } else {
            trans.commit()
        }
    }

    /// Insert a row into table, and datas is the value to be insert.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: insert into table_name(id,alias) values (3,'alias1')
    /// let datas = &DbMap::from([("id", Value::Number(3), ("alias", Value::Bytes(b"alias1"))]);
    /// let ret = table.insert_row(datas);
    /// ```
    pub(crate) fn insert_row(&self, datas: &DbMap) -> Result<i32> {
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
        let stmt = Statement::prepare(&sql, self.db)?;
        let mut index = 1;
        bind_datas(datas, &stmt, &mut index)?;
        stmt.step()?;
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// Delete row from table.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: delete from table_name where id=2
    /// let condition = &DbMap::from([("id", Value::Number(2)]);
    /// let ret = table.delete_row(condition, None, false);
    /// ```
    pub(crate) fn delete_row(
        &self,
        condition: &DbMap,
        reverse_condition: Option<&DbMap>,
        is_filter_sync: bool,
    ) -> Result<i32> {
        let mut sql = format!("delete from {}", self.table_name);
        build_sql_where(condition, is_filter_sync, &mut sql);
        build_sql_reverse_condition(reverse_condition, &mut sql);
        let stmt = Statement::prepare(&sql, self.db)?;
        let mut index = 1;
        bind_where_datas(condition, &stmt, &mut index)?;
        if let Some(datas) = reverse_condition {
            bind_datas(datas, &stmt, &mut index)?;
        }
        stmt.step()?;
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// Delete row from table with specific condition.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: delete from table_name where id=2
    /// let specific_cond = "id".to_string();
    /// let condition_value = Value::Number(2);
    /// let ret = table.delete_with_specific_cond(specific_cond, condition_value);
    /// ```
    pub(crate) fn delete_with_specific_cond(&self, specific_cond: &str, condition_value: &[Value]) -> Result<i32> {
        let sql: String = format!("delete from {} where {}", self.table_name, specific_cond);
        let stmt = Statement::prepare(&sql, self.db)?;
        let mut index = 1;
        bind_where_with_specific_condifion(condition_value, &stmt, &mut index)?;
        stmt.step()?;
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// Update a row in table.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: update table_name set alias='update_value' where id=2
    /// let condition = &DbMap::from([("id", Value::Number(2)]);
    /// let datas = &DbMap::from([("alias", Value::Bytes(b"update_value")]);
    /// let ret = table.update_row(conditions, false, datas);
    /// ```
    pub(crate) fn update_row(&self, condition: &DbMap, is_filter_sync: bool, datas: &DbMap) -> Result<i32> {
        let mut sql = format!("update {} set ", self.table_name);
        for (i, column_name) in datas.keys().enumerate() {
            sql.push_str(column_name);
            sql.push_str("=?");
            if i != datas.len() - 1 {
                sql.push(',');
            }
        }
        build_sql_where(condition, is_filter_sync, &mut sql);
        let stmt = Statement::prepare(&sql, self.db)?;
        let mut index = 1;
        bind_datas(datas, &stmt, &mut index)?;
        bind_where_datas(condition, &stmt, &mut index)?;
        stmt.step()?;
        let count = unsafe { SqliteChanges(self.db.handle as _) };
        Ok(count)
    }

    /// Query row from table.
    /// If length of columns is 0, all table columns are queried. (eg. select * xxx)
    /// If length of condition is 0, all data in the table is queried.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: select alias,blobs from table_name
    /// let result_set = table.query_datas_with_key_value(&vec!["alias", "blobs"], false, &vec![]);
    /// ```
    pub(crate) fn query_row(
        &self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
        is_filter_sync: bool,
        column_info: &'static [ColumnInfo],
    ) -> Result<Vec<DbMap>> {
        let mut sql = String::from("select ");
        if !columns.is_empty() {
            sql.push_str("distinct ");
        }
        build_sql_columns(columns, &mut sql);
        sql.push_str(" from ");
        sql.push_str(self.table_name.as_str());
        build_sql_where(condition, is_filter_sync, &mut sql);
        build_sql_query_options(query_options, &mut sql);
        let stmt = Statement::prepare(&sql, self.db)?;
        let mut index = 1;
        bind_where_datas(condition, &stmt, &mut index)?;
        let mut result = vec![];
        while stmt.step()? == SQLITE_ROW {
            let mut record = DbMap::new();
            let n = stmt.data_count();
            for i in 0..n {
                let column_name = stmt.query_column_name(i)?;
                let column_info = get_column_info(column_info, column_name)?;
                match stmt.query_column_auto_type(i)? {
                    Some(Value::Number(n)) if column_info.data_type == DataType::Bool => {
                        record.insert(column_info.name, Value::Bool(n != 0))
                    },
                    Some(n) if n.data_type() == column_info.data_type => record.insert(column_info.name, n),
                    Some(_) => {
                        return log_throw_error!(ErrCode::DataCorrupted, "The data in DB has been tampered with.")
                    },
                    None => continue,
                };
            }
            result.push(record);
        }
        Ok(result)
    }

    /// Count the number of datas with query condition(can be empty).
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: select count(*) as count from table_name where id=3
    /// let count = table.count_datas(&DbMap::from([("id", Value::Number(3))]), false);
    /// ```
    pub(crate) fn count_datas(&self, condition: &DbMap, is_filter_sync: bool) -> Result<u32> {
        let mut sql = format!("select count(*) as count from {}", self.table_name);
        build_sql_where(condition, is_filter_sync, &mut sql);
        let stmt = Statement::prepare(&sql, self.db)?;
        let mut index = 1;
        bind_where_datas(condition, &stmt, &mut index)?;
        stmt.step()?;
        let count = stmt.query_column_int(0);
        Ok(count)
    }

    /// Check whether data exists in the database table.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: select count(*) as count from table_name where id=3 and alias='alias'
    /// let exits = table
    ///     .is_data_exists(&DbMap::from([("id", Value::Number(3)), ("alias", Value::Bytes(b"alias"))]), false);
    /// ```
    pub(crate) fn is_data_exists(&self, cond: &DbMap, is_filter_sync: bool) -> Result<bool> {
        let ret = self.count_datas(cond, is_filter_sync);
        match ret {
            Ok(count) => Ok(count > 0),
            Err(e) => Err(e),
        }
    }

    /// Add new column tp table.
    /// 1. Primary key cannot be added.
    /// 2. Cannot add a non-null column with no default value
    /// 3. Only the integer and blob types support the default value, and the default value of the blob type is null.
    ///
    /// # Examples
    ///
    /// ```
    /// // SQL: alter table table_name add cloumn id integer not null
    /// let ret = table.add_column(
    ///     ColumnInfo {
    ///         name: "id",
    ///         data_type: DataType::INTEGER,
    ///         is_primary_key: false,
    ///         not_null: true,
    ///     },
    ///     Some(Value::Number(0)),
    /// );
    /// ```
    pub(crate) fn add_column(&self, column: &ColumnInfo, default_value: &Option<Value>) -> Result<()> {
        if column.is_primary_key {
            return log_throw_error!(ErrCode::InvalidArgument, "The primary key already exists in the table.");
        }
        if column.not_null && default_value.is_none() {
            return log_throw_error!(ErrCode::InvalidArgument, "A default value is required for a non-null column.");
        }
        let data_type = from_data_type_to_str(&column.data_type);
        let mut sql = format!("ALTER TABLE {} ADD COLUMN {} {}", self.table_name, column.name, data_type);
        if let Some(data) = default_value {
            sql.push_str(" DEFAULT ");
            sql.push_str(&from_data_value_to_str_value(data));
        }
        if column.not_null {
            sql.push_str(" NOT NULL");
        }
        self.db.exec(sql.as_str())
    }

    pub(crate) fn replace_row(&self, condition: &DbMap, is_filter_sync: bool, datas: &DbMap) -> Result<()> {
        let mut trans = Transaction::new(self.db);
        trans.begin()?;
        if self.delete_row(condition, None, is_filter_sync).is_ok() && self.insert_row(datas).is_ok() {
            trans.commit()
        } else {
            trans.rollback()
        }
    }
}
