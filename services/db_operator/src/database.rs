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

//! impl for db
//! each user have a db, each db have a db file and a lock, the lock is mutex
//! db link is auto drop by RAII

use core::ffi::c_void;
use std::{ffi::CStr, fs, path::Path, ptr::null_mut, sync::Mutex};

use crate::{
    statement::Statement,
    table::Table,
    types::{
        from_data_type_to_str, ColumnInfo, Sqlite3Callback, Sqlite3ErrMsg, SqliteErrCode, SQLITE_DONE, SQLITE_ERROR,
        SQLITE_OK, SQLITE_ROW,
    },
};
#[cfg(test)]
use asset_definition::DataType;
#[cfg(test)]
use core::panic;

extern "C" {
    fn SqliteOpen(file_name: *const u8, pp_db: *mut *mut c_void) -> i32;
    fn SqliteCloseV2(db: *mut c_void) -> i32;
    fn SqliteExec(
        db: *mut c_void,
        sql: *const u8,
        callback: Option<Sqlite3Callback>,
        data: *mut c_void,
        msg: *mut *mut u8,
    ) -> i32;
    fn SqliteFree(data: *mut c_void);
    fn SqliteErrMsg(db: *mut c_void) -> *const u8;
}

const SQLITE_CORRUPT: i32 = 11;
const SQLITE_NOTADB: i32 = 26;

/// each user have a Database file
pub struct UserIdFileLock {
    /// user_id
    pub(crate) user_id: i32,
    /// file lock
    pub(crate) mtx: Mutex<i32>,
}

/// save all the user_id file locks
static G_USER_DB_LOCK_LIST: Mutex<Vec<&'static UserIdFileLock>> = Mutex::new(Vec::new());

/// if user_id exists, return reference, or create a new lock, insert into list and return reference
fn get_file_lock_by_user_id(user_id: i32) -> &'static UserIdFileLock {
    let mut list = G_USER_DB_LOCK_LIST.lock().unwrap();
    for f in list.iter() {
        if f.user_id == user_id {
            return f;
        }
    }
    let nf = Box::new(UserIdFileLock { user_id, mtx: Mutex::new(user_id) });
    // SAFETY: We just push item into G_USER_FILE_LOCK_LIST, never remove item or modify item,
    // so return a reference of leak item is safe.
    let nf: &'static UserIdFileLock = Box::leak(nf);
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
    /// raw pointer
    pub(crate) handle: usize,
    /// db file
    pub(crate) file: &'a UserIdFileLock,
}

/// update func callback
pub type UpdateDatabaseCallbackFunc = fn(db: &Database, old_ver: u32, new_ver: u32) -> SqliteErrCode;

/// default callback func for update database
pub fn default_update_database_func(db: &Database, old_ver: u32, new_ver: u32) -> SqliteErrCode {
    if new_ver > old_ver {
        // do something
        asset_log::logi!("database {} update from ver {} to {}", db.path, old_ver, new_ver);
        return db.update_version(new_ver as _);
    }
    if new_ver < old_ver {
        asset_log::loge!("database version rollback is not supported!");
        return SQLITE_ERROR;
    }
    SQLITE_OK
}

/// format database path
#[inline(always)]
fn fmt_db_path(user_id: i32) -> String {
    #[cfg(test)]
    return format!("/data/asset_test/{}/asset.db", user_id);
    #[cfg(not(test))]
    format!("/data/service/el1/public/asset_service/{}/asset.db", user_id)
}

/// get backup path
#[inline(always)]
fn fmt_backup_path(path: &str) -> String {
    let mut bp = path.to_string();
    bp.push_str(".backup");
    bp
}

/// recovery if database file format error
/// if recovery_master == false, will recovery backup file
#[inline(always)]
pub fn copy_db_file_inner(from: &String, to: &String) -> Result<u64, std::io::Error> {
    fs::copy(from, to)
}

/// recovery if database file format error
/// if master_or_backup == false, will recovery backup file
/// if master_or_backup == true, will recovery master file
pub fn copy_db_file(db: &Database, master_or_backup: bool) -> Result<u64, std::io::Error> {
    if master_or_backup {
        fs::copy(&db.back_path, &db.path)
    } else {
        fs::copy(&db.path, &db.back_path)
    }
}

/// wrap sqlite open
#[inline(always)]
fn sqlite3_open_wrap(file: &str, handle: &mut usize) -> SqliteErrCode {
    unsafe { SqliteOpen(file.as_ptr(), handle as *mut usize as _) }
}

/// is corrupt
#[inline(always)]
pub fn is_db_corrupt(ret: SqliteErrCode) -> bool {
    ret == SQLITE_CORRUPT || ret == SQLITE_NOTADB
}

/// open db, will recovery wrong db file
fn open_db(db: &mut Database, path: String, back_path: String) -> Result<(), SqliteErrCode> {
    let _lock = db.file.mtx.lock().unwrap();
    let mut ret = sqlite3_open_wrap(&path, &mut db.handle);
    if is_db_corrupt(ret) {
        // recovery master db
        let mut back_handle = 0usize;
        let back_ret = sqlite3_open_wrap(&back_path, &mut back_handle);
        if back_ret != SQLITE_OK {
            asset_log::loge!("both master backup db fail: {} {} {}", path, ret, back_ret);
            return Err(ret);
        }
        let close_ret = sqlite3_close_wrap(back_handle);
        if close_ret != SQLITE_OK {
            asset_log::loge!("close back fail {}", close_ret);
        }
        let r_ret = copy_db_file(db, true);
        if r_ret.is_err() {
            asset_log::loge!("recovery master db {} fail", path);
            return Err(ret);
        }
        asset_log::logi!("recovery master db {} succ", path);
        ret = sqlite3_open_wrap(&path, &mut db.handle);
        if ret != SQLITE_OK {
            asset_log::loge!("reopen master db {} fail {}", path, ret);
            return Err(ret);
        }
        Ok(())
    } else if ret == SQLITE_OK {
        Ok(())
    } else {
        Err(ret)
    }
}

impl<'a> Database<'a> {
    /// reopen db file
    pub(crate) fn re_open(&mut self) -> Result<(), SqliteErrCode> {
        if self.handle != 0 {
            self.handle = 0;
        }
        let mut path_c = self.path.clone();
        path_c.push('\0');
        let ret = sqlite3_open_wrap(&path_c, &mut self.handle);
        if ret != SQLITE_OK {
            asset_log::loge!("reopen handle {} fail {}", self.path, ret);
            return Err(ret);
        }
        Ok(())
    }

    /// open database file.
    /// will create it if not exits.
    #[cfg(test)]
    pub(crate) fn new(path: &str) -> Result<Database, SqliteErrCode> {
        let mut path_c = path.to_string();
        let mut back_path_c = fmt_backup_path(path);
        let mut db: Database<'_> = Database {
            // user - mutex
            path: path_c.clone(),
            back_path: back_path_c.clone(),
            handle: 0,
            file: get_file_lock_by_user_id(i32::MAX),
        };
        path_c.push('\0');
        back_path_c.push('\0');
        open_db(&mut db, path_c, back_path_c)?;
        Ok(db)
    }

    /// create default database
    pub(crate) fn default_new(user_id: i32) -> Result<Database<'a>, SqliteErrCode> {
        let path = fmt_db_path(user_id);
        let mut path_c = path.clone();
        let mut back_path_c = fmt_backup_path(path.as_str());
        let mut db = Database {
            path: path_c.clone(),
            back_path: back_path_c.clone(),
            handle: 0,
            file: get_file_lock_by_user_id(user_id),
        };
        path_c.push('\0');
        back_path_c.push('\0');
        open_db(&mut db, path_c, back_path_c)?;
        Ok(db)
    }

    /// get database user_version
    pub(crate) fn get_version(&self) -> Result<u32, SqliteErrCode> {
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
    #[cfg(test)]
    pub(crate) fn new_with_version_update(
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
    pub(crate) fn default_new_with_version_update(
        user_id: i32,
        ver: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database<'a>, SqliteErrCode> {
        let db = Database::default_new(user_id)?;
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

    /// delete database with delete the file
    pub fn drop_database(path: &str) -> std::io::Result<()> {
        let name = String::from(path);
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
    pub fn drop_default_database(user_id: i32) -> std::io::Result<()> {
        let path = fmt_db_path(user_id);
        Database::drop_database(path.as_str())
    }

    /// delete default database and backup db
    pub fn drop_default_database_and_backup(user_id: i32) -> std::io::Result<()> {
        let path = fmt_db_path(user_id);
        let back_path = fmt_backup_path(path.as_str());
        let ret = Database::drop_database(path.as_str());
        let back_ret = Database::drop_database(back_path.as_str());
        ret?;
        back_ret
    }

    /// return err msg if get error.
    /// return None if no error.
    /// You do NOT need to free err msg, it's auto freed.
    pub(crate) fn get_err_msg(&self) -> Option<Sqlite3ErrMsg> {
        let msg = unsafe { SqliteErrMsg(self.handle as _) };
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            if let Ok(rs) = s.to_str() {
                let se = Sqlite3ErrMsg { s: rs, db: self };
                return Some(se);
            } else {
                return None;
            }
        }
        None
    }

    pub(crate) fn print_err_msg(&self, msg: *const u8) {
        unsafe {
            let s = CStr::from_ptr(msg as _);
            if let Ok(rs) = s.to_str() {
                asset_log::loge!("exec fail error msg: {}", rs);
            } else {
                asset_log::loge!("exec fail error msg: none");
            }
        }
    }

    /// execute sql without prepare.
    /// you should use statement.step for prepared statement.
    /// callback function for process result set.
    /// the final param data will be passed into callback function.
    pub(crate) fn exec(
        &self,
        stmt: &Statement<false>,
        callback: Option<Sqlite3Callback>,
        data: usize,
    ) -> SqliteErrCode {
        let mut msg: *mut u8 = null_mut();
        let ret = unsafe { SqliteExec(self.handle as _, stmt.sql.as_ptr(), callback, data as _, &mut msg as _) };
        if !msg.is_null() {
            self.print_err_msg(msg);
            unsafe { SqliteFree(msg as _) };
            return SQLITE_ERROR;
        }
        ret
    }

    /// set database version
    pub(crate) fn update_version(&self, ver: u32) -> SqliteErrCode {
        let sql = format!("pragma user_version = {}", ver);
        let statement = Statement::new(sql.as_str(), self);
        statement.exec(None, 0)
    }

    /// open a table, if the table not exists, return Ok(None)
    pub(crate) fn open_table(&self, table_name: &str) -> Result<Option<Table>, SqliteErrCode> {
        let sql = format!("select * from sqlite_master where type ='table' and name = '{}'", table_name);
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
    /// name, is_primary_key, is_not_null, data_type
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
    ///         data_type: DataType::BLOB,
    ///     },
    /// ];
    /// let table = match db.create_table("table_test", columns) {
    ///     Ok(t) => t,
    ///     Err(e) => {
    ///         println!("create table err {}", e);
    ///     }
    /// };
    pub(crate) fn create_table(&self, table_name: &str, columns: &[ColumnInfo]) -> Result<Table, SqliteErrCode> {
        let mut sql = format!("CREATE TABLE {}(", table_name);
        for i in 0..columns.len() {
            let column = &columns[i];
            sql.push_str(column.name);
            sql.push(' ');
            sql.push_str(from_data_type_to_str(&column.data_type));
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
        let stmt = Statement::<false>::new(sql.as_str(), self);
        let ret = stmt.exec(None, 0);
        if ret != SQLITE_OK {
            asset_log::loge!("exec create table fail {}", ret);
            return Err(ret);
        }
        Ok(Table::new(table_name, self))
    }

    #[cfg(test)]
    pub(crate) fn drop_db(db: Database) -> std::io::Result<()> {
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
pub(crate) fn sqlite3_close_wrap(handle: usize) -> SqliteErrCode {
    unsafe { SqliteCloseV2(handle as _) }
}

impl<'a> Drop for Database<'a> {
    fn drop(&mut self) {
        if self.handle != 0 {
            let ret = sqlite3_close_wrap(self.handle);
            if ret != SQLITE_OK {
                asset_log::loge!("close db fail ret {}", ret);
            }
        }
    }
}

#[test]
pub fn test_for_sqlite3_open() {
    let _ = match Database::new("test.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    match Database::new("/root/test.db") {
        Ok(_) => {
            panic!("read root");
        },
        Err(ret) => {
            println!("expected fault {}", ret);
        },
    };
    let _ = fs::create_dir("db");
}

#[test]
pub fn test_for_drop_database() {
    let _ = match Database::new("test1.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    let _ = Database::drop_database("test1.db");
}

#[test]
pub fn test_for_update_version() {
    let db = match Database::new("test0.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    if db.update_version(1) != SQLITE_OK {
        panic!("update version fail");
    }
}

#[test]
pub fn test_for_error_exec() {
    let db = match Database::new("test1.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    let sql = "pragma zzz user_version = {} mmm";
    let statement = Statement::new(sql, &db);
    let ret = statement.exec(None, 0);
    assert_ne!(ret, 0);
}

#[test]
pub fn test_for_open_table() {
    let db = match Database::new("test3.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };
    let table = db.open_table("table_name");
    match table {
        Ok(_o) => {
            println!("open table succ");
        },
        Err(e) => {
            panic!("expect open table fail {}", e);
        },
    }

    let _ = Database::drop_database("test4.db");

    let db = match Database::new("test4.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let _table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let _ = match db.open_table("table_test") {
        Ok(o) => {
            println!("open table succ");
            o
        },
        Err(e) => {
            panic!("open table fail {}", e)
        },
    };
}

#[test]
pub fn test_for_drop_table() {
    let db = match Database::new("test5.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let _table = match db.create_table("table_test", columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };

    let ret = db.drop_table("table_test");
    assert_eq!(ret, SQLITE_OK);
}

#[test]
pub fn test_for_special_sql() {
    let _ = Database::drop_database("test19.db");
    let db = match Database::new("test19.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![Value::Bytes(b"owner1".to_vec()), Value::Bytes(b"alias1".to_vec()), Value::Bytes(b"aaaa".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias2".to_vec()), Value::Bytes(b"bbbb".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias3".to_vec()), Value::Bytes(b"cccc".to_vec())],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    let sql = "select Owner,Alias from asset_table where Id>?";
    let stmt = Statement::<true>::prepare(sql, &db).unwrap();
    let ret = stmt.bind_data(1, &Value::Number(1));
    assert_eq!(ret, SQLITE_OK);

    while stmt.step() == SQLITE_ROW {
        print!("line: ");
        let owner = stmt.query_column_text(0);
        let alias = stmt.query_column_text(1);
        print!(
            "{} {}",
            from_data_value_to_str_value(&Value::Bytes(owner.to_vec())),
            from_data_value_to_str_value(&Value::Bytes(alias.to_vec()))
        );
        println!();
    }
}

#[test]
pub fn test_for_update_ver() {
    let _ = Database::drop_database("test20.db");
    let db = Database::new("test20.db").unwrap();
    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let _ = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    drop(db);
    let db2 = Database::new_with_version_update("test20.db", 0, default_update_database_func).unwrap();
    drop(db2);

    let db3 = Database::new_with_version_update("test20.db", 1, default_update_database_func).unwrap();
    drop(db3);

    let db4 = Database::new_with_version_update("test20.db", 0, default_update_database_func);
    assert!(db4.is_err());
}

#[test]
pub fn test_for_recovery() {
    let db = Database::new("test111.db").unwrap();
    let table = db
        .create_table(
            "tt",
            &[ColumnInfo { name: "Id", data_type: DataType::Number, is_primary_key: true, not_null: true }],
        )
        .unwrap();
    let count = table.insert_row(&DbMap::from([("Id", Value::Number(1))])).unwrap();
    assert_eq!(count, 1);
    fs::copy("test111.db", "test111.db.backup").unwrap();
    fs::remove_file("test111.db").unwrap();
    fs::copy("test111.db.backup", "test111.db").unwrap();
    let count = table.count_datas(&DbMap::new()).unwrap();
    assert_eq!(count, 1);
    let _ = Database::drop_database_and_backup(db);
}

#[test]
pub fn test_db_fun() {
    let _ = Database::drop_database("test18.db");
    let db = match Database::new("test18.db") {
        Ok(o) => o,
        Err(ret) => {
            panic!("test sqlite3 open fail ret {}", ret);
        },
    };

    let columns = &[
        ColumnInfo { name: "Id", is_primary_key: true, not_null: true, data_type: DataType::Number },
        ColumnInfo { name: "Owner", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "Alias", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
        ColumnInfo { name: "value", is_primary_key: false, not_null: true, data_type: DataType::Bytes },
    ];
    let table = match db.create_table(ASSET_TABLE_NAME, columns) {
        Ok(t) => t,
        Err(e) => {
            panic!("create table err {}", e);
        },
    };
    let columns = &vec!["Owner", "Alias", "value"];
    let dataset = vec![
        vec![Value::Bytes(b"owner1".to_vec()), Value::Bytes(b"alias1".to_vec()), Value::Bytes(b"aaaa".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias2".to_vec()), Value::Bytes(b"bbbb".to_vec())],
        vec![Value::Bytes(b"owner2".to_vec()), Value::Bytes(b"alias3".to_vec()), Value::Bytes(b"cccc".to_vec())],
    ];
    let count = table.insert_multi_row_datas(columns, &dataset).unwrap();
    assert_eq!(count, 3);

    // query
    let exist = db
        .is_data_exists(&DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias1".to_vec())),
        ]))
        .unwrap();
    assert!(exist);

    let exist = db
        .is_data_exists(&DbMap::from([
            (column::OWNER, Value::Bytes(b"owner1".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias2".to_vec())),
        ]))
        .unwrap();
    assert!(!exist);

    db_fun(db);
}

#[cfg(test)]
fn db_fun(db: Database<'_>) {
    let count = db.select_count(&DbMap::from([(column::OWNER, Value::Bytes(b"owner2".to_vec()))])).unwrap();
    assert_eq!(count, 2);

    let ret = db
        .insert_datas(&DbMap::from([
            ("value", Value::Bytes(b"value4".to_vec())),
            (column::OWNER, Value::Bytes(b"owner4".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias4".to_vec())),
        ]))
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .update_datas(
            &DbMap::from([
                (column::OWNER, Value::Bytes(b"owner4".to_vec())),
                (column::ALIAS, Value::Bytes(b"alias4".to_vec())),
            ]),
            &DbMap::from([("value", Value::Bytes(b"value5".to_vec()))]),
        )
        .unwrap();
    assert_eq!(ret, 1);

    let ret = db
        .delete_datas(&DbMap::from([
            (column::OWNER, Value::Bytes(b"owner4".to_vec())),
            (column::ALIAS, Value::Bytes(b"alias4".to_vec())),
        ]))
        .unwrap();
    assert_eq!(ret, 1);

    let result = db
        .query_datas(
            &DbMap::from([
                (column::OWNER, Value::Bytes(b"owner1".to_vec())),
                (column::ALIAS, Value::Bytes(b"alias1".to_vec())),
            ]),
            None,
        )
        .unwrap();
    assert_eq!(result.len(), 1);
    for data in result {
        print!("line: ");
        for d in data {
            print!("{}, ", from_data_value_to_str_value(&d));
        }
        println!();
    }
}
