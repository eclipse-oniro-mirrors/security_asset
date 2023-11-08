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

//! This module provides interfaces for database management.
//! Databases are isolated based on users and protected by locks.

use core::ffi::c_void;
use std::{ffi::CStr, fs, ptr::null_mut, sync::Mutex};

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_log::{loge, logi};

use crate::{
    statement::Statement,
    table::Table,
    types::{sqlite_err_handle, DbMap, QueryOptions, COLUMN_INFO, SQLITE_OK, TABLE_NAME},
};

extern "C" {
    fn SqliteOpen(file_name: *const u8, pp_db: *mut *mut c_void) -> i32;
    fn SqliteCloseV2(db: *mut c_void) -> i32;
    fn SqliteExec(db: *mut c_void, sql: *const u8, msg: *mut *mut u8) -> i32;
    fn SqliteFree(data: *mut c_void);
    fn SqliteErrMsg(db: *mut c_void) -> *const u8;
}

/// each user have a Database file
pub(crate) struct UserDbLock {
    pub(crate) user_id: i32,
    pub(crate) mtx: Mutex<i32>,
}

static USER_DB_LOCK_LIST: Mutex<Vec<&'static UserDbLock>> = Mutex::new(Vec::new());

/// If the user exists, the reference to the lock is returned.
/// Otherwise, a new lock is created and its reference is returned.
fn get_file_lock_by_user_id(user_id: i32) -> &'static UserDbLock {
    let mut list = USER_DB_LOCK_LIST.lock().unwrap();
    for f in list.iter() {
        if f.user_id == user_id {
            return f;
        }
    }
    let nf = Box::new(UserDbLock { user_id, mtx: Mutex::new(user_id) });
    // SAFETY: We just push item into USER_DB_LOCK_LIST, never remove item or modify item,
    // so return a reference of leak item is safe.
    let nf: &'static UserDbLock = Box::leak(nf);
    list.push(nf);
    list[list.len() - 1]
}

/// Struct used to store database files and connection information.
#[repr(C)]
pub struct Database {
    pub(crate) path: String,
    pub(crate) backup_path: String,
    pub(crate) handle: usize, // Pointer to the database connection.
    pub(crate) db_lock: &'static UserDbLock,
}

/// Callback for database upgrade.
pub type UpgradeDbCallback = fn(db: &Database, old_ver: u32, new_ver: u32) -> Result<()>;

#[cfg(not(test))]
const ROOT_PATH: &str = "/data/service/el1/public/asset_service";
#[cfg(test)]
const ROOT_PATH: &str = "/data/asset_test";

#[inline(always)]
fn fmt_db_path(user_id: i32) -> String {
    format!("{}/{}/asset.db", ROOT_PATH, user_id)
}

#[inline(always)]
fn fmt_backup_path(path: &str) -> String {
    let mut bp = path.to_string();
    bp.push_str(".backup");
    bp
}

impl Database {
    /// Create a database.
    pub fn build(user_id: i32) -> Result<Database> {
        let path = fmt_db_path(user_id);
        let backup_path = fmt_backup_path(path.as_str());
        let lock = get_file_lock_by_user_id(user_id);
        let mut db = Database { path, backup_path, handle: 0, db_lock: lock };
        let _lock = db.db_lock.mtx.lock().unwrap();
        db.open_and_recovery()?;
        db.execute_and_backup(true, |e: &Table| e.create(COLUMN_INFO))?;
        Ok(db)
    }

    // Open database connection.
    pub(crate) fn open(&mut self) -> Result<()> {
        let mut path_c = self.path.clone();
        path_c.push('\0');

        let ret = unsafe { SqliteOpen(path_c.as_ptr(), &mut self.handle as *mut usize as _) };
        if ret == SQLITE_OK {
            Ok(())
        } else {
            self.close();
            log_throw_error!(sqlite_err_handle(ret), "[FATAL][DB]Open database failed, err={}", ret)
        }
    }

    /// Open the database connection and recovery the database if the connection fails.
    fn open_and_recovery(&mut self) -> Result<()> {
        match self.open() {
            Err(ret) if ret.code == ErrCode::DataCorrupted => self.recovery(),
            ret => ret,
        }
    }

    /// Close database connection.
    pub(crate) fn close(&mut self) {
        if self.handle != 0 {
            unsafe { SqliteCloseV2(self.handle as _) };
            self.handle = 0;
        }
    }

    // Recovery the corrupt database and reopen it.
    pub(crate) fn recovery(&mut self) -> Result<()> {
        logi!("[WARNING]Database is corrupt, start to recovery, path={}", self.path);
        self.close();
        if let Err(e) = fs::copy(&self.backup_path, &self.path) {
            return log_throw_error!(ErrCode::FileOperationError, "[FATAL][DB]Recovery database failed, err={}", e);
        }
        self.open()
    }

    /// Get database version, default is 0.
    pub(crate) fn get_version(&self) -> Result<u32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let stmt = Statement::prepare("pragma user_version", self)?;
        stmt.step()?;
        let version = stmt.query_column_int(0);
        Ok(version)
    }

    /// Update the database version for database upgrade.
    pub fn set_version(&self, ver: u32) -> Result<()> {
        let sql = format!("pragma user_version = {}", ver);
        self.exec(sql.as_str())
    }

    /// Upgrade database to new version.
    pub fn upgrade(&self, ver: u32, callback: UpgradeDbCallback) -> Result<()> {
        let version_old = self.get_version()?;
        callback(self, version_old, ver)
    }

    /// Delete database file.
    pub fn delete(user_id: i32) -> Result<()> {
        let path = fmt_db_path(user_id);
        let backup_path = fmt_backup_path(&path);
        if let Err(e) = fs::remove_file(path) {
            return log_throw_error!(ErrCode::FileOperationError, "[FATAL][DB]Delete database failed, err={}", e);
        }

        if let Err(e) = fs::remove_file(backup_path) {
            return log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][DB]Delete backup database failed, err={}",
                e
            );
        }
        Ok(())
    }

    /// Print the error message of database.
    pub(crate) fn print_db_msg(&self) {
        let msg = unsafe { SqliteErrMsg(self.handle as _) };
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            if let Ok(rs) = s.to_str() {
                loge!("[FATAL][DB]Database error message: {}", rs);
            }
        }
    }

    /// execute sql without prepare
    pub(crate) fn exec(&self, sql: &str) -> Result<()> {
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        let mut msg: *mut u8 = null_mut();
        let ret = unsafe { SqliteExec(self.handle as _, sql_s.as_ptr(), &mut msg as _) };
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            if let Ok(rs) = s.to_str() {
                return log_throw_error!(sqlite_err_handle(ret), "[FATAL]Database execute sql failed, err={}", rs);
            }
            unsafe { SqliteFree(msg as _) };
        }
        if ret == SQLITE_OK {
            Ok(())
        } else {
            log_throw_error!(sqlite_err_handle(ret), "[FATAL]Database execute sql failed.")
        }
    }

    /// do same operation in backup database when do something in main db
    /// backup every success operation, recovery every fail operation
    pub(crate) fn execute_and_backup<T, F: Fn(&Table) -> Result<T>>(&mut self, modified: bool, func: F) -> Result<T> {
        let table = Table::new(TABLE_NAME, self);
        let ret = match func(&table) {
            Err(ret) if ret.code == ErrCode::DataCorrupted => {
                self.recovery()?;
                let table = Table::new(TABLE_NAME, self); // Database handle will be changed.
                func(&table)
            },
            ret => ret,
        };

        if ret.is_ok() && modified && fs::copy(&self.path, &self.backup_path).is_err() {
            loge!("[WARNING]Backup database {} failed", self.backup_path);
        }
        ret
    }

    /// Insert datas into database.
    /// The datas is a map of column-data pair.
    /// If the operation is successful, the number of inserted data is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: insert into table_name(Owner,Alias,value) values('owner','alias','insert_value')
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// datas.insert("value", Value::Bytes(b"insert_value".to_vec()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.insert_datas(&datas);
    /// ```
    ///
    #[inline(always)]
    pub fn insert_datas(&mut self, datas: &DbMap) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.insert_row(datas);
        self.execute_and_backup(true, closure)
    }

    /// Delete datas from database.
    /// The condition is a map of column-data pair.
    /// If the operation is successful, the number of deleted data is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: delete from table_name where Owner='owner' and Alias='alias' and value='delete_value'
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// datas.insert("value", Value::Bytes(b"delete_value".to_vec()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.delete_datas(&cond);
    /// ```
    ///
    ///
    #[inline(always)]
    pub fn delete_datas(&mut self, condition: &DbMap) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.delete_row(condition);
        self.execute_and_backup(true, closure)
    }

    /// Update datas in database.
    /// The datas is a map of column-data pair.
    /// If the operation is successful, the number of updated data is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: update table_name set alias='update_value' where Owner='owner' and Alias='alias'
    /// let condition = DbMap.new();
    /// condition.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// condition.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let datas = DbMap::from([("alias", Value::Bytes(b"update_value".to_vec()))]);
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.update_datas(&condition, &datas);
    /// ```
    #[inline(always)]
    pub fn update_datas(&mut self, condition: &DbMap, datas: &DbMap) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.update_row(condition, datas);
        self.execute_and_backup(true, closure)
    }

    /// Check whether data exists in the database.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: select count(*) as count from table_name where Owner='owner' and Alias='alias'
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let exist = Database::build(user_id)?.is_data_exists(&datas);
    /// ```
    #[inline(always)]
    pub fn is_data_exists(&mut self, condition: &DbMap) -> Result<bool> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.is_data_exists(condition);
        self.execute_and_backup(false, closure)
    }

    /// Query data that meets specified conditions(can be empty) from the database.
    /// If the operation is successful, the resultSet is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: select * from table_name where Owner='owner' and Alias='alias'
    /// let cond = DbMap::new();
    /// cond.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// cond.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.query_datas(&vec![], &cond, None);
    /// ```
    #[inline(always)]
    pub fn query_datas(
        &mut self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.query_row(columns, condition, query_options, COLUMN_INFO);
        self.execute_and_backup(false, closure)
    }

    /// Delete old data and insert new data.
    pub fn replace_datas(&mut self, condition: &DbMap, datas: &DbMap) -> Result<()> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.replace_row(condition, datas);
        self.execute_and_backup(true, closure)
    }
}

impl Drop for Database {
    fn drop(&mut self) {
        self.close()
    }
}
