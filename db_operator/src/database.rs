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
use crate::{
    statement::Statement,
    table::Table,
    types::{from_datatype_to_str, ColumnInfo, Sqlite3Errmsg},
};

use super::*;
use std::{ffi::CStr, fs, ptr::null_mut};

/// sqlite database file
#[repr(C)]
pub struct Database<'a> {
    /// database file path
    pub path: &'a str,
    /// is opened with sqlite3_open_v2
    pub(crate) v2: bool,
    /// raw pointer
    pub(crate) handle: usize, // C pointer
}

/// update func callback
pub type UpdateDatabaseCallbackFunc =
    fn(db: &Database, old_ver: i32, new_ver: i32) -> SqliteErrcode;

/// default callback func for update database
pub fn default_update_database_func(db: &Database, old_ver: i32, new_ver: i32) -> SqliteErrcode {
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

impl<'a> Database<'a> {
    ///
    /// open database file.
    /// will create it if not exits.
    ///
    pub fn new(path: &str) -> Result<Database, SqliteErrcode> {
        let mut db = Database { path, v2: false, handle: 0 };
        let mut s = path.to_string();
        s.push('\0');
        let ret = sqlite3_open_func(&s, &mut db.handle);
        if ret == SQLITE_OK {
            Ok(db)
        } else {
            Err(ret)
        }
    }

    ///
    /// create default database
    ///
    pub fn default_new(userid: &str, el: &str) -> Result<Database<'a>, SqliteErrcode> {
        let mut path = format!("/data/service/{}/{}/asset_service/asset.db", el, userid);
        let mut db = Database { path: "-", v2: false, handle: 0 };
        path.push('\0');
        let ret = sqlite3_open_func(&path, &mut db.handle);
        if ret == SQLITE_OK {
            Ok(db)
        } else {
            Err(ret)
        }
    }

    ///
    /// get database user_version
    ///
    pub fn get_version(&self) -> Result<i32, SqliteErrcode> {
        let stmt = Statement::<true>::prepare("pragma user_version", self)?;
        let ret = stmt.step();
        if ret != SQLITE_ROW {
            return Err(ret);
        }
        let version_old = stmt.query_column_int(0);
        Ok(version_old)
    }

    ///
    /// open database with version update callback
    ///
    pub fn new_with_version_update(
        path: &str,
        ver: i32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database, SqliteErrcode> {
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

    ///
    /// open database with version update callback
    ///
    pub fn default_new_with_version_update(
        userid: &str,
        el: &str,
        ver: i32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database<'a>, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
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

    ///
    /// open database file
    /// use sqlite3_open_v2 instead of sqlite3_open
    ///
    pub fn new_v2<'b>(
        path: &'a str,
        flags: i32,
        vfs: Option<&'b [u8]>,
    ) -> Result<Database<'a>, SqliteErrcode> {
        let mut db = Database { path, v2: false, handle: 0 };
        let mut s = path.to_string();
        s.push('\0');
        let ret = sqlite3_open_v2_func(&s, &mut db.handle, flags, vfs);
        if ret == SQLITE_OK {
            Ok(db)
        } else {
            Err(ret)
        }
    }

    ///
    /// delete database whth delete the file
    ///
    pub fn drop_database(path: &str) -> std::io::Result<()> {
        let name = String::from_utf8(path.as_bytes().to_vec()).unwrap();
        let name = name.trim_matches(char::from(0));
        fs::remove_file(name)
    }

    ///
    /// delete default database
    ///
    pub fn drop_default_database(userid: &str, el: &str) -> std::io::Result<()> {
        let path = format!("/data/service/{}/{}/asset_service/asset.db", el, userid);
        Database::drop_database(path.as_str())
    }

    ///
    /// return errmsg if get error.
    /// return None if no error.
    /// You do NOT need to free errmsg, it's auto freed.
    ///
    pub fn get_errmsg(&self) -> Option<Sqlite3Errmsg> {
        let msg = sqlite3_errmsg_func(self.handle);
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            let se = Sqlite3Errmsg { s: s.to_str().unwrap(), db: self };
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

    ///
    /// execute sql without prepare.
    /// you should use statement.step for prepared statement.
    /// callback function for process result set.
    /// the final param data will be passed into callback function.
    ///
    pub fn exec(
        &self,
        stmt: &Statement<false>,
        callback: Option<sqlite3_callback>,
        data: usize,
    ) -> SqliteErrcode {
        let mut msg = null_mut();
        let ret = sqlite3_exec_func(self.handle, &stmt.sql, callback, data, &mut msg);
        if !msg.is_null() {
            self.print_err_msg(msg);
            sqlite3_free_func(msg as _);
            return SQLITE_ERROR;
        }
        ret
    }

    ///
    /// set database version
    ///
    pub fn update_version(&self, ver: u32) -> SqliteErrcode {
        let sql = format!("pragma user_version = {}", ver);
        let statement = Statement::new(sql.as_str(), self);
        statement.exec(None, 0)
    }

    ///
    /// open a table, if the table not exists, return error
    ///
    pub fn open_table<'b>(&'a self, table_name: &'b str) -> Result<Table<'b, 'a>, SqliteErrcode> {
        let sql =
            format!("select * from sqlite_master where type ='table' and name = '{}'", table_name);
        let stmt = match Statement::<true>::prepare(sql.as_str(), self) {
            Ok(o) => o,
            Err(e) => {
                #[cfg(test)]
                {
                    let msg = sqlite3_errmsg_func(self.handle);
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

    ///
    /// drop a table
    ///
    pub fn drop_table<'b>(&'a self, table_name: &'b str) -> SqliteErrcode {
        let sql = format!("DROP TABLE {}", table_name);
        let stmt = Statement::<false>::new(sql.as_str(), self);
        stmt.exec(None, 0)
    }

    ///
    /// create table with name 'table_name'
    /// the columns is decriptions for each column.
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
    ///
    pub fn create_table<'b>(
        &'a self,
        table_name: &'b str,
        columns: &[ColumnInfo],
    ) -> Result<Table<'b, 'a>, SqliteErrcode> {
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
        let path = db.path;
        drop(db);
        if path != "-" {
            return Database::drop_database(path);
        }
        Ok(())
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
