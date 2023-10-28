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

//! struct types for db

use core::ffi::c_void;
use std::{cmp::Ordering, collections::HashMap};

use asset_definition::{DataType, Value};

use crate::database::Database;

/// change data type to sql str
pub fn from_data_type_to_str(value: &DataType) -> &'static str {
    match *value {
        DataType::Bytes => "BLOB",
        DataType::Number => "INTEGER",
        DataType::Bool => "INTEGER",
    }
}

/// change data value to str value when build sql
pub fn from_data_value_to_str_value(value: &Value) -> String {
    match *value {
        Value::Number(i) => format!("{}", i),
        Value::Bytes(_) => String::from("NOT SUPPORTED"),
        Value::Bool(b) => format!("{}", b),
    }
}

/// A Map type containing tag-value pairs that describe the attributes of an DB field.
pub type DbMap = HashMap<&'static str, Value>;

/// query conditions
pub type Condition = DbMap;

/// column info for create table
#[repr(C)]
pub struct ColumnInfo {
    /// column name
    pub name: &'static str,
    /// column data type
    pub data_type: DataType,
    /// id auto inc for primary key
    pub is_primary_key: bool,
    /// this column should not be null
    pub not_null: bool,
}

/// result set
pub type ResultSet = Vec<Vec<Value>>;

/// err msg for database after exec sql
#[repr(C)]
pub struct Sqlite3ErrMsg<'a, 'b> {
    /// error string
    pub s: &'a str,
    /// point to database for auto drop
    pub db: &'b Database<'b>,
}

/// query options
#[repr(C)]
pub struct QueryOptions {
    /// offset param
    pub offset: Option<u32>,
    /// limit param
    pub limit: Option<u32>,
    /// order param
    /// Ordering::Greater => ASC
    /// Ordering::Less => DESC
    pub order: Option<Ordering>,
    /// order by columns
    pub order_by: Option<Vec<&'static str>>,
}

/// sqlite error type
pub type SqliteErrCode = i32;
/// Successful result
pub const SQLITE_OK: i32 = 0;
/// Generic error
pub const DATABASE_ERROR: i32 = 1;
/// sqlite3_step() has another row ready
pub const SQLITE_ROW: i32 = 100;
/// sqlite3_step() has finished executing
pub const SQLITE_DONE: i32 = 101;
/// data: pointer passed by sqlite3_exec
/// argc: count of ResultSet
/// argv: Result
/// az_col_name: Column names
pub(crate) type Sqlite3Callback =
    extern "C" fn(data: *mut c_void, argc: i32, argv: *const *const u8, az_col_name: *const *const u8) -> SqliteErrCode;
