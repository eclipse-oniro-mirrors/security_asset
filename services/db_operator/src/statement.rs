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

//! sqlite statement impl, support two types of statement: exec and prepare
//! statement is auto drop by RAII

use core::ffi::c_void;
use std::ffi::CStr;

use asset_definition::{DataType, Value};
use asset_log::loge;

use crate::{
    database::Database,
    types::{Sqlite3Callback, SqliteErrCode, DATABASE_ERROR, SQLITE_OK},
};

type BindCallback = extern "C" fn(p: *mut c_void);
extern "C" {
    fn SqliteFinalize(stmt: *mut c_void) -> i32;
    fn SqlitePrepareV2(
        db: *mut c_void,
        z_sql: *const u8,
        n_byte: i32,
        pp_stmt: *mut *mut c_void,
        pz_tail: *mut *mut u8,
    ) -> i32;
    fn SqliteBindBlob(stmt: *mut c_void, index: i32, blob: *const u8, n: i32, callback: Option<BindCallback>) -> i32;
    fn SqliteBindInt64(stmt: *mut c_void, index: i32, value: i64) -> i32;
    fn SqliteStep(stmt: *mut c_void) -> i32;
    fn SqliteColumnCount(stmt: *mut c_void) -> i32;
    fn SqliteColumnName(stmt: *mut c_void, n: i32) -> *const u8;
    fn SqliteDataCount(stmt: *mut c_void) -> i32;
    fn SqliteColumnBlob(stmt: *mut c_void, i_col: i32) -> *const u8;
    fn SqliteColumnInt64(stmt: *mut c_void, i_col: i32) -> i64;
    fn SqliteColumnText(stmt: *mut c_void, i_col: i32) -> *const u8;
    fn SqliteColumnBytes(stmt: *mut c_void, i_col: i32) -> i32;
    fn SqliteColumnType(stmt: *mut c_void, i_col: i32) -> i32;
    fn SqliteReset(stmt: *mut c_void) -> i32;
}

const SQLITE_INTEGER: i32 = 1;
const SQLITE_BLOB: i32 = 4;
const SQLITE_NULL: i32 = 5;

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
        unsafe { SqliteStep(self.handle as _) }
    }

    /// prepare a sql, you can use '?' for datas and bind datas later
    pub fn prepare(sql: &str, db: &'b Database) -> Result<Statement<'b, true>, SqliteErrCode> {
        let mut tail = 0usize;
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        let mut stmt = Statement { sql: sql_s, db, handle: 0 };
        let ret = unsafe {
            SqlitePrepareV2(
                db.handle as _,
                stmt.sql.as_ptr(),
                -1,
                &mut stmt.handle as *mut usize as _,
                &mut tail as *mut usize as _,
            )
        };
        if ret == 0 {
            Ok(stmt)
        } else {
            Err(ret)
        }
    }

    /// bind datas
    /// data_type is detected by enum Value,
    /// index is start with 1, for '?' in sql.
    pub fn bind_data(&self, index: i32, data: &Value) -> SqliteErrCode {
        match data {
            Value::Bytes(b) => unsafe { SqliteBindBlob(self.handle as _, index, b.as_ptr(), b.len() as _, None) },
            Value::Number(i) => unsafe { SqliteBindInt64(self.handle as _, index, *i as _) },
            Value::Bool(b) => unsafe { SqliteBindInt64(self.handle as _, index, *b as _) },
        }
    }

    /// you should reset statement before bind data for insert statement
    pub fn reset(&self) -> SqliteErrCode {
        unsafe { SqliteReset(self.handle as _) }
    }

    /// get column count for select statement
    pub fn column_count(&self) -> i32 {
        unsafe { SqliteColumnCount(self.handle as _) }
    }

    /// return the column name
    pub fn query_column_name(&self, n: i32) -> Result<&str, SqliteErrCode> {
        let s = unsafe { SqliteColumnName(self.handle as _, n) };
        if !s.is_null() {
            let name = unsafe { CStr::from_ptr(s as _) };
            if let Ok(rn) = name.to_str() {
                return Ok(rn);
            } else {
                loge!("asset column name error");
                return Err(DATABASE_ERROR);
            }
        }
        Err(DATABASE_ERROR)
    }

    /// data count
    pub fn data_count(&self) -> i32 {
        unsafe { SqliteDataCount(self.handle as _) }
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
            DataType::Bool => Some(Value::Bool(self.query_column_int(index) != 0)),
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
            _ => return Err(DATABASE_ERROR),
        };
        Ok(data)
    }

    /// query column datas in result set for blob data
    /// the index is start with 0
    pub fn query_column_blob(&self, index: i32) -> &'b [u8] {
        let blob = unsafe { SqliteColumnBlob(self.handle as _, index) };
        let len = self.column_bytes(index);
        unsafe { core::slice::from_raw_parts(blob, len as _) }
    }

    /// query column datas in result set for int data
    /// the index is start with 0
    pub fn query_column_int(&self, index: i32) -> u32 {
        unsafe { SqliteColumnInt64(self.handle as _, index) as u32 }
    }

    /// query column datas in result set for text data
    /// the index is start with 0
    pub fn query_column_text(&self, index: i32) -> &'b [u8] {
        let text = unsafe { SqliteColumnText(self.handle as _, index) };
        let len = self.column_bytes(index);
        unsafe { core::slice::from_raw_parts(text, len as _) }
    }

    /// return the bytes of data, you should first call query_column_text or query_column_blob,
    /// then call column_bytes.
    pub fn column_bytes(&self, index: i32) -> i32 {
        unsafe { SqliteColumnBytes(self.handle as _, index) }
    }

    /// return column data_type
    pub fn column_type(&self, index: i32) -> i32 {
        unsafe { SqliteColumnType(self.handle as _, index) }
    }
}

impl<'b, const PREPARE: bool> Drop for Statement<'b, PREPARE> {
    fn drop(&mut self) {
        if !PREPARE {
            return;
        }
        if self.handle != 0 {
            let ret = unsafe { SqliteFinalize(self.handle as _) };
            if ret != SQLITE_OK {
                loge!("sqlite3 finalize fail ret {}", ret);
            }
        }
    }
}
