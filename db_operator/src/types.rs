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
use crate::database::Database;

/// DataType for DB
#[derive(PartialEq, Eq, Clone, Copy)]
pub enum DataType {
    /// numbers
    INTEGER,
    /// string
    TEXT,
    /// binary data
    BLOB,
    /// float value
    DOUBLE,
}

/// change datatype to sql str
pub fn from_datatype_to_str(value: DataType) -> &'static str {
    match value {
        DataType::BLOB => "BLOB",
        DataType::DOUBLE => "REAL",
        DataType::INTEGER => "INTEGER",
        DataType::TEXT => "TEXT",
    }
}

/// Datavalue for DB
#[derive(Clone, Copy)]
#[repr(C)]
pub enum DataValue<'a> {
    /// numbers
    Integer(i32),
    /// float value
    Double(f64),
    /// string
    Text(&'a [u8]),
    /// binary data
    Blob(&'a [u8]),
    /// for null
    NoData,
}

/// change datavalue to str value when build sql
pub fn from_datavalue_to_str_value(value: DataValue) -> String {
    match value {
        DataValue::NoData => String::from("NULL"),
        DataValue::Double(d) => format!("{}", d),
        DataValue::Integer(i) => format!("{}", i),
        DataValue::Blob(_b) => String::from("NOT SUPPORTED"),
        DataValue::Text(t) => {
            let s = unsafe { String::from_utf8_unchecked(t.to_vec()) };
            format!("'{}'", s)
        },
    }
}

/// Result set data value for query sql
#[repr(C)]
pub enum ResultDataValue {
    /// numbers
    Integer(i32),
    /// float value
    Double(f64),
    /// string
    Text(Option<Box<Vec<u8>>>),
    /// binary data
    Blob(Option<Box<Vec<u8>>>),
    /// for null
    Null,
}

/// change result value to string when output
pub fn from_resultvalue_to_str_value(value: &ResultDataValue) -> String {
    match value {
        ResultDataValue::Null => String::from("NULL"),
        ResultDataValue::Integer(b) => format!("{}", b),
        ResultDataValue::Double(b) => format!("{}", b),
        ResultDataValue::Blob(b) => {
            if let Some(b) = b {
                let s = unsafe { String::from_utf8_unchecked(b.to_vec()) };
                format!("'{}'", s)
            } else {
                String::from("NULL")
            }
        },
        ResultDataValue::Text(b) => {
            if let Some(b) = b {
                let s = unsafe { String::from_utf8_unchecked(b.to_vec()) };
                format!("'{}'", s)
            } else {
                String::from("NULL")
            }
        },
    }
}

/// change result datatype to str when output
pub fn from_result_datatype_to_str(value: &ResultDataValue) -> &'static str {
    match value {
        ResultDataValue::Blob(_) => "BLOB",
        ResultDataValue::Double(_) => "REAL",
        ResultDataValue::Integer(_) => "INTEGER",
        ResultDataValue::Text(_) => "TEXT",
        ResultDataValue::Null => "NULL",
    }
}

/// Pair struct for query condition or exec data
#[derive(Clone, Copy)]
#[repr(C)]
pub struct Pair<'a> {
    /// column name for condition
    pub column_name: &'a str,
    /// query value for condition
    pub value: DataValue<'a>,
}

/// query conditons
pub type Condition<'a> = Vec<Pair<'a>>;

/// column info for create table
#[repr(C)]
pub struct ColumnInfo<'a> {
    /// column name
    pub name: &'a str,
    /// column datatype
    pub data_type: DataType,
    /// id auto inc for primary key
    pub is_primary_key: bool,
    /// this column should not be null
    pub not_null: bool,
}

/// result set
pub type ResultSet = Vec<Vec<ResultDataValue>>;

/// err msg for database after exec sql
#[repr(C)]
pub struct Sqlite3Errmsg<'a, 'b> {
    /// error string
    pub s: &'a str,
    /// point to database for auto drop
    pub db: &'b Database,
}
