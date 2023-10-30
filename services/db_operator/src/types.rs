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
pub(crate) fn from_data_type_to_str(value: &DataType) -> &'static str {
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

/// Table name of asset database.
pub const ASSET_TABLE_NAME: &str = "asset_table";
/// Latest data version number.
pub const DB_DATA_VERSION: u32 = 1;
/// Column name of asset database.
pub mod column {
    /// default column name Id
    pub const ID: &str = "Id";
    /// default column name Secret
    pub const SECRET: &str = "Secret";
    /// default column name Alias
    pub const ALIAS: &str = "Alias";
    /// default column name Owner
    pub const OWNER: &str = "Owner";
    /// default column name OwnerType
    pub const OWNER_TYPE: &str = "OwnerType";
    /// default column name GroupId
    pub const GROUP_ID: &str = "GroupId";
    /// default column name SyncType
    pub const SYNC_TYPE: &str = "SyncType";
    /// default column name Accessibility
    pub const ACCESSIBILITY: &str = "Accessibility";
    /// default column name AuthType
    pub const AUTH_TYPE: &str = "AuthType";
    /// default column name CreateTime
    pub const CREATE_TIME: &str = "CreateTime";
    /// default column name UpdateTime
    pub const UPDATE_TIME: &str = "UpdateTime";
    /// default column name DeleteType
    pub const DELETE_TYPE: &str = "DeleteType";
    /// default column name Version
    pub const VERSION: &str = "Version";
    /// default column name RequirePasswordSet
    pub const REQUIRE_PASSWORD_SET: &str = "RequirePasswordSet";
    /// default column name DataLabelCritical_1
    pub const CRITICAL1: &str = "DataLabelCritical_1";
    /// default column name DataLabelCritical_2
    pub const CRITICAL2: &str = "DataLabelCritical_2";
    /// default column name DataLabelCritical_3
    pub const CRITICAL3: &str = "DataLabelCritical_3";
    /// default column name DataLabelCritical_4
    pub const CRITICAL4: &str = "DataLabelCritical_4";
    /// default column name DataLabelNormal_1
    pub const NORMAL1: &str = "DataLabelNormal_1";
    /// default column name DataLabelNormal_2
    pub const NORMAL2: &str = "DataLabelNormal_2";
    /// default column name DataLabelNormal_3
    pub const NORMAL3: &str = "DataLabelNormal_3";
    /// default column name DataLabelNormal_4
    pub const NORMAL4: &str = "DataLabelNormal_4";
}

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

/// columns info for default asset_table
pub(crate) const COLUMN_INFO: &[ColumnInfo] = &[
    ColumnInfo { name: column::ID, data_type: DataType::Number, is_primary_key: true, not_null: true },
    ColumnInfo { name: column::SECRET, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::ALIAS, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::OWNER, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::OWNER_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::GROUP_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::SYNC_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::ACCESSIBILITY, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::AUTH_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::CREATE_TIME, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::UPDATE_TIME, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::DELETE_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::VERSION, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::REQUIRE_PASSWORD_SET, data_type: DataType::Bool, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::CRITICAL1, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CRITICAL2, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CRITICAL3, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CRITICAL4, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL1, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL2, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL3, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL4, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
];

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
pub const SQLITE_ERROR: i32 = 1;
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
