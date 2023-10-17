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

use asset_common::definition::{DataType, ErrCode, Value};

use crate::{
    database::{
        copy_db_file, copy_db_file_inner, is_db_corrupt, sqlite3_close_wrap, Database,
        UpdateDatabaseCallbackFunc,
    },
    from_sqlite_code_to_asset_code,
    table::Table,
    transaction::Transaction,
    types::{ColumnInfo, Condition, DbMap, QueryOptions, ResultSet},
    SqliteErrCode, SQLITE_OK,
};

/// just use database
pub type DatabaseHelper<'a> = Database<'a>;
/// just use database
pub type DefaultDatabaseHelper<'a> = Database<'a>;
/// just use table
pub type TableHelper<'a> = Table<'a>;

/// default table name
pub const ASSET_TABLE_NAME: &str = "asset_table";
/// default column name Id
pub const COLUMN_ID: &str = "Id";
/// default column name Secret
pub const COLUMN_SECRET: &str = "Secret";
/// default column name Alias
pub const COLUMN_ALIAS: &str = "Alias";
/// default column name Owner
pub const COLUMN_OWNER: &str = "Owner";
/// default column name OwnerType
pub const COLUMN_OWNER_TYPE: &str = "OwnerType";
/// default column name GroupId
pub const COLUMN_GROUP_ID: &str = "GroupId";
/// default column name SyncType
pub const COLUMN_SYNC_TYPE: &str = "SyncType";
/// default column name Accessibility
pub const COLUMN_ACCESSIBILITY: &str = "Accessibility";
/// default column name AuthType
pub const COLUMN_AUTH_TYPE: &str = "AuthType";
/// default column name CreateTime
pub const COLUMN_CREATE_TIME: &str = "CreateTime";
/// default column name UpdateTime
pub const COLUMN_UPDATE_TIME: &str = "UpdateTime";
/// default column name DeleteType
pub const COLUMN_DELETE_TYPE: &str = "DeleteType";
/// default column name Version
pub const COLUMN_VERSION: &str = "Version";
/// default column name RequirePasswordSet
pub const COLUMN_REQUIRE_PASSWORD_SET: &str = "RequirePasswordSet";
/// default column name DataLabelCritical_1
pub const COLUMN_CRITICAL1: &str = "DataLabelCritical_1";
/// default column name DataLabelCritical_2
pub const COLUMN_CRITICAL2: &str = "DataLabelCritical_2";
/// default column name DataLabelCritical_3
pub const COLUMN_CRITICAL3: &str = "DataLabelCritical_3";
/// default column name DataLabelCritical_4
pub const COLUMN_CRITICAL4: &str = "DataLabelCritical_4";
/// default column name DataLabelNormal_1
pub const COLUMN_NORMAL1: &str = "DataLabelNormal_1";
/// default column name DataLabelNormal_2
pub const COLUMN_NORMAL2: &str = "DataLabelNormal_2";
/// default column name DataLabelNormal_3
pub const COLUMN_NORMAL3: &str = "DataLabelNormal_3";
/// default column name DataLabelNormal_4
pub const COLUMN_NORMAL4: &str = "DataLabelNormal_4";

/// the columns for default asset table
pub const COLUMN_TABLE: [&str; 22] = [
    COLUMN_ID,
    COLUMN_SECRET,
    COLUMN_ALIAS,
    COLUMN_OWNER,
    COLUMN_OWNER_TYPE,
    COLUMN_GROUP_ID,
    COLUMN_SYNC_TYPE,
    COLUMN_ACCESSIBILITY,
    COLUMN_AUTH_TYPE,
    COLUMN_CREATE_TIME,
    COLUMN_UPDATE_TIME,
    COLUMN_DELETE_TYPE,
    COLUMN_VERSION,
    COLUMN_REQUIRE_PASSWORD_SET,
    COLUMN_CRITICAL1,
    COLUMN_CRITICAL2,
    COLUMN_CRITICAL3,
    COLUMN_CRITICAL4,
    COLUMN_NORMAL1,
    COLUMN_NORMAL2,
    COLUMN_NORMAL3,
    COLUMN_NORMAL4,
];

/// columns info for default asset_table
pub const COLUMN_INFO: &[ColumnInfo] = &[
    ColumnInfo {
        name: COLUMN_ID,
        data_type: DataType::Uint32,
        is_primary_key: true,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_SECRET,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_ALIAS,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_OWNER,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_OWNER_TYPE,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_GROUP_ID,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_SYNC_TYPE,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_ACCESSIBILITY,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_AUTH_TYPE,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_CREATE_TIME,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_UPDATE_TIME,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_DELETE_TYPE,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_VERSION,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_REQUIRE_PASSWORD_SET,
        data_type: DataType::Uint32,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: COLUMN_CRITICAL1,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_CRITICAL2,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_CRITICAL3,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_CRITICAL4,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_NORMAL1,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_NORMAL2,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_NORMAL3,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: COLUMN_NORMAL4,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
];

/// do same operation in backup database when do something in master db
/// TODO backup every success operation or only when charge idle?
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
                    asset_common::loge!("backup db {} fail", table.db.back_path);
                } else {
                    asset_common::logi!("backup db {} succ", table.db.back_path);
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
                    asset_common::loge!("recovery master db {} fail", table.db.path);
                    Err(ErrCode::SqliteError)
                } else {
                    asset_common::logi!("recovery master db {} succ", table.db.path);

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
    /// the datas is a list of column-data Pair.
    /// if success, return line changes.
    /// if fail, return err code.
    ///
    /// the code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    ///
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let ret = helper.update_datas_default(&HashMap::<&'static str, Value>::new(), &HashMap::<&'static str, Value>::new());
    /// ```
    /// sql like:
    /// update table_name set alias='test_update' where AppId='owner' and Alias='alias'
    pub fn update_datas(&self, condition: &Condition, datas: &DbMap) -> Result<i32, ErrCode> {
        let closure = |e: &Table| e.update_row(condition, datas);
        back_db_when_succ(true, self, closure)
    }

    /// insert datas into asset db table.
    /// owner and alias is the primary-key for resources.
    /// the datas is a list of column-data Pair.
    /// if success, return line changes.
    /// if fail, return err code.
    ///
    /// the code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    ///
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let ret = helper.insert_datas_default(&HashMap::<&'static str, Value>::new());
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
    pub fn insert_multi_datas(
        &self,
        columns: &Vec<&'static str>,
        datas: &Vec<Vec<Value>>,
    ) -> Result<i32, ErrCode> {
        let closure = |e: &Table| e.insert_multi_row_datas(columns, datas);
        back_db_when_succ(true, self, closure)
    }

    /// delete datas from asset db table.
    /// owner and alias is the primary-key for resources.
    /// the cond is a list of column-data Pair.
    /// if success, return line changes.
    /// if fail, return err code.
    ///
    /// the code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let ret = helper.delete_datas_default(&HashMap::<&'static str, Value>::new());
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
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let exist = helper.is_data_exists_default(&HashMap::<&'static str, Value>::new());
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
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let count = helper.select_count_default(&HashMap::<&'static str, Value>::new());
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
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let result = helper.query_datas_default(&HashMap::<&'static str, Value>::new(), None);
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
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// use std::collections::HashMap;
    /// use asset_common::definition::Value,
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let result = helper.query_columns_default(&vec![], &HashMap::<&'static str, Value>::new(), None);
    /// ```
    /// sql like:
    /// select * from table_name where AppId='owner' and Alias='alias'
    pub fn query_columns(
        &self,
        columns: &Vec<&'static str>,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>, ErrCode> {
        let closure = |e: &Table| e.query_datas_advanced(columns, condition, query_options);
        back_db_when_succ(false, self, closure)
    }
}

/// process err msg, this may be use in test, consider delete this function when release
pub fn process_err_msg<T>(
    res: Result<T, ErrCode>,
    db: &DefaultDatabaseHelper,
) -> Result<T, ErrCode> {
    if res.is_err() {
        if let Some(msg) = db.get_err_msg() {
            asset_common::loge!("db err info: {}", msg.s);
        } else {
            asset_common::loge!("db err with no msg");
        }
    }
    res
}

/// if table not exist, create default asset table
#[inline(always)]
fn create_default_table<'a>(db: &'a Database) -> Result<Table<'a>, ErrCode> {
    let res =
        db.create_table(ASSET_TABLE_NAME, COLUMN_INFO).map_err(from_sqlite_code_to_asset_code);
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
                let _ = sqlite3_close_wrap(db.v2, db.handle);
                // recovery master db
                let r_ret = copy_db_file(db, true);
                if r_ret.is_err() {
                    asset_common::loge!("recovery master db {} fail", db.path);
                    Err(ErrCode::SqliteError)
                } else {
                    asset_common::logi!("recovery master db {} succ", db.path);
                    let o_ret = db.re_open();
                    if let Err(e) = o_ret {
                        asset_common::loge!("reopen master db {} fail {}", db.path, e);
                        return Err(ErrCode::SqliteError);
                    }
                    process_err_msg(
                        db.open_table(ASSET_TABLE_NAME).map_err(from_sqlite_code_to_asset_code),
                        db,
                    )
                }
            } else {
                process_err_msg(Err(from_sqlite_code_to_asset_code(e)), db)
            }
        },
    }
}

impl<'a> DefaultDatabaseHelper<'a> {
    /// see TableHelper
    #[inline(always)]
    pub fn update_datas_default(
        &self,
        condition: &Condition,
        datas: &DbMap,
    ) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        #[cfg(feature = "auto_insert_time")]
        {
            let mut contain_update_time = false;
            for data in datas {
                if data.column_name == COLUMN_UPDATE_TIME {
                    contain_update_time = true;
                    break;
                }
            }
            if !contain_update_time {
                let ctime = std::time::UNIX_EPOCH.elapsed().unwrap().as_secs().to_string();
                let mut datas_new = Vec::<Pair>::with_capacity(datas.len() + 1);
                for data in datas {
                    datas_new.push(*data);
                }
                datas_new.push(Pair {
                    column_name: COLUMN_UPDATE_TIME,
                    value: Value::Bytes(ctime.as_bytes()),
                });
                return table.update_datas(condition, &datas_new);
            }
        }
        table.update_datas(condition, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_datas_default(&self, datas: &DbMap) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        #[cfg(feature = "auto_insert_time")]
        {
            let mut contain_create_time = false;
            let mut contain_update_time = false;
            for data in datas {
                if data.column_name == COLUMN_CREATE_TIME {
                    contain_create_time = true;
                }
                if data.column_name == COLUMN_UPDATE_TIME {
                    contain_update_time = true;
                }
            }
            if !contain_create_time || !contain_update_time {
                let ctime = std::time::UNIX_EPOCH.elapsed().unwrap().as_secs().to_string();
                let mut datas_new = Vec::<Pair>::with_capacity(datas.len() + 2);
                for data in datas {
                    datas_new.push(*data);
                }
                if !contain_create_time {
                    datas_new.push(Pair {
                        column_name: COLUMN_CREATE_TIME,
                        value: Value::Bytes(ctime.as_bytes()),
                    });
                }
                if !contain_update_time {
                    datas_new.push(Pair {
                        column_name: COLUMN_UPDATE_TIME,
                        value: Value::Bytes(ctime.as_bytes()),
                    });
                }
                return table.insert_datas(&datas_new);
            }
        }
        table.insert_datas(datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_multi_datas_default(
        &self,
        columns: &Vec<&'static str>,
        datas: &Vec<Vec<Value>>,
    ) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.insert_multi_datas(columns, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn delete_datas_default(&self, cond: &Condition) -> Result<i32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.delete_datas(cond)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn is_data_exists_default(&self, condition: &Condition) -> Result<bool, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.is_data_exist(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn select_count_default(&self, condition: &Condition) -> Result<u32, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.select_count(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_datas_default(
        &self,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<ResultSet, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.query_datas(condition, query_options)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_columns_default(
        &self,
        columns: &Vec<&'static str>,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>, ErrCode> {
        let table = Table::new(ASSET_TABLE_NAME, self);
        table.query_columns(columns, condition, query_options)
    }
}

impl<'a> DefaultDatabaseHelper<'a> {
    /// open default database and table
    pub fn open_default_database_table(userid: i32) -> Result<DefaultDatabaseHelper<'a>, ErrCode> {
        let mut db = Database::default_new(userid).map_err(from_sqlite_code_to_asset_code)?;
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
    pub fn open_default_database_table_with_version_update(
        userid: i32,
        version_new: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<DefaultDatabaseHelper<'a>, ErrCode> {
        let mut db = Database::default_new_with_version_update(userid, version_new, callback)
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
    pub fn update_datas_default_once(
        userid: i32,
        condition: &Condition,
        datas: &DbMap,
    ) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.update_datas_default(condition, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_datas_default_once(userid: i32, datas: &DbMap) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.insert_datas_default(datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_multi_datas_default_once(
        userid: i32,
        columns: &Vec<&'static str>,
        datas: &Vec<Vec<Value>>,
    ) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.insert_multi_datas_default(columns, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn delete_datas_default_once(userid: i32, cond: &Condition) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.delete_datas_default(cond)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn is_data_exists_default_once(
        userid: i32,
        condition: &Condition,
    ) -> Result<bool, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.is_data_exists_default(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn select_count_default_once(userid: i32, condition: &Condition) -> Result<u32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.select_count_default(condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_datas_default_once(
        userid: i32,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<ResultSet, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.query_datas_default(condition, query_options)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_columns_default_once(
        userid: i32,
        columns: &Vec<&'static str>,
        condition: &Condition,
        query_options: Option<&QueryOptions>,
    ) -> Result<Vec<DbMap>, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.query_columns_default(columns, condition, query_options)
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
pub fn do_transaction<F: Fn(&Database) -> bool>(userid: i32, callback: F) -> Result<bool, ErrCode> {
    let db = match DefaultDatabaseHelper::open_default_database_table(userid) {
        Ok(o) => o,
        Err(e) => {
            asset_common::loge!("transaction open db fail");
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
            asset_common::loge!("trans commit fail {}", ret);
            return Err(from_sqlite_code_to_asset_code(ret));
        }
        Ok(true)
    } else {
        let ret = trans.rollback();
        if ret != SQLITE_OK {
            asset_common::loge!("trans rollback fail {}", ret);
            return Err(from_sqlite_code_to_asset_code(ret));
        }
        Ok(false)
    }
}
