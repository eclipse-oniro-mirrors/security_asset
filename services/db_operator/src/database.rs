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
use std::{ffi::CStr, fs, path::Path, ptr::null_mut, sync::Mutex};

use super::*;
use crate::{
    statement::Statement,
    table::Table,
    types::{from_datatype_to_str, ColumnInfo, Sqlite3ErrMsg},
};

/// each user have a Database file
pub struct UseridFileLock {
    /// userid
    pub(crate) userid: i32,
    /// file lock
    pub(crate) mtx: Mutex<i32>,
}

/// save all the userid file locks
static G_USER_FILE_LOCK_LIST: Mutex<Vec<&'static UseridFileLock>> = Mutex::new(Vec::new());

/// if userid exists, return reference, or create a new lock, insert into list and return reference
fn get_file_lock_by_userid(userid: i32) -> &'static UseridFileLock {
    let mut list = G_USER_FILE_LOCK_LIST.lock().unwrap();
    for f in list.iter() {
        if f.userid == userid {
            return f;
        }
    }
    let nf = Box::new(UseridFileLock { userid, mtx: Mutex::new(userid) });
    // SAFETY: We just push item into G_USER_FILE_LOCK_LIST, never remove item or modify item,
    // so return a reference of leak item is safe.
    let nf: &'static UseridFileLock = Box::leak(nf);
    list.push(nf);
    list[list.len() - 1]
}

/// sqlite database file
#[repr(C)]
pub struct Database<'a> {
    /// database file string
    pub(crate) path: String,
    /// database file string
    pub(crate) back_path: String,
    /// is opened with sqlite3_open_v2
    pub(crate) v2: bool,
    /// open flags
    pub(crate) flags: i32,
    /// vfs
    pub(crate) vfs: Option<&'a [u8]>,
    /// raw pointer
    pub(crate) handle: usize,
    /// db file
    pub(crate) file: &'a UseridFileLock,
    /// backup
    pub(crate) backup_handle: usize,
}

/// update func callback
pub type UpdateDatabaseCallbackFunc =
    fn(db: &Database, old_ver: u32, new_ver: u32) -> SqliteErrCode;

/// default callback func for update database
pub fn default_update_database_func(db: &Database, old_ver: u32, new_ver: u32) -> SqliteErrCode {
    if new_ver > old_ver {
        // TODO do something
        asset_common::loge!("database {} update from ver {} to {}", db.path, old_ver, new_ver);
        return db.update_version(new_ver as _);
    }
    if new_ver < old_ver {
        asset_common::loge!("database version rollback is not supported!");
        return SQLITE_ERROR;
    }
    SQLITE_OK
}

/// format database path
#[inline(always)]
fn fmt_db_path(userid: i32) -> String {
    format!("/data/service/el1/public/asset_service/{}/asset.db", userid)
}

/// get backup path
fn fmt_backup_path(path: &str) -> String {
    let mut bp = path.to_string();
    bp.push_str(".backup");
    bp
}

/// recovery if database file format error
/// if recovery_master == false, will recovery backup file
pub fn recovery_db_file(db: &Database, recovery_master: bool) -> Result<u64, std::io::Error> {
    if recovery_master {
        fs::copy(&db.back_path, &db.path)
    } else {
        fs::copy(&db.path, &db.back_path)
    }
}

/// wrap sqlite open
#[inline(always)]
fn sqlite3_open_wrap(
    file: &str,
    handle: &mut usize,
    flag: i32,
    vfs: Option<&[u8]>,
    v2: bool,
) -> SqliteErrCode {
    if v2 {
        sqlite3_open_v2_func(file, handle, flag, vfs)
    } else {
        sqlite3_open_func(file, handle)
    }
}

/// is corrupt
fn is_database_file_error(ret: SqliteErrCode) -> bool {
    ret == SQLITE_CORRUPT || ret == SQLITE_NOTADB
}

/// open db, will recovery wrong db file
fn open_db(
    db: &mut Database,
    path: String,
    back_path: String,
    flag: i32,
    vfs: Option<&[u8]>,
) -> Result<(), SqliteErrCode> {
    let _lock = db.file.mtx.lock().unwrap();
    let ret = sqlite3_open_wrap(&path, &mut db.handle, flag, vfs, db.v2);
    let b_ret = sqlite3_open_wrap(&back_path, &mut db.backup_handle, flag, vfs, db.v2);
    if ret == SQLITE_OK && b_ret == SQLITE_OK {
        Ok(())
    } else if ret == SQLITE_OK && is_database_file_error(b_ret) {
        db.backup_handle = 0;
        let ret = recovery_db_file(db, false);
        if ret.is_ok() {
            asset_common::loge!("recovery {} succ", db.back_path);
            // TODO ignore return value : if re open backup fail
            let ret = sqlite3_open_wrap(&back_path, &mut db.backup_handle, flag, vfs, db.v2);
            if ret != SQLITE_OK {
                asset_common::loge!("re open {} fail", &back_path);
            }
        } else {
            // TODO ignore error : if recovery backup fail
            asset_common::loge!("recovery {} fail", db.back_path);
        }
        Ok(())
    } else if is_database_file_error(ret) && b_ret == SQLITE_OK {
        let ret = recovery_db_file(db, true);
        // swap master and backup
        db.switch_master_backup();
        if ret.is_ok() {
            asset_common::loge!("recovery {} succ", db.back_path);
            // TODO ignore return value : if re open backup fail
            let ret = sqlite3_open_wrap(&path, &mut db.backup_handle, flag, vfs, db.v2);
            if ret != SQLITE_OK {
                asset_common::loge!("re open {} fail", &path);
            }
        } else {
            // TODO ignore error : if recovery backup fail
            asset_common::loge!("recovery {} fail", db.back_path);
        }
        Ok(())
    } else {
        Err(ret)
    }
}

impl<'a> Database<'a> {
    /// create backup db
    pub(crate) fn backup_db(&self) -> Database {
        Database {
            path: self.back_path.clone(),
            back_path: self.path.clone(),
            v2: self.v2,
            flags: 0,
            vfs: None,
            handle: self.backup_handle,
            file: self.file,
            backup_handle: self.handle,
        }
    }

    /// switch master backup
    pub(crate) fn switch_master_backup(&mut self) {
        self.handle = self.backup_handle;
        self.backup_handle = 0;
        let p = self.path.clone();
        self.path = self.back_path.clone();
        self.back_path = p;
    }

    /// reopen db file
    pub(crate) fn re_open(&mut self, re_open_master: bool) {
        let re_open_func =
            |v2: bool, handle: &mut usize, path: &String, flags: i32, vfs: Option<&[u8]>| {
                if *handle != 0 {
                    *handle = 0;
                }
                let mut path_c = path.clone();
                path_c.push('\0');
                let ret = sqlite3_open_wrap(&path_c, handle, flags, vfs, v2);
                if ret != SQLITE_OK {
                    asset_common::loge!("re open handle {} fail {}", path, ret);
                }
            };
        if re_open_master {
            re_open_func(self.v2, &mut self.handle, &self.path, self.flags, self.vfs);
        } else {
            re_open_func(self.v2, &mut self.backup_handle, &self.back_path, self.flags, self.vfs);
        }
    }

    /// open database file.
    /// will create it if not exits.
    pub fn new(path: &str) -> Result<Database, SqliteErrCode> {
        let mut path_c = path.to_string();
        let mut back_path_c = fmt_backup_path(path);
        let mut db = Database {
            path: path_c.clone(),
            back_path: back_path_c.clone(),
            v2: false,
            flags: 0,
            vfs: None,
            handle: 0,
            file: get_file_lock_by_userid(i32::MAX),
            backup_handle: 0,
        };
        path_c.push('\0');
        back_path_c.push('\0');
        open_db(&mut db, path_c, back_path_c, 0, None)?;
        Ok(db)
    }

    /// create default database
    pub fn default_new(userid: i32) -> Result<Database<'a>, SqliteErrCode> {
        let path = fmt_db_path(userid);
        let mut path_c = path.clone();
        let mut back_path_c = fmt_backup_path(path.as_str());
        let mut db = Database {
            path: path_c.clone(),
            back_path: back_path_c.clone(),
            v2: false,
            flags: 0,
            vfs: None,
            handle: 0,
            file: get_file_lock_by_userid(i32::MAX),
            backup_handle: 0,
        };
        path_c.push('\0');
        back_path_c.push('\0');
        open_db(&mut db, path_c, back_path_c, 0, None)?;
        Ok(db)
    }

    /// get database user_version
    pub fn get_version(&self) -> Result<u32, SqliteErrCode> {
        let _lock = self.file.mtx.lock().unwrap();
        let stmt = Statement::<true>::prepare("pragma user_version", self)?;
        let ret = stmt.step();
        if ret != SQLITE_ROW {
            return Err(ret);
        }
        let version_old = stmt.query_column_int(0);
        Ok(version_old)
    }

    /// open database with version update callback
    pub fn new_with_version_update(
        path: &str,
        ver: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database, SqliteErrCode> {
        let db = Database::new(path)?;
        let version_old = db.get_version()?;
        #[cfg(test)]
        {
            println!("database version old {}", version_old);
        }
        let ret = callback(&db, version_old, ver);
        if ret != SQLITE_OK {
            return Err(ret);
        }

        Ok(db)
    }

    /// open database with version update callback
    pub fn default_new_with_version_update(
        userid: i32,
        ver: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database<'a>, SqliteErrCode> {
        let db = Database::default_new(userid)?;
        let version_old = db.get_version()?;
        #[cfg(test)]
        {
            println!("database version old {}", version_old);
        }
        let ret = callback(&db, version_old, ver);
        if ret != SQLITE_OK {
            return Err(ret);
        }

        Ok(db)
    }

    /// open database file
    /// use sqlite3_open_v2 instead of sqlite3_open
    pub fn new_v2(
        path: &str,
        flags: i32,
        vfs: Option<&'a [u8]>,
    ) -> Result<Database<'a>, SqliteErrCode> {
        let mut path_c = path.to_string();
        let mut back_path_c = fmt_backup_path(path);
        let mut db = Database {
            path: path_c.clone(),
            back_path: back_path_c.clone(),
            v2: true,
            flags,
            vfs,
            handle: 0,
            file: get_file_lock_by_userid(i32::MAX),
            backup_handle: 0,
        };
        path_c.push('\0');
        back_path_c.push('\0');
        open_db(&mut db, path_c, back_path_c, flags, vfs)?;
        Ok(db)
    }

    /// delete database with delete the file
    pub fn drop_database(path: &str) -> std::io::Result<()> {
        let name = String::from_utf8(path.as_bytes().to_vec()).unwrap();
        let name = name.trim_matches(char::from(0));
        fs::remove_file(name)
    }

    /// delete database with delete the file and backup file
    pub fn drop_database_and_backup(self) -> std::io::Result<()> {
        let _lock = self.file.mtx.lock().unwrap();
        let path = self.path.clone();
        let path: &Path = path.as_ref();
        let back_path = self.back_path.clone();
        let back_path: &Path = back_path.as_ref();
        drop(self);
        let ret = fs::remove_file(path);
        let back_ret = if back_path.exists() { fs::remove_file(back_path) } else { Ok(()) };
        ret?;
        back_ret
    }

    /// delete default database
    pub fn drop_default_database(userid: i32) -> std::io::Result<()> {
        let path = fmt_db_path(userid);
        Database::drop_database(path.as_str())
    }

    /// delete default database and backup db
    pub fn drop_default_database_and_backup(userid: i32) -> std::io::Result<()> {
        let path = fmt_db_path(userid);
        let back_path = fmt_backup_path(path.as_str());
        let ret = Database::drop_database(path.as_str());
        let back_ret = Database::drop_database(back_path.as_str());
        ret?;
        back_ret
    }

    /// return err msg if get error.
    /// return None if no error.
    /// You do NOT need to free err msg, it's auto freed.
    pub fn get_err_msg(&self) -> Option<Sqlite3ErrMsg> {
        let msg = sqlite3_err_msg_func(self.handle);
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            let se = Sqlite3ErrMsg { s: s.to_str().unwrap(), db: self };
            return Some(se);
        }
        None
    }

    pub(crate) fn print_err_msg(&self, msg: *const u8) {
        unsafe {
            let s = CStr::from_ptr(msg as _);
            asset_common::loge!("exec fail error msg: {}", s.to_str().unwrap());
        }
    }

    /// execute sql without prepare.
    /// you should use statement.step for prepared statement.
    /// callback function for process result set.
    /// the final param data will be passed into callback function.
    pub fn exec(
        &self,
        stmt: &Statement<false>,
        callback: Option<Sqlite3Callback>,
        data: usize,
    ) -> SqliteErrCode {
        let mut msg = null_mut();
        let ret = sqlite3_exec_func(self.handle, &stmt.sql, callback, data, &mut msg);
        if !msg.is_null() {
            self.print_err_msg(msg);
            sqlite3_free_func(msg as _);
            return SQLITE_ERROR;
        }
        ret
    }

    /// set database version
    pub fn update_version(&self, ver: u32) -> SqliteErrCode {
        let sql = format!("pragma user_version = {}", ver);
        let statement = Statement::new(sql.as_str(), self);
        statement.exec(None, 0)
    }

    /// open a table, if the table not exists, return Ok(None)
    pub fn open_table(&self, table_name: &str) -> Result<Option<Table>, SqliteErrCode> {
        let sql =
            format!("select * from sqlite_master where type ='table' and name = '{}'", table_name);
        let stmt = Statement::<true>::prepare(sql.as_str(), self)?;
        let ret = stmt.step();
        if ret != SQLITE_ROW {
            if ret == SQLITE_DONE {
                Ok(None)
            } else {
                Err(ret)
            }
        } else {
            Ok(Some(Table::new(table_name, self)))
        }
    }

    /// drop a table
    pub fn drop_table(&self, table_name: &str) -> SqliteErrCode {
        let sql = format!("DROP TABLE {}", table_name);
        let stmt = Statement::<false>::new(sql.as_str(), self);
        stmt.exec(None, 0)
    }

    /// create table with name 'table_name'
    /// the columns is descriptions for each column.
    /// for each column, there is 4 attr
    /// name, is_primary_key, is_not_null, datatype
    /// code like follows:
    ///
    /// let db = match Database::new("test7.db") {
    ///     Ok(o) => o,
    ///     Err(ret) => {
    ///         println!("test sqlite3 open fail ret {}", ret);
    ///     }
    /// };
    /// let columns = &[
    ///     ColumnInfo {
    ///         name: "id",
    ///         is_primary_key: true,
    ///         not_null: true,
    ///         data_type: DataType::INTEGER,
    ///     },
    ///     ColumnInfo {
    ///         name: "alias",
    ///         is_primary_key: false,
    ///         not_null: true,
    ///         data_type: DataType::TEXT,
    ///     },
    /// ];
    /// let table = match db.create_table("table_test", columns) {
    ///     Ok(t) => t,
    ///     Err(e) => {
    ///         println!("create table err {}", e);
    ///     }
    /// };
    pub fn create_table(
        &self,
        table_name: &str,
        columns: &[ColumnInfo],
    ) -> Result<Table, SqliteErrCode> {
        let mut sql = format!("CREATE TABLE {}(", table_name);
        for i in 0..columns.len() {
            let column = &columns[i];
            sql.push_str(column.name);
            sql.push(' ');
            sql.push_str(from_datatype_to_str(column.data_type));
            if column.is_primary_key {
                sql.push_str(" PRIMARY KEY");
            }
            if column.not_null {
                sql.push_str(" NOT NULL");
            }
            if i != columns.len() - 1 {
                sql.push(',')
            };
        }
        sql.push_str(");");
        asset_common::loge!("{}", sql);
        let stmt = Statement::<false>::new(sql.as_str(), self);
        let ret = stmt.exec(None, 0);
        if ret != SQLITE_OK {
            asset_common::loge!("exec create table fail {}", ret);
            return Err(ret);
        }
        Ok(Table::new(table_name, self))
    }

    #[cfg(test)]
    pub fn drop_db(db: Database) -> std::io::Result<()> {
        let path = db.path.clone();
        let b_path = db.back_path.clone();
        drop(db);
        let ret = Database::drop_database(path.as_str());
        let b_ret = Database::drop_database(b_path.as_str());
        ret?;
        b_ret?;
        Ok(())
    }
}

/// wrap close func
pub(crate) fn sqlite3_close_wrap(v2: bool, handle: usize) -> SqliteErrCode {
    if v2 {
        sqlite3_close_v2_func(handle)
    } else {
        sqlite3_close_func(handle)
    }
}

impl<'a> Drop for Database<'a> {
    fn drop(&mut self) {
        if self.handle != 0 {
            let ret = sqlite3_close_wrap(self.v2, self.handle);
            if ret != SQLITE_OK {
                asset_common::loge!("close db fail ret {}", ret);
            }
        }
        if self.backup_handle != 0 {
            let back_ret = sqlite3_close_wrap(self.v2, self.backup_handle);
            if back_ret != SQLITE_OK {
                asset_common::loge!("close back db fail ret {}", back_ret);
            }
        }
    }
}
