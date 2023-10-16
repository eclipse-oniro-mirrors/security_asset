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
use std::{cmp::Ordering, collections::HashMap};

use crate::database::Database;

/// DataType for DB
#[derive(PartialEq, Eq, Clone, Copy)]
pub enum DataType {
    /// numbers
    INTEGER,
    /// binary data
    BLOB,
}

/// change datatype to sql str
pub fn from_datatype_to_str(value: DataType) -> &'static str {
    match value {
        DataType::BLOB => "BLOB",
        DataType::INTEGER => "INTEGER",
    }
}

/// Data value for DB
#[derive(Clone)]
#[repr(C)]
pub enum DataValue {
    /// numbers
    Integer(u32),
    /// binary data
    Blob(Vec<u8>),
    /// for null
    NoData,
}

/// change data value to str value when build sql
pub fn from_data_value_to_str_value(value: DataValue) -> String {
    match value {
        DataValue::NoData => String::from("NULL"),
        DataValue::Integer(i) => format!("{}", i),
        DataValue::Blob(_b) => String::from("NOT SUPPORTED"),
    }
}

/// Result set data value for query sql
#[repr(C)]
pub enum ResultDataValue {
    /// numbers
    Integer(u32),
    /// binary data
    Blob(Box<Vec<u8>>),
    /// for null
    Null,
}

/// change result value to string when output
pub fn from_result_value_to_str_value(value: &ResultDataValue) -> String {
    match value {
        ResultDataValue::Null => String::from("NULL"),
        ResultDataValue::Integer(b) => format!("{}", b),
        ResultDataValue::Blob(b) => {
            let s = unsafe { String::from_utf8_unchecked(b.to_vec()) };
            format!("'{}'", s)
        },
    }
}

/// change result datatype to str when output
pub fn from_result_datatype_to_str(value: &ResultDataValue) -> &'static str {
    match value {
        ResultDataValue::Blob(_) => "BLOB",
        ResultDataValue::Integer(_) => "INTEGER",
        ResultDataValue::Null => "NULL",
    }
}

/// Pair struct for query condition or exec data
#[derive(Clone)]
#[repr(C)]
pub struct Pair {
    /// column name for condition
    pub column_name: &'static str,
    /// query value for condition
    pub value: DataValue,
}

/// query conditions
pub type Condition = Vec<Pair>;

/// column info for create table
#[repr(C)]
pub struct ColumnInfo {
    /// column name
    pub name: &'static str,
    /// column datatype
    pub data_type: DataType,
    /// id auto inc for primary key
    pub is_primary_key: bool,
    /// this column should not be null
    pub not_null: bool,
}

/// result set
pub type ResultSet = Vec<Vec<ResultDataValue>>;

/// advanced result set
pub type AdvancedResultSet = Vec<HashMap<String, ResultDataValue>>;

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
