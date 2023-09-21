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
use std::{ffi::CStr, fs, ptr::null_mut, sync::Mutex};

use super::*;
use crate::{
    statement::Statement,
    table::Table,
    types::{from_datatype_to_str, ColumnInfo, Sqlite3ErrMsg},
};

/// each user have a Database file
pub struct UseridFileLock {
    /// userid
    pub(crate) userid: u32,
    /// file lock
    pub(crate) mtx: Mutex<u32>,
}

/// save all the userid file locks
static G_USER_FILE_LOCK_LIST: Mutex<Vec<&'static UseridFileLock>> = Mutex::new(Vec::new());

/// if userid exists, return reference, or create a new lock, insert into list and return reference
fn get_file_lock_by_userid(userid: u32) -> &'static UseridFileLock {
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
    /// database file path
    pub(crate) path: String,
    /// is opened with sqlite3_open_v2
    pub(crate) v2: bool,
    /// raw pointer
    pub(crate) handle: usize,
    /// db file
    pub(crate) file: &'a UseridFileLock,
}

/// update func callback
pub type UpdateDatabaseCallbackFunc =
    fn(db: &Database, old_ver: u32, new_ver: u32) -> SqliteErrCode;

/// default callback func for update database
pub fn default_update_database_func(db: &Database, old_ver: u32, new_ver: u32) -> SqliteErrCode {
    if new_ver > old_ver {
        // TODO do something
        println!("database {} update from ver {} to {}", db.path, old_ver, new_ver);
        return db.update_version(new_ver as _);
    }
    if new_ver < old_ver {
        println!("database version rollback is not supported!");
        return SQLITE_ERROR;
    }
    SQLITE_OK
}

/// format database path
#[inline(always)]
pub fn fmt_db_path(userid: u32) -> String {
    format!("/data/service/el1/public/asset_service/{}/asset.db", userid)
}

impl<'a> Database<'a> {
    /// open database file.
    /// will create it if not exits.
    pub fn new(path: &str) -> Result<Database<'a>, SqliteErrCode> {
        let mut s = path.to_string();
        let mut db = Database {
            path: s.clone(),
            v2: false,
            handle: 0,
            file: get_file_lock_by_userid(u32::MAX),
        };
        s.push('\0');
        let _lock = db.file.mtx.lock().unwrap();
        let ret = sqlite3_open_func(&s, &mut db.handle);
        if ret == SQLITE_OK {
            Ok(db)
        } else {
            Err(ret)
        }
    }

    /// create default database
    pub fn default_new(userid: u32) -> Result<Database<'a>, SqliteErrCode> {
        let mut path = fmt_db_path(userid);
        let mut db = Database {
            path: path.clone(),
            v2: false,
            handle: 0,
            file: get_file_lock_by_userid(userid),
        };
        path.push('\0');
        let _lock = db.file.mtx.lock().unwrap();
        let ret = sqlite3_open_func(&path, &mut db.handle);
        if ret == SQLITE_OK {
            Ok(db)
        } else {
            Err(ret)
        }
    }

    /// get database user_version
    pub fn get_version(&self) -> Result<u32, SqliteErrCode> {
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
        let _lock = db.file.mtx.lock().unwrap();
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
        userid: u32,
        ver: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database<'a>, SqliteErrCode> {
        let db = Database::default_new(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
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
        vfs: Option<&[u8]>,
    ) -> Result<Database<'a>, SqliteErrCode> {
        let mut s = path.to_string();
        let mut db = Database {
            path: s.clone(),
            v2: true,
            handle: 0,
            file: get_file_lock_by_userid(u32::MAX),
        };
        s.push('\0');
        let _lock = db.file.mtx.lock().unwrap();
        let ret = sqlite3_open_v2_func(&s, &mut db.handle, flags, vfs);
        if ret == SQLITE_OK {
            Ok(db)
        } else {
            Err(ret)
        }
    }

    /// delete database with delete the file
    pub fn drop_database(path: &str) -> std::io::Result<()> {
        let name = String::from_utf8(path.as_bytes().to_vec()).unwrap();
        let name = name.trim_matches(char::from(0));
        fs::remove_file(name)
    }

    /// delete default database
    pub fn drop_default_database(userid: u32) -> std::io::Result<()> {
        let path = fmt_db_path(userid);
        Database::drop_database(path.as_str())
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
            println!("exec fail error msg: {}", s.to_str().unwrap());
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

    /// open a table, if the table not exists, return error
    pub fn open_table(&self, table_name: &str) -> Result<Table, SqliteErrCode> {
        let sql =
            format!("select * from sqlite_master where type ='table' and name = '{}'", table_name);
        let stmt = match Statement::<true>::prepare(sql.as_str(), self) {
            Ok(o) => o,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = sqlite3_err_msg_func(self.handle);
                    if !msg.is_null() {
                        self.print_err_msg(msg);
                        sqlite3_free_func(msg as _);
                    }
                }
                return Err(e);
            },
        };
        let ret = stmt.step();
        if ret != SQLITE_ROW {
            Err(ret)
        } else {
            Ok(Table::new(table_name, self))
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
        #[cfg(test)]
        {
            println!("{}", sql);
        }
        let stmt = Statement::<false>::new(sql.as_str(), self);
        let ret = stmt.exec(None, 0);
        if ret != SQLITE_OK {
            println!("exec create table fail {}", ret);
            return Err(ret);
        }
        Ok(Table::new(table_name, self))
    }

    #[cfg(test)]
    pub fn drop_db(db: Database) -> std::io::Result<()> {
        let path = db.path.clone();
        drop(db);
        Database::drop_database(path.as_str())
    }
}

impl<'a> Drop for Database<'a> {
    fn drop(&mut self) {
        let ret = if self.v2 {
            sqlite3_close_v2_func(self.handle)
        } else {
            sqlite3_close_func(self.handle)
        };
        if ret != SQLITE_OK {
            println!("close db fail ret {}", ret);
        }
    }
}
