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

//! This module implements database statements and provides precompiled query capabilities.

use core::ffi::c_void;
use std::ffi::CStr;

use asset_definition::{log_throw_error, ErrCode, Result, Value};
use asset_log::loge;

use crate::{
    database::Database,
    types::{sqlite_err_handle, SQLITE_DONE, SQLITE_OK, SQLITE_ROW},
};

type BindCallback = extern "C" fn(p: *mut c_void);
extern "C" {
    fn SqliteFinalize(stmt: *mut c_void) -> i32;
    fn SqlitePrepareV2(db: *mut c_void, z_sql: *const u8, pp_stmt: *mut *mut c_void, pz_tail: *mut *mut u8) -> i32;
    fn SqliteBindBlob(stmt: *mut c_void, index: i32, blob: *const u8, n: i32, callback: Option<BindCallback>) -> i32;
    fn SqliteBindInt64(stmt: *mut c_void, index: i32, value: i64) -> i32;
    fn SqliteBindNull(stmt: *mut c_void, index: i32) -> i32;
    fn SqliteStep(stmt: *mut c_void) -> i32;
    fn SqliteColumnName(stmt: *mut c_void, n: i32) -> *const u8;
    fn SqliteDataCount(stmt: *mut c_void) -> i32;
    fn SqliteColumnBlob(stmt: *mut c_void, i_col: i32) -> *const u8;
    fn SqliteColumnInt64(stmt: *mut c_void, i_col: i32) -> i64;
    fn SqliteColumnBytes(stmt: *mut c_void, i_col: i32) -> i32;
    fn SqliteColumnType(stmt: *mut c_void, i_col: i32) -> i32;
    fn SqliteReset(stmt: *mut c_void) -> i32;
}

const SQLITE_INTEGER: i32 = 1;
const SQLITE_BLOB: i32 = 4;
const SQLITE_NULL: i32 = 5;

#[repr(C)]
pub(crate) struct Statement<'b> {
    pub(crate) sql: String,
    db: &'b Database,
    handle: usize, // Poiner to statement.
}

impl<'b> Statement<'b> {
    /// Prepare a sql, you can use '?' for datas and bind datas later.
    pub(crate) fn prepare(sql: &str, db: &'b Database) -> Result<Statement<'b>> {
        let mut tail = 0usize;
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        let mut stmt = Statement { sql: sql_s, handle: 0, db };
        let ret = unsafe {
            SqlitePrepareV2(
                db.handle as _,
                stmt.sql.as_ptr(),
                &mut stmt.handle as *mut usize as _,
                &mut tail as *mut usize as _,
            )
        };
        if ret == 0 {
            Ok(stmt)
        } else {
            db.print_db_msg();
            log_throw_error!(sqlite_err_handle(ret), "Prepare statement failed, err={}", ret)
        }
    }

    /// Executing the precompiled sql. if succ
    /// If the execution is successful, return SQLITE_DONE for update, insert, delete and return SQLITE_ROW for select.
    pub(crate) fn step(&self) -> Result<i32> {
        let ret = unsafe { SqliteStep(self.handle as _) };
        if ret != SQLITE_ROW && ret != SQLITE_DONE {
            self.db.print_db_msg();
            log_throw_error!(sqlite_err_handle(ret), "Step statement failed, err={}", ret)
        } else {
            Ok(ret)
        }
    }

    /// Reset statement before bind data for insert statement.
    #[allow(dead_code)]
    pub(crate) fn reset(&self) -> Result<()> {
        let ret = unsafe { SqliteReset(self.handle as _) };
        if ret != SQLITE_OK {
            self.db.print_db_msg();
            log_throw_error!(sqlite_err_handle(ret), "Reset statement failed, err={}", ret)
        } else {
            Ok(())
        }
    }

    /// Bind data to prepared statement. The index is start from 1.
    pub(crate) fn bind_data(&self, index: i32, data: &Value) -> Result<()> {
        let ret = match data {
            Value::Bytes(b) => unsafe { SqliteBindBlob(self.handle as _, index, b.as_ptr(), b.len() as _, None) },
            Value::Number(i) => unsafe { SqliteBindInt64(self.handle as _, index, *i as _) },
            Value::Bool(b) => unsafe { SqliteBindInt64(self.handle as _, index, *b as _) },
        };
        if ret != SQLITE_OK {
            self.db.print_db_msg();
            log_throw_error!(sqlite_err_handle(ret), "Bind data failed, index={}, err={}", index, ret)
        } else {
            Ok(())
        }
    }

    /// Bind data to prepared statement. The index is start from 1.
    pub(crate) fn bind_data_or_none(&self, index: i32, data: Option<&Value>) -> Result<()> {
        let ret = match data {
            Some(Value::Bytes(b)) => unsafe { SqliteBindBlob(self.handle as _, index, b.as_ptr(), b.len() as _, None) },
            Some(Value::Number(i)) => unsafe { SqliteBindInt64(self.handle as _, index, *i as _) },
            Some(Value::Bool(b)) => unsafe { SqliteBindInt64(self.handle as _, index, *b as _) },
            None => unsafe { SqliteBindNull(self.handle as _, index) },
        };
        if ret != SQLITE_OK {
            self.db.print_db_msg();
            log_throw_error!(sqlite_err_handle(ret), "Bind data failed, index={}, err={}", index, ret)
        } else {
            Ok(())
        }
    }

    /// Query the column name.
    pub(crate) fn query_column_name(&self, n: i32) -> Result<&str> {
        let s = unsafe { SqliteColumnName(self.handle as _, n) };
        if !s.is_null() {
            let name = unsafe { CStr::from_ptr(s as _) };
            if let Ok(rn) = name.to_str() {
                return Ok(rn);
            }
        }
        log_throw_error!(ErrCode::DatabaseError, "[FATAL][DB]Get asset column name failed.")
    }

    /// Get the count of columns in the query result.
    pub(crate) fn data_count(&self) -> i32 {
        unsafe { SqliteDataCount(self.handle as _) }
    }

    /// Query column and return a value of the Value type.
    pub(crate) fn query_column_auto_type(&self, i: i32) -> Result<Option<Value>> {
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
            t => return log_throw_error!(ErrCode::DatabaseError, "Unexpect column type: {}.", t),
        };
        Ok(data)
    }

    /// Query column datas in result set of blob type
    /// The index is start from 0.
    pub(crate) fn query_column_blob(&self, index: i32) -> &[u8] {
        let blob = unsafe { SqliteColumnBlob(self.handle as _, index) };
        let len = self.column_bytes(index);
        unsafe { core::slice::from_raw_parts(blob, len as _) }
    }

    /// Query column datas in result set of int type.
    /// The index is start with 0.
    pub(crate) fn query_column_int(&self, index: i32) -> u32 {
        unsafe { SqliteColumnInt64(self.handle as _, index) as u32 }
    }

    /// Get the bytes of data, you should first call query_column_text or query_column_blob,
    pub(crate) fn column_bytes(&self, index: i32) -> i32 {
        unsafe { SqliteColumnBytes(self.handle as _, index) }
    }

    /// Get the type of column.
    pub(crate) fn column_type(&self, index: i32) -> i32 {
        unsafe { SqliteColumnType(self.handle as _, index) }
    }
}

impl<'b> Drop for Statement<'b> {
    fn drop(&mut self) {
        if self.handle != 0 {
            let ret = unsafe { SqliteFinalize(self.handle as _) };
            if ret != SQLITE_OK {
                loge!("sqlite3 finalize fail ret {}", ret);
            }
        }
    }
}
