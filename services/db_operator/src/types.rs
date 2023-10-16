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

use asset_common::{definition::{Value, DataType}, loge};

/// change datatype to sql str
pub fn from_datatype_to_str(value: &DataType) -> &'static str {
    match *value {
        DataType::Bytes => "BLOB",
        DataType::Uint32 => "INTEGER",
        DataType::Bool => {
            loge!("Unexpected bool type.");
            panic!()
        },
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

/// Pair struct for query condition or exec data
#[derive(Clone)]
#[repr(C)]
pub struct Pair {
    /// column name for condition
    pub column_name: &'static str,
    /// query value for condition
    pub value: Value,
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
pub type ResultSet = Vec<Vec<Value>>;

/// advanced result set
pub type AdvancedResultSet = Vec<HashMap<String, Value>>;

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
