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

//! This module provides interfaces for database management.
//! Databases are isolated based on users and protected by locks.

use core::ffi::c_void;
use std::{ffi::CStr, fs, ptr::null_mut, sync::Mutex};

use asset_common::{CallingInfo, OwnerType};
use asset_db_key_operator::DbKey;
use asset_definition::{log_throw_error, AssetMap, ErrCode, Extension, Result, Tag, Value};
use asset_file_operator::{ce_operator::remove_ce_files, common::is_file_exist};
use asset_log::{loge, logi};

use crate::{
    database_file_upgrade::{check_and_split_db, construct_splited_db_name, fmt_old_de_db_path},
    statement::Statement,
    table::Table,
    types::{
        column, sqlite_err_handle, DbMap, QueryOptions, COLUMN_INFO, DB_UPGRADE_VERSION, DB_UPGRADE_VERSION_V1,
        DB_UPGRADE_VERSION_V2, SQLITE_OK, TABLE_NAME, UPGRADE_COLUMN_INFO, UPGRADE_COLUMN_INFO_V2,
    },
};

extern "C" {
    fn SqliteOpen(file_name: *const u8, pp_db: *mut *mut c_void) -> i32;
    fn SqliteCloseV2(db: *mut c_void) -> i32;
    fn SqliteExec(db: *mut c_void, sql: *const u8, msg: *mut *mut u8) -> i32;
    fn SqliteFree(data: *mut c_void);
    fn SqliteErrMsg(db: *mut c_void) -> *const u8;
    fn SqliteKey(db: *mut c_void, pKey: *const c_void, nKey: i32) -> i32;
}

/// each user have a Database file
pub(crate) struct UserDbLock {
    pub(crate) user_id: i32,
    pub(crate) mtx: Mutex<i32>,
    #[allow(dead_code)]
    pub(crate) db_file_name: String,
}

static USER_DB_LOCK_LIST: Mutex<Vec<&'static UserDbLock>> = Mutex::new(Vec::new());
static SPLIT_DB_LOCK_LIST: Mutex<Vec<&'static UserDbLock>> = Mutex::new(Vec::new());
pub(crate) static OLD_DB_NAME: &str = "asset";

pub(crate) fn get_split_db_lock_by_user_id(user_id: i32) -> &'static UserDbLock {
    let mut list = SPLIT_DB_LOCK_LIST.lock().unwrap();
    for f in list.iter() {
        if f.user_id == user_id {
            return f;
        }
    }
    let nf = Box::new(UserDbLock { user_id, mtx: Mutex::new(user_id), db_file_name: OLD_DB_NAME.clone().to_string() });
    // SAFETY: We just push item into USER_DB_LOCK_LIST, never remove item or modify item,
    // so return a reference of leak item is safe.
    let nf: &'static UserDbLock = Box::leak(nf);
    list.push(nf);
    list[list.len() - 1]
}

/// If the user exists, the reference to the lock is returned.
/// Otherwise, a new lock is created and its reference is returned.
pub(crate) fn get_file_lock_by_user_id_db_file_name(user_id: i32, db_file_name: String) -> &'static UserDbLock {
    let mut list = USER_DB_LOCK_LIST.lock().unwrap();
    for f in list.iter() {
        if f.user_id == user_id && f.db_file_name == db_file_name {
            return f;
        }
    }
    let nf = Box::new(UserDbLock { user_id, mtx: Mutex::new(user_id), db_file_name });
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
    pub(crate) db_name: String,
}

/// Callback for database upgrade.
pub type UpgradeDbCallback = fn(db: &Database, old_ver: u32, new_ver: u32) -> Result<()>;

#[cfg(not(test))]
pub(crate) const DE_ROOT_PATH: &str = "/data/service/el1/public/asset_service";
#[cfg(test)]
pub(crate) const DE_ROOT_PATH: &str = "/data/asset_test";

#[inline(always)]
pub(crate) fn fmt_backup_path(path: &str) -> String {
    let mut bp = path.to_string();
    bp.push_str(".backup");
    bp
}

/// Get asset storage path.
pub fn get_path() -> String {
    DE_ROOT_PATH.to_string()
}

#[inline(always)]
pub(crate) fn fmt_ce_db_path_with_name(user_id: i32, db_name: &str) -> String {
    format!("data/service/el2/{}/asset_service/{}.db", user_id, db_name)
}

#[inline(always)]
pub(crate) fn fmt_de_db_path_with_name(user_id: i32, db_name: &str) -> String {
    format!("{}/{}/{}.db", DE_ROOT_PATH, user_id, db_name)
}

fn check_validity_of_db_key(path: &str, user_id: i32) -> Result<()> {
    if is_file_exist(path)? && !DbKey::check_existance(user_id)? {
        loge!("[FATAL]There is database bot no database key. Now all data should be cleared and restart over.");
        remove_ce_files(user_id)?;
        return log_throw_error!(ErrCode::DataCorrupted, "[FATAL]All data is cleared in {}.", user_id);
    }
    Ok(())
}

pub(crate) fn get_db(user_id: i32, db_name: &str, is_ce: bool) -> Result<Database> {
    let path =
        if is_ce { fmt_ce_db_path_with_name(user_id, db_name) } else { fmt_de_db_path_with_name(user_id, db_name) };
    let db_key = if is_ce {
        check_validity_of_db_key(&path, user_id)?;
        let calling_info = CallingInfo::new_part_info(user_id);
        match DbKey::get_db_key(&calling_info) {
            Ok(res) => Some(res),
            Err(e) if e.code == ErrCode::NotFound || e.code == ErrCode::DataCorrupted => {
                loge!(
                    "[FATAL]The key is corrupted. Now all data should be cleared and restart over, err is {}.",
                    e.code
                );
                remove_ce_files(user_id)?;
                return log_throw_error!(ErrCode::DataCorrupted, "[FATAL]All data is cleared in {}.", user_id);
            },
            Err(e) => return Err(e),
        }
    } else {
        None
    };

    let backup_path = fmt_backup_path(path.as_str());
    let lock = get_file_lock_by_user_id_db_file_name(user_id, db_name.to_string().clone());
    let mut db = Database { path, backup_path, handle: 0, db_lock: lock, db_name: db_name.to_string() };
    let _lock = db.db_lock.mtx.lock().unwrap();
    db.open_and_restore(db_key.as_ref())?;
    db.restore_if_exec_fail(|e: &Table| e.create(COLUMN_INFO))?;
    db.upgrade(DB_UPGRADE_VERSION, |_, _, _| Ok(()))?;
    Ok(db)
}

/// Create de db instance if the value of tag "RequireAttrEncrypted" is not specified or set to false.
/// Create ce db instance if true.
pub fn create_db_instance(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<Database> {
    match attributes.get(&Tag::RequireAttrEncrypted) {
        Some(Value::Bool(true)) => {
            let db = Database::build(calling_info, true)?;
            Ok(db)
        },
        _ => {
            let db = Database::build(calling_info, false)?;
            Ok(db)
        },
    }
}

impl Database {
    /// Create a database.
    pub fn build(calling_info: &CallingInfo, is_ce: bool) -> Result<Database> {
        if !is_ce {
            // DE database needs trigger the upgrade action.
            check_and_split_db(calling_info.user_id())?;
        }
        get_db(
            calling_info.user_id(),
            &construct_splited_db_name(calling_info.owner_type_enum(), calling_info.owner_info(), is_ce)?,
            is_ce,
        )
    }

    /// Create a database from a file name.
    pub fn build_with_file_name(user_id: i32, db_name: &str, is_ce: bool) -> Result<Database> {
        check_and_split_db(user_id)?;
        get_db(user_id, db_name, is_ce)
    }

    /// Check whether de db is ok
    pub fn check_de_db_accessible(path: String, user_id: i32, db_name: String) -> Result<()> {
        let lock = get_file_lock_by_user_id_db_file_name(user_id, db_name.clone());
        let mut db = Database { path: path.clone(), backup_path: path, handle: 0, db_lock: lock, db_name };
        db.open()?;
        let table = Table::new(TABLE_NAME, &db);
        table.create(COLUMN_INFO)
    }

    /// Open database connection.
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

    /// Open the database connection and restore the database if the connection fails.
    pub(crate) fn open_and_restore(&mut self, db_key: Option<&DbKey>) -> Result<()> {
        let result = self.open();
        if let Some(db_key) = db_key {
            self.set_db_key(db_key)?;
        }
        let result = match result {
            Err(ret) if ret.code == ErrCode::DataCorrupted => self.restore(),
            ret => ret,
        };
        result
    }

    /// Get db name.
    pub(crate) fn get_db_name(&mut self) -> &str {
        &self.db_name
    }

    /// Close database connection.
    fn close(&mut self) {
        if self.handle != 0 {
            unsafe { SqliteCloseV2(self.handle as _) };
            self.handle = 0;
        }
    }

    /// Close database connection.
    pub(crate) fn close_db(&mut self) {
        let _lock = self.db_lock.mtx.lock().unwrap();
        self.close()
    }

    /// Encrypt/Decrypt CE database.
    pub fn set_db_key(&mut self, db_key: &DbKey) -> Result<()> {
        let ret =
            unsafe { SqliteKey(self.handle as _, db_key.db_key.as_ptr() as *const c_void, db_key.db_key.len() as i32) };
        if ret == SQLITE_OK {
            Ok(())
        } else {
            log_throw_error!(sqlite_err_handle(ret), "[FATAL][DB]Set database key failed, err={}", ret)
        }
    }

    // Recovery the corrupt database and reopen it.
    pub(crate) fn restore(&mut self) -> Result<()> {
        loge!("[WARNING]Database is corrupt, start to restore");
        self.close();
        if let Err(e) = fs::copy(&self.backup_path, &self.path) {
            return log_throw_error!(ErrCode::FileOperationError, "[FATAL][DB]Recovery database failed, err={}", e);
        }
        self.open()
    }

    /// Get database version, default is 0.
    fn get_db_version(&self) -> Result<u32> {
        let stmt = Statement::prepare("pragma user_version", self)?;
        stmt.step()?;
        let version = stmt.query_column_int(0);
        Ok(version)
    }

    /// Get database version, default is 0.
    #[allow(dead_code)]
    pub(crate) fn get_version(&self) -> Result<u32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        self.get_db_version()
    }

    /// Update the database version for database upgrade.
    #[allow(dead_code)]
    pub(crate) fn set_version(&self, ver: u32) -> Result<()> {
        let sql = format!("pragma user_version = {}", ver);
        self.exec(sql.as_str())
    }

    /// Upgrade database to new version.
    #[allow(dead_code)]
    pub fn upgrade(&mut self, ver: u32, callback: UpgradeDbCallback) -> Result<()> {
        let mut version_old = self.get_db_version()?;
        logi!("current database version: {}", version_old);
        if version_old >= ver {
            return Ok(());
        }
        if version_old == DB_UPGRADE_VERSION_V1 {
            self.restore_if_exec_fail(|e: &Table| e.upgrade(DB_UPGRADE_VERSION_V2, UPGRADE_COLUMN_INFO_V2))?;
            version_old += 1;
        }
        if version_old == DB_UPGRADE_VERSION_V2 {
            self.restore_if_exec_fail(|e: &Table| e.upgrade(DB_UPGRADE_VERSION, UPGRADE_COLUMN_INFO))?;
        }
        callback(self, version_old, ver)
    }

    /// Delete database file.
    #[allow(dead_code)]
    pub(crate) fn delete(user_id: i32) -> Result<()> {
        let path = fmt_old_de_db_path(user_id);
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
    pub fn exec(&self, sql: &str) -> Result<()> {
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        let mut msg: *mut u8 = null_mut();
        let ret = unsafe { SqliteExec(self.handle as _, sql_s.as_ptr(), &mut msg as _) };
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            if let Ok(rs) = s.to_str() {
                return log_throw_error!(
                    sqlite_err_handle(ret),
                    "[FATAL]Database execute sql failed. error code={}, error msg={}",
                    ret,
                    rs
                );
            }
            unsafe { SqliteFree(msg as _) };
        }
        if ret == SQLITE_OK {
            Ok(())
        } else {
            log_throw_error!(sqlite_err_handle(ret), "[FATAL]Database execute sql failed. error code={}", ret)
        }
    }

    /// execute func in db, if failed and error code is data corrupted then restore
    pub(crate) fn restore_if_exec_fail<T, F: Fn(&Table) -> Result<T>>(&mut self, func: F) -> Result<T> {
        let table = Table::new(TABLE_NAME, self);
        let result = func(&table);
        match result {
            Err(ret) if ret.code == ErrCode::DataCorrupted => {
                self.restore()?;
                let table = Table::new(TABLE_NAME, self); // Database handle will be changed.
                func(&table)
            },
            ret => ret,
        }
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
    /// // SQL: insert into table_name(Owner,OwnerType,Alias,value) values('owner',1,'alias','insert_value')
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// datas.insert("value", Value::Bytes(b"insert_value".to_vec()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.insert_datas(&datas);
    /// ```
    ///
    #[inline(always)]
    pub fn insert_datas(&mut self, datas: &DbMap) -> Result<i32> {
        let _lock: std::sync::MutexGuard<'_, i32> = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| {
            let mut query = DbMap::new();
            query.insert_attr(column::ALIAS, datas.get_bytes_attr(&column::ALIAS)?.clone());
            query.insert_attr(column::OWNER, datas.get_bytes_attr(&column::OWNER)?.clone());
            query.insert_attr(column::OWNER_TYPE, datas.get_enum_attr::<OwnerType>(&column::OWNER_TYPE)?);
            if e.is_data_exists(&query, false)? {
                log_throw_error!(ErrCode::Duplicated, "[FATAL]The data with the specified alias already exists.")
            } else {
                e.insert_row(datas)
            }
        };
        self.restore_if_exec_fail(closure)
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
    /// // SQL: delete from table_name where Owner='owner' and OwnerType=1 and Alias='alias' and value='delete_value'
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// datas.insert("value", Value::Bytes(b"delete_value".to_vec()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.delete_datas(&cond, None, false);
    /// ```
    ///
    ///
    #[inline(always)]
    pub fn delete_datas(
        &mut self,
        condition: &DbMap,
        reverse_condition: Option<&DbMap>,
        is_filter_sync: bool,
    ) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.delete_row(condition, reverse_condition, is_filter_sync);
        self.restore_if_exec_fail(closure)
    }

    /// Delete datas from database with specific condition.
    /// If the operation is successful, the number of deleted data is returned.
    #[inline(always)]
    pub fn delete_specific_condition_datas(&mut self, specific_cond: &str, condition_value: &[Value]) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.delete_with_specific_cond(specific_cond, condition_value);
        self.restore_if_exec_fail(closure)
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
    /// // SQL: update table_name set alias='update_value' where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let cond = DbMap.new();
    /// cond.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// cond.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let datas = DbMap::from([("alias", Value::Bytes(b"update_value".to_vec()))]);
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.update_datas(&condition, true, &datas);
    /// ```
    #[inline(always)]
    pub fn update_datas(&mut self, condition: &DbMap, is_filter_sync: bool, datas: &DbMap) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.update_row(condition, is_filter_sync, datas);
        self.restore_if_exec_fail(closure)
    }

    /// Check whether data exists in the database.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: select count(*) as count from table_name where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let exist = Database::build(user_id)?.is_data_exists(&datas, false);
    /// ```
    #[inline(always)]
    pub fn is_data_exists(&mut self, condition: &DbMap, is_filter_sync: bool) -> Result<bool> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.is_data_exists(condition, is_filter_sync);
        self.restore_if_exec_fail(closure)
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
    /// // SQL: select * from table_name where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let cond = DbMap::new();
    /// cond.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// cond.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.query_datas(&vec![], &cond, None, false);
    /// ```
    #[inline(always)]
    pub fn query_datas(
        &mut self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
        is_filter_sync: bool,
    ) -> Result<Vec<DbMap>> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.query_row(columns, condition, query_options, is_filter_sync, COLUMN_INFO);
        self.restore_if_exec_fail(closure)
    }

    /// query how many data fit the query condition
    pub fn query_data_count(&mut self, condition: &DbMap) -> Result<u32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.count_datas(condition, false);
        self.restore_if_exec_fail(closure)
    }

    /// Delete old data and insert new data.
    pub fn replace_datas(&mut self, condition: &DbMap, is_filter_sync: bool, datas: &DbMap) -> Result<()> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.replace_row(condition, is_filter_sync, datas);
        self.restore_if_exec_fail(closure)
    }
}

impl Drop for Database {
    fn drop(&mut self) {
        self.close_db()
    }
}
