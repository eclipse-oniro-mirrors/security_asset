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
use std::ffi::CStr;

use crate::{
    database::Database,
    sqlite3_bind_blob_func, sqlite3_bind_double_func, sqlite3_bind_int64_func,
    sqlite3_bind_null_func, sqlite3_bind_text_func, sqlite3_column_blob_func,
    sqlite3_column_bytes_func, sqlite3_column_count_func, sqlite3_column_double_func,
    sqlite3_column_int64_func, sqlite3_column_name_func, sqlite3_column_text_func,
    sqlite3_column_type_func, sqlite3_data_count_func, sqlite3_finalize_func,
    sqlite3_prepare_v2_func, sqlite3_reset_func, sqlite3_step_func,
    types::{DataValue, ResultDataValue},
    Sqlite3Callback, SqliteErrCode, SQLITE_ERROR, SQLITE_OK,
};

/// sql statement
#[repr(C)]
pub struct Statement<'b, const PREPARE: bool> {
    /// sql string
    pub sql: String,
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

    /// bind datas
    /// datatype is detected by enum DataValue,
    /// index is start with 1, for '?' in sql.
    pub fn bind_data(&self, index: i32, data: &DataValue) -> SqliteErrCode {
        match data {
            DataValue::Blob(b) => sqlite3_bind_blob_func(self.handle, index, b, b.len() as _, None),
            DataValue::Integer(i) => sqlite3_bind_int64_func(self.handle, index, *i as _),
            DataValue::Double(d) => sqlite3_bind_double_func(self.handle, index, *d),
            DataValue::Text(t) => sqlite3_bind_text_func(self.handle, index, t, t.len() as _, None),
            DataValue::NoData => sqlite3_bind_null_func(self.handle, index),
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
    /// datatype is auto detected by ResultDataValue
    /// the index if start with 0
    pub fn query_column(&self, index: i32, out: &mut ResultDataValue) {
        match out {
            ResultDataValue::Null => {},
            ResultDataValue::Blob(_) => {
                let blob = self.query_column_blob(index);
                if blob.is_empty() {
                    *out = ResultDataValue::Blob(None);
                } else {
                    let mut out_blob = Box::new(vec![0u8; blob.len()]);
                    out_blob.copy_from_slice(blob);
                    *out = ResultDataValue::Blob(Some(out_blob));
                }
            },
            ResultDataValue::Text(_) => {
                let text = self.query_column_text(index);
                if text.is_empty() {
                    *out = ResultDataValue::Text(None);
                } else {
                    let mut out_text = Box::new(vec![0u8; text.len()]);
                    out_text.copy_from_slice(text);
                    *out = ResultDataValue::Text(Some(out_text));
                }
            },
            ResultDataValue::Integer(i) => {
                *i = self.query_column_int(index);
            },
            ResultDataValue::Double(d) => {
                *d = self.query_column_double(index);
            },
        }
    }

    /// query column datas in result set for blob data
    /// the index if start with 0
    pub fn query_column_blob(&self, index: i32) -> &'b [u8] {
        let blob = sqlite3_column_blob_func(self.handle, index);
        let len = self.column_bytes(index);
        unsafe { core::slice::from_raw_parts(blob, len as _) }
    }

    /// query column datas in result set for double data
    /// the index if start with 0
    pub fn query_column_double(&self, index: i32) -> f64 {
        sqlite3_column_double_func(self.handle, index)
    }

    /// query column datas in result set for int data
    /// the index if start with 0
    pub fn query_column_int(&self, index: i32) -> u32 {
        sqlite3_column_int64_func(self.handle, index) as u32
    }

    /// query column datas in result set for text data
    /// the index if start with 0
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

    /// return column datatype
    pub fn column_type(&self, index: i32) -> i32 {
        sqlite3_column_type_func(self.handle, index)
    }
}

impl<'b, const PREPARE: bool> Drop for Statement<'b, PREPARE> {
    fn drop(&mut self) {
        if !PREPARE {
            return;
        }
        let ret = sqlite3_finalize_func(self.handle);
        if ret != SQLITE_OK {
            println!("sqlite3 finalize fail ret {}", ret);
        }
    }
}
