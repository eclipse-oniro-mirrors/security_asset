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

//! the interfaces of db_operator public for other module
//! including transaction and create default db,table

use asset_definition::{ErrCode, Value};
use asset_log::{loge, logi};

use crate::{
    database::{
        copy_db_file, copy_db_file_inner, is_db_corrupt, sqlite3_close_wrap, Database, UpdateDatabaseCallbackFunc,
    },
    table::Table,
    transaction::Transaction,
    types::{
        Condition, DbMap, QueryOptions, ResultSet, SqliteErrCode, ASSET_TABLE_NAME, COLUMN_INFO, SQLITE_DONE, SQLITE_OK,
    },
};

/// just use database
pub struct DatabaseHelper;
/// just use table
pub type TableHelper<'a> = Table<'a>;

/// change sqlite err code to asset err code
fn from_sqlite_code_to_asset_code(value: SqliteErrCode) -> ErrCode {
    if value != SQLITE_OK && value != SQLITE_DONE {
        asset_log::loge!("error ret {}", value);
    }
    ErrCode::DatabaseError
}

/// do same operation in backup database when do something in master db
/// backup every success operation, recovery every fail operation
fn back_db_when_succ<T, F: Fn(&Table) -> Result<T, SqliteErrCode>>(
    modified: bool,
    table: &Table,
    func: F,
) -> Result<T, ErrCode> {
    let ret = func(table);
    match ret {
        Ok(o) => {
            if modified {
                // let _ = thread::spawn(move || {
                let back_ret = copy_db_file_inner(&table.db.path, &table.db.back_path);
                if back_ret.is_err() {
                    loge!("backup db {} fail", table.db.back_path);
                } else {
                    logi!("backup db {} succ", table.db.back_path);
                }
                //});
            }
            Ok(o)
        },
        Err(e) => {
            if is_db_corrupt(e) {
                // recovery master db
                let r_ret = copy_db_file(table.db, true);
                if r_ret.is_err() {
                    loge!("recovery master db {} fail", table.db.path);
                    Err(ErrCode::DatabaseError)
                } else {
                    logi!("recovery master db {} succ", table.db.path);

                    let res = func(table);
                    process_err_msg(res.map_err(from_sqlite_code_to_asset_code), table.db)
                }
            } else {
                process_err_msg(Err(from_sqlite_code_to_asset_code(e)), table.db)
            }
        },
    }
}

impl<'a> TableHelper<'a> {
    /// update datas in asset db table.
    /// owner and alias is the primary-key for resources.
    /// the datas is a map of column-data pair.
    /// if success, return line changes.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use asset_db_operator::types::DbMap;
    ///
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let datas = DbMap::from([("value", Value::Bytes(b"test_update".to_vec()))]);
    /// let ret = helper.update_datas(&DbMap::new(), &datas);
    /// ```
    /// sql like:
    /// update table_name set alias='test_update' where AppId='owner' and Alias='alias'
    pub fn update_datas(&self, condition: &Condition, datas: &DbMap) -> Result<i32, ErrCode> {
        let closure = |e: &Table| e.update_row(condition, datas);
        back_db_when_succ(true, self, closure)
    }

    /// insert datas into asset db table.
    /// owner and alias is the primary-key for resources.
    /// the datas is a map of column-data pair.
    /// if success, return line changes.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use asset_db_operator::types::DbMap;
    ///
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let datas = DbMap::from([("value", Value::Bytes(b"test_update".to_vec()))]);
    ///
    /// let ret = helper.insert_datas(&datas);
    /// ```
    ///
    /// sql like:
    /// insert into table_name(Owner,Alias,value) values(owner,alias,'test_update')
    pub fn insert_datas(&self, datas: &DbMap) -> Result<i32, ErrCode> {
        let closure = |e: &Table| e.insert_row(datas);
        back_db_when_succ(true, self, closure)
    }

    /// insert multi datas
    /// columns: the columns
    /// datas: the data set
    pub fn insert_multi_datas(&self, columns: &Vec<&'static str>, datas: &Vec<Vec<Value>>) -> Result<i32, ErrCode> {
        let closure = |e: &Table| e.insert_multi_row_datas(columns, datas);
        back_db_when_succ(true, self, closure)
    }

    /// delete datas from asset db table.
    /// owner and alias is the primary-key for resources.
    /// the cond is a map of column-data pair.
    /// if success, return line changes.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use asset_db_operator::types::DbMap;
    ///
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let cond = DbMap::from([("value", Value::Bytes(b"test_delete".to_vec()))]);
    ///
    /// let ret = helper.delete_datas(&cond);
    /// ```
    ///
    /// sql like:
    /// delete from table_name where Owner=owner and Alias=alias and value='test_update'
    pub fn delete_datas(&self, condition: &Condition) -> Result<i32, ErrCode> {
        let closure = |e: &Table| e.delete_row(condition);
        back_db_when_succ(true, self, closure)
    }

    /// return if data exists.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_definition::Value;
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let exist = helper.is_data_exists(&HashMap::<&'static str, Value>::new());
    /// ```
    ///
    /// sql like:
    /// select count(*) as count from table_name where Owner='owner1' and Alias='alias1'
    pub fn is_data_exist(&self, condition: &Condition) -> Result<bool, ErrCode> {
        let closure = |e: &Table| e.is_data_exists(condition);
        back_db_when_succ(false, self, closure)
    }

    /// return select count for owner.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_definition::Value;
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let count = helper.select_count(&HashMap::<&'static str, Value>::new());
    /// ```
    /// sql like:
    /// select count(*) as count from table_name where AppId='owner2'
    pub fn select_count(&self, condition: &Condition) -> Result<u32, ErrCode> {
        let closure = |e: &Table| e.count_datas(condition);
        back_db_when_succ(false, self, closure)
    }

    /// query all datas for owner and alias with condition(condition could be empty).
    /// if success, return result set.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_definition::Value;
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let result = helper.query_datas(&HashMap::<&'static str, Value>::new(), None);
    /// ```
    /// sql like:
    /// select * from table_name where AppId='owner' and Alias='alias'
    pub fn query_datas(
        &self,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<ResultSet, ErrCode> {
        let closure = |e: &Table| e.query_row(&vec![], condition, query_options);
        back_db_when_succ(false, self, closure)
    }

    /// query special columns with condition(condition could be empty).
    /// if success, return result set.
    /// if fail, return err code.
    ///
    /// # Example
    /// ```
    /// use asset_db_operator::database_table_helper::DatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_definition::Value;
    /// let helper = DatabaseHelper::open_default_database_table(1).unwrap();
    /// let result = helper.query_columns(&vec![], &HashMap::<&'static str, Value>::new(), None);
    /// ```
    /// sql like:
    /// select * from table_name where AppId='owner' and Alias='alias'
    pub fn query_columns(
        &self,
        columns: &Vec<&'static str>,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>, ErrCode> {
        let closure = |e: &Table| e.query_datas_advanced(columns, condition, query_options, COLUMN_INFO);
        back_db_when_succ(false, self, closure)
    }
}

/// process err msg, this may be use in test, consider delete this function when release
pub fn process_err_msg<T>(res: Result<T, ErrCode>, db: &Database) -> Result<T, ErrCode> {
    if res.is_err() {
        if let Some(msg) = db.get_err_msg() {
            loge!("db err info: {}", msg.s);
        } else {
            loge!("db err with no msg");
        }
    }
    res
}

/// if table not exist, create default asset table
#[inline(always)]
fn create_default_table<'a>(db: &'a Database) -> Result<Table<'a>, ErrCode> {
    let res = db.create_table(ASSET_TABLE_NAME, COLUMN_INFO).map_err(from_sqlite_code_to_asset_code);
    match process_err_msg(res, db) {
        Ok(table) => {
            let closure = |_e: &Table| Ok(());
            let _ = back_db_when_succ(true, &table, closure);
            Ok(table)
        },
        Err(e) => Err(e),
    }
}

/// open default table
fn open_default_table<'a>(db: &'a mut Database) -> Result<Option<Table<'a>>, ErrCode> {
    let res = db.open_table(ASSET_TABLE_NAME);
    match res {
        Ok(o) => {
            if o.is_none() {
                return Ok(None);
            }
            return Ok(Some(Table::new(ASSET_TABLE_NAME, db)));
        },
        Err(e) => {
            if is_db_corrupt(e) {
                let _ = sqlite3_close_wrap(db.handle);
                // recovery master db
                let r_ret = copy_db_file(db, true);
                if r_ret.is_err() {
                    loge!("recovery master db {} fail", db.path);
                    Err(ErrCode::DatabaseError)
                } else {
                    logi!("recovery master db {} succ", db.path);
                    let o_ret = db.re_open();
                    if let Err(e) = o_ret {
                        loge!("reopen master db {} fail {}", db.path, e);
                        return Err(ErrCode::DatabaseError);
                    }
                    process_err_msg(db.open_table(ASSET_TABLE_NAME).map_err(from_sqlite_code_to_asset_code), db)
                }
            } else {
                process_err_msg(Err(from_sqlite_code_to_asset_code(e)), db)
            }
        },
    }
}

impl<'a> Database<'a> {
    /// see TableHelper
    #[inline(always)]
    pub fn update_datas(&self, condition: &Condition, datas: &DbMap) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.update_datas(condition, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_datas(&self, datas: &DbMap) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.insert_datas(datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_multi_datas(&self, columns: &Vec<&'static str>, datas: &Vec<Vec<Value>>) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.insert_multi_datas(columns, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn delete_datas(&self, cond: &Condition) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.delete_datas(cond)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn is_data_exists(&self, condition: &Condition) -> Result<bool, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.is_data_exist(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn select_count(&self, condition: &Condition) -> Result<u32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.select_count(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_datas(
        &self,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<ResultSet, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.query_datas(condition, query_options)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_columns(
        &self,
        columns: &Vec<&'static str>,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.query_columns(columns, condition, query_options)
    }
}

impl DatabaseHelper {
    /// open default database and table
    pub fn open_default_database_table<'a>(user_id: i32) -> Result<Database<'a>, ErrCode> {
        let mut db = Database::default_new(user_id).map_err(from_sqlite_code_to_asset_code)?;
        let _lock = db.file.mtx.lock().unwrap();
        match open_default_table(&mut db) {
            Ok(o) => {
                if o.is_none() {
                    create_default_table(&db)?;
                }
            },
            Err(_) => {
                create_default_table(&db)?;
            },
        };
        Ok(db)
    }

    /// open default database and table, if need update db version, input callback
    pub fn open_default_database_table_with_version_update<'a>(
        user_id: i32,
        version_new: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<Database<'a>, ErrCode> {
        let mut db = Database::default_new_with_version_update(user_id, version_new, callback)
            .map_err(from_sqlite_code_to_asset_code)?;
        let _lock = db.file.mtx.lock().unwrap();
        match open_default_table(&mut db) {
            Ok(o) => {
                if o.is_none() {
                    create_default_table(&db)?;
                }
            },
            Err(_) => {
                create_default_table(&db)?;
            },
        };
        Ok(db)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn update_datas(user_id: i32, condition: &Condition, datas: &DbMap) -> Result<i32, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.update_datas(condition, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_datas(user_id: i32, datas: &DbMap) -> Result<i32, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.insert_datas(datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_multi_datas(
        user_id: i32,
        columns: &Vec<&'static str>,
        datas: &Vec<Vec<Value>>,
    ) -> Result<i32, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.insert_multi_datas(columns, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn delete_datas(user_id: i32, cond: &Condition) -> Result<i32, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.delete_datas(cond)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn is_data_exists(user_id: i32, condition: &Condition) -> Result<bool, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.is_data_exists(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn select_count(user_id: i32, condition: &Condition) -> Result<u32, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.select_count(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_datas(
        user_id: i32,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<ResultSet, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.query_datas(condition, query_options)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_columns(
        user_id: i32,
        columns: &Vec<&'static str>,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>, ErrCode> {
        let db = DatabaseHelper::open_default_database_table(user_id)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.query_columns(columns, condition, query_options)
    }
}

/// transaction callback func, do NOT lock database in this callback function
/// return true if want to commit, false if want to rollback
/// the func can be a closure or a function like this:
/// pub type TransactionCallback = fn(db: &Database) -> bool;
///
/// do transaction
/// if commit, return true
/// if rollback, return false
pub fn do_transaction<F: Fn(&Database) -> bool>(user_id: i32, callback: F) -> Result<bool, ErrCode> {
    let db = match DatabaseHelper::open_default_database_table(user_id) {
        Ok(o) => o,
        Err(e) => {
            loge!("transaction open db fail");
            return Err(e);
        },
    };

    let mut trans = Transaction::new(&db);
    let _lock = db.file.mtx.lock().unwrap();
    let ret = trans.begin();
    if ret != 0 {
        return Err(from_sqlite_code_to_asset_code(ret));
    }
    if callback(&db) {
        let ret = trans.commit();
        if ret != SQLITE_OK {
            loge!("trans commit fail {}", ret);
            return Err(from_sqlite_code_to_asset_code(ret));
        }
        Ok(true)
    } else {
        let ret = trans.rollback();
        if ret != SQLITE_OK {
            loge!("trans rollback fail {}", ret);
            return Err(from_sqlite_code_to_asset_code(ret));
        }
        Ok(false)
    }
}
