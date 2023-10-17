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

//! yuanhao: 补充DOC

use std::ffi::CStr;
use std::fmt::Write;

use asset_common::{definition::{DataType, Value}, loge};

use crate::{
    database::Database, sqlite3_bind_blob_func, sqlite3_bind_int64_func, sqlite3_column_blob_func,
    sqlite3_column_bytes_func, sqlite3_column_count_func, sqlite3_column_double_func,
    sqlite3_column_int64_func, sqlite3_column_name_func, sqlite3_column_text_func,
    sqlite3_column_type_func, sqlite3_data_count_func, sqlite3_finalize_func,
    sqlite3_prepare_v2_func, sqlite3_reset_func, sqlite3_step_func, Sqlite3Callback, SqliteErrCode,
    SQLITE_BLOB, SQLITE_ERROR, SQLITE_INTEGER, SQLITE_NULL, SQLITE_OK,
};

/// sql statement
#[repr(C)]
pub struct Statement<'b, const PREPARE: bool> {
    /// sql string
    pub(crate) sql: String,
    /// point to db
    db: &'b Database<'b>,
    /// raw pointer
    handle: usize,
}

impl<'b> Statement<'b, false> {
    /// create a statement without prepare
    pub fn new(sql: &str, db: &'b Database) -> Statement<'b, false> {
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        Statement { sql: sql_s, db, handle: 0 }
    }

    /// execute sql without prepare
    /// the callback is to process the result set
    /// the data will be passed into callback function
    pub fn exec(&self, callback: Option<Sqlite3Callback>, data: usize) -> SqliteErrCode {
        self.db.exec(self, callback, data)
    }
}

impl<'b> Statement<'b, true> {
    /// wrap for sqlite3_step,
    /// if step succ, will return SQLITE_DONE for update,insert,delete or SQLITE_ROW for select
    pub fn step(&self) -> SqliteErrCode {
        sqlite3_step_func(self.handle)
    }

    /// prepare a sql, you can use '?' for datas and bind datas later
    pub fn prepare(sql: &str, db: &'b Database) -> Result<Statement<'b, true>, SqliteErrCode> {
        let mut tail = 0usize;
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        let mut stmt = Statement { sql: sql_s, db, handle: 0 };
        let ret = sqlite3_prepare_v2_func(db.handle, &stmt.sql, -1, &mut stmt.handle, &mut tail);
        if ret == 0 {
            Ok(stmt)
        } else {
            Err(ret)
        }
    }

    fn print_vec(i: i32, v: &[u8]) {
        // todo: delete
        let mut s = String::new();
        for byte in v {
            write!(s, "{:02x}", byte).expect("Unable to write to string");
        }
        loge!("[YZT] index = {}, bind vec = {}", i, s);
    }

    /// bind datas
    /// data_type is detected by enum Value,
    /// index is start with 1, for '?' in sql.
    pub fn bind_data(&self, index: i32, data: &Value) -> SqliteErrCode {
        match data {
            Value::Bytes(b) => {
                Self::print_vec(index, b);
                sqlite3_bind_blob_func(self.handle, index, b, b.len() as _, None)
            },
            Value::Number(i) => {
                loge!("[YZT] index = {}, bind integer = {}", index, i);
                sqlite3_bind_int64_func(self.handle, index, *i as _)
            },
            Value::Bool(b) => {
                loge!("[YZT] index = {}, bind bool = {}", index, b);
                sqlite3_bind_int64_func(self.handle, index, *b as _)
            },
        }
    }

    /// you should reset statement before bind data for insert statement
    pub fn reset(&self) -> SqliteErrCode {
        sqlite3_reset_func(self.handle)
    }

    /// get column count for select statement
    pub fn column_count(&self) -> i32 {
        sqlite3_column_count_func(self.handle)
    }

    /// return the column name
    pub fn query_column_name(&self, n: i32) -> Result<&str, SqliteErrCode> {
        let s = sqlite3_column_name_func(self.handle, n);
        if !s.is_null() {
            let name = unsafe { CStr::from_ptr(s as _) };
            return Ok(name.to_str().unwrap());
        }
        Err(SQLITE_ERROR)
    }

    /// data count
    pub fn data_count(&self) -> i32 {
        sqlite3_data_count_func(self.handle)
    }

    /// query column datas in result set
    /// data_type is auto detected by Value
    /// the index if start with 0
    pub fn query_column(&self, index: i32, out: &DataType) -> Option<Value> {
        match out {
            DataType::Bytes => {
                let blob = self.query_column_blob(index);
                if blob.is_empty() {
                    None
                } else {
                    Some(Value::Bytes(blob.to_vec()))
                }
            },
            DataType::Number => Some(Value::Number(self.query_column_int(index))),
            DataType::Bool => Some(Value::Bool(self.query_column_int(index) != 0))
        }
    }

    /// query columns auto type
    pub fn query_columns_auto_type(&self, i: i32) -> Result<Option<Value>, SqliteErrCode> {
        let tp = self.column_type(i);
        let data = match tp {
            SQLITE_INTEGER => Some(Value::Number(self.query_column_int(i))),
            SQLITE_BLOB => {
                let blob = self.query_column_blob(i);
                if blob.is_empty() {
                    None
                } else {
                    Some(Value::Bytes(blob.to_vec()))
                }
            },
            SQLITE_NULL => None,
            _ => return Err(SQLITE_ERROR),
        };
        Ok(data)
    }

    /// query column datas in result set for blob data
    /// the index is start with 0
    pub fn query_column_blob(&self, index: i32) -> &'b [u8] {
        let blob = sqlite3_column_blob_func(self.handle, index);
        let len = self.column_bytes(index);
        unsafe { core::slice::from_raw_parts(blob, len as _) }
    }

    /// query column datas in result set for double data
    /// the index is start with 0
    pub fn query_column_double(&self, index: i32) -> f64 {
        sqlite3_column_double_func(self.handle, index)
    }

    /// query column datas in result set for int data
    /// the index is start with 0
    pub fn query_column_int(&self, index: i32) -> u32 {
        sqlite3_column_int64_func(self.handle, index) as u32
    }

    /// query column datas in result set for text data
    /// the index is start with 0
    pub fn query_column_text(&self, index: i32) -> &'b [u8] {
        let text = sqlite3_column_text_func(self.handle, index);
        let len = self.column_bytes(index);
        unsafe { core::slice::from_raw_parts(text, len as _) }
    }

    /// return the bytes of data, you should first call query_column_text or query_column_blob,
    /// then call column_bytes.
    pub fn column_bytes(&self, index: i32) -> i32 {
        sqlite3_column_bytes_func(self.handle, index)
    }

    /// return column data_type
    pub fn column_type(&self, index: i32) -> i32 {
        sqlite3_column_type_func(self.handle, index)
    }
}

impl<'b, const PREPARE: bool> Drop for Statement<'b, PREPARE> {
    fn drop(&mut self) {
        if !PREPARE {
            return;
        }
        if self.handle != 0 {
            let ret = sqlite3_finalize_func(self.handle);
            if ret != SQLITE_OK {
                asset_common::loge!("sqlite3 finalize fail ret {}", ret);
            }
        }
    }
}
