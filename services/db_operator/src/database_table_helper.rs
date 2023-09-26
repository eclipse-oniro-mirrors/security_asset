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

use asset_common::definition::ErrCode;

use crate::{
    database::{
        copy_db_file, copy_db_file_inner, is_database_file_error, sqlite3_close_wrap, Database,
        UpdateDatabaseCallbackFunc,
    },
    from_sqlite_code_to_asset_code,
    table::Table,
    transaction::Transaction,
    types::{AdvancedResultSet, ColumnInfo, Condition, DataType, DataValue, Pair, ResultSet},
    SqliteErrCode, SQLITE_OK,
};

/// just use database
pub type DatabaseHelper<'a> = Database<'a>;
/// just use database
pub type DefaultDatabaseHelper<'a> = Database<'a>;
/// just use table
pub type TableHelper<'a> = Table<'a>;

/// default table name
pub const G_ASSET_TABLE_NAME: &str = "asset_table";
/// default column name
pub const G_COLUMN_ID: &str = "Id";
/// default column name
pub const G_COLUMN_SECRET: &str = "Secret";
/// default column name
pub const G_COLUMN_OWNER: &str = "Owner";
/// default column name
pub const G_COLUMN_ALIAS: &str = "Alias";
/// default column name
pub const G_COLUMN_OWNER_TYPE: &str = "OwnerType";
/// default column name
pub const G_COLUMN_GROUP_ID: &str = "GroupId";
/// default column name
pub const G_COLUMN_SYNC_TYPE: &str = "SyncType";
/// default column name
pub const G_COLUMN_ACCESS_TYPE: &str = "AccessType";
/// default column name
pub const G_COLUMN_AUTH_TYPE: &str = "AuthType";
/// default column name
pub const G_COLUMN_CREATE_TIME: &str = "CreateTime";
/// default column name
pub const G_COLUMN_UPDATE_TIME: &str = "UpdateTime";
/// default column name
pub const G_COLUMN_DELETE_TYPE: &str = "DeleteType";
/// default column name
pub const G_COLUMN_VERSION: &str = "Version";
/// default column name
pub const G_COLUMN_REQUIRE_PASSWORD_SET: &str = "RequirePasswordSet";
/// default column name
pub const G_COLUMN_CRITICAL1: &str = "DataLabelCritical_1";
/// default column name
pub const G_COLUMN_CRITICAL2: &str = "DataLabelCritical_2";
/// default column name
pub const G_COLUMN_CRITICAL3: &str = "DataLabelCritical_3";
/// default column name
pub const G_COLUMN_CRITICAL4: &str = "DataLabelCritical_4";
/// default column name
pub const G_COLUMN_NORMAL1: &str = "DataLabelNormal_1";
/// default column name
pub const G_COLUMN_NORMAL2: &str = "DataLabelNormal_2";
/// default column name
pub const G_COLUMN_NORMAL3: &str = "DataLabelNormal_3";
/// default column name
pub const G_COLUMN_NORMAL4: &str = "DataLabelNormal_4";

/// columns info for default asset_table
pub const G_COLUMNS_INFO: &[ColumnInfo] = &[
    ColumnInfo {
        name: G_COLUMN_ID,
        data_type: DataType::INTEGER,
        is_primary_key: true,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_SECRET,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_ALIAS,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_OWNER,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_OWNER_TYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_GROUP_ID,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_SYNC_TYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_ACCESS_TYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_AUTH_TYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_CREATE_TIME,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_UPDATE_TIME,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_DELETE_TYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_VERSION,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_REQUIRE_PASSWORD_SET,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_CRITICAL1,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_CRITICAL2,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_CRITICAL3,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_CRITICAL4,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_NORMAL1,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_NORMAL2,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_NORMAL3,
        data_type: DataType::BLOB,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_NORMAL4,
        data_type: DataType::BLOB,
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
                    println!("backup db {} fail", table.db.back_path);
                } else {
                    println!("backup db {} succ", table.db.back_path);
                }
                //});
            }
            Ok(o)
        },
        Err(e) => {
            //if is_database_file_error(e) {
            // let _ = sqlite3_close_wrap(table.db.v2, table.db.handle);
            // // recovery master db
            // let r_ret = recovery_db_file(table.db, true);
            // if r_ret.is_err() {
            //     asset_common::loge!("recovery master db {} fail", table.db.path);
            //     Err(ErrCode::SqliteERROR)
            // } else {
            //     asset_common::logi!("recovery master db {} succ", table.db.path);
            //     let o_ret = table.db.re_open();
            //     if let Err(e) = o_ret {
            //         asset_common::loge!("reopen master db {} fail {}", table.db.path, e);
            //         Err(ErrCode::SqliteERROR)
            //     } else {
            //         let res = func(table);
            //         process_err_msg(res.map_err(from_sqlite_code_to_asset_code), table.db)
            //     }
            // }
            //} else {
            process_err_msg(Err(from_sqlite_code_to_asset_code(e)), table.db)
            //}
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
    /// use db_operator::types::{DataValue, Pair};
    ///
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let datas = &vec![Pair {
    ///     column_name: "alias",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = helper.update_datas_default("owner", "alias", datas);
    /// ```
    /// sql like:
    /// update table_name set alias='test_update' where AppId='owner' and Alias='alias'
    pub fn update_datas(
        &self,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, ErrCode> {
        let mut v = vec![];
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        if !alias.is_empty() {
            v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        }
        let closure = |e: &Table| e.update_row(&v, datas);
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
    /// use db_operator::types::{DataValue, Pair};
    ///
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let datas = vec![Pair {
    ///     column_name: "value",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = helper.insert_datas_default("owner", "alias", &datas);
    /// ```
    ///
    /// sql like:
    /// insert into table_name(Owner,Alias,value) values(owner,alias,'test_update')
    pub fn insert_datas(
        &self,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, ErrCode> {
        let mut v = Vec::<Pair>::with_capacity(datas.len() + 2);
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        if !alias.is_empty() {
            v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        }
        for data in datas {
            v.push(*data);
        }
        let closure = |e: &Table| e.insert_row(&v);
        back_db_when_succ(true, self, closure)
    }

    /// insert multi datas
    /// columns: the columns
    /// datas: the data set
    pub fn insert_multi_datas(
        &self,
        columns: &Vec<&str>,
        datas: &Vec<Vec<DataValue>>,
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
    /// use db_operator::types::{DataValue, Pair};
    ///
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let cond = vec![Pair {
    ///     column_name: "value",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = helper.delete_datas_default("owner", "alias", &cond);
    /// ```
    ///
    /// sql like:
    /// delete from table_name where Owner=owner and Alias=alias and value='test_update'
    pub fn delete_datas(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<i32, ErrCode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        if !alias.is_empty() {
            v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        }
        for c in condition {
            v.push(*c);
        }
        let closure = |e: &Table| e.delete_row(&v);
        back_db_when_succ(true, self, closure)
    }

    /// return if data exists.
    /// if fail, return err code.
    ///
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let exist = helper.is_data_exists_default("owner1", "alias1");
    /// ```
    ///
    /// sql like:
    /// select count(*) as count from table_name where Owner='owner1' and Alias='alias1'
    pub fn is_data_exist(&self, owner: &str, alias: &str) -> Result<bool, ErrCode> {
        let mut v = vec![];
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        if !alias.is_empty() {
            v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        }
        let closure = |e: &Table| e.is_data_exists(&v);
        back_db_when_succ(false, self, closure)
    }

    /// return select count for owner.
    /// if fail, return err code.
    ///
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let count = helper.select_count_default("owner2");
    /// ```
    /// sql like:
    /// select count(*) as count from table_name where AppId='owner2'
    pub fn select_count(&self, owner: &str) -> Result<u32, ErrCode> {
        let mut v = vec![];
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        let closure = |e: &Table| e.count_datas(&v);
        back_db_when_succ(false, self, closure)
    }

    /// query all datas for owner and alias with condition(condition could be empty).
    /// if success, return result set.
    /// if fail, return err code.
    ///
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let result = helper.query_datas_default("owner", "alias", &vec![]);
    /// ```
    /// sql like:
    /// select * from table_name where AppId='owner' and Alias='alias'
    pub fn query_datas(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, ErrCode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        if !alias.is_empty() {
            v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        }
        for c in condition {
            v.push(*c);
        }
        let closure = |e: &Table| e.query_row(&vec![], &v);
        back_db_when_succ(false, self, closure)
    }

    /// query special columns with condition(condition could be empty).
    /// if success, return result set.
    /// if fail, return err code.
    ///
    /// code like:
    /// ```
    /// use db_operator::database_table_helper::DefaultDatabaseHelper;
    /// let helper = DefaultDatabaseHelper::open_default_database_table(1).unwrap();
    /// let result = helper.query_columns_default(&vec![], "owner", "alias", &vec![]);
    /// ```
    /// sql like:
    /// select * from table_name where AppId='owner' and Alias='alias'
    pub fn query_columns(
        &self,
        columns: &Vec<&str>,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<AdvancedResultSet, ErrCode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        if !owner.is_empty() {
            v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        } else {
            return Err(ErrCode::AccessDenied);
        }
        if !alias.is_empty() {
            v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        }
        for c in condition {
            v.push(*c);
        }
        let closure = |e: &Table| e.query_datas_advanced(columns, &v);
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
        db.create_table(G_ASSET_TABLE_NAME, G_COLUMNS_INFO).map_err(from_sqlite_code_to_asset_code);
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
    let res = db.open_table(G_ASSET_TABLE_NAME);
    match res {
        Ok(o) => {
            if o.is_none() {
                return Ok(None);
            }
            return Ok(Some(Table::new(G_ASSET_TABLE_NAME, db)));
        },
        Err(e) => {
            if is_database_file_error(e) {
                let _ = sqlite3_close_wrap(db.v2, db.handle);
                // recovery master db
                let r_ret = copy_db_file(db, true);
                if r_ret.is_err() {
                    asset_common::loge!("recovery master db {} fail", db.path);
                    Err(ErrCode::SqliteERROR)
                } else {
                    asset_common::logi!("recovery master db {} succ", db.path);
                    let o_ret = db.re_open();
                    if let Err(e) = o_ret {
                        asset_common::loge!("reopen master db {} fail {}", db.path, e);
                        return Err(ErrCode::SqliteERROR);
                    }
                    process_err_msg(
                        db.open_table(G_ASSET_TABLE_NAME).map_err(from_sqlite_code_to_asset_code),
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
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        #[cfg(feature = "auto_insert_time")]
        {
            let mut contain_update_time = false;
            for data in datas {
                if data.column_name == G_COLUMN_UPDATE_TIME {
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
                    column_name: G_COLUMN_UPDATE_TIME,
                    value: DataValue::Text(ctime.as_bytes()),
                });
                return table.update_datas(owner, alias, &datas_new);
            }
        }
        table.update_datas(owner, alias, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_datas_default(
        &self,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        #[cfg(feature = "auto_insert_time")]
        {
            let mut contain_create_time = false;
            let mut contain_update_time = false;
            for data in datas {
                if data.column_name == G_COLUMN_CREATE_TIME {
                    contain_create_time = true;
                }
                if data.column_name == G_COLUMN_UPDATE_TIME {
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
                        column_name: G_COLUMN_CREATE_TIME,
                        value: DataValue::Text(ctime.as_bytes()),
                    });
                }
                if !contain_update_time {
                    datas_new.push(Pair {
                        column_name: G_COLUMN_UPDATE_TIME,
                        value: DataValue::Text(ctime.as_bytes()),
                    });
                }
                return table.insert_datas(owner, alias, &datas_new);
            }
        }
        table.insert_datas(owner, alias, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_multi_datas_default(
        &self,
        columns: &Vec<&str>,
        datas: &Vec<Vec<DataValue>>,
    ) -> Result<i32, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.insert_multi_datas(columns, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn delete_datas_default(
        &self,
        owner: &str,
        alias: &str,
        cond: &Condition,
    ) -> Result<i32, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.delete_datas(owner, alias, cond)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn is_data_exists_default(&self, owner: &str, alias: &str) -> Result<bool, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.is_data_exist(owner, alias)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn select_count_default(&self, owner: &str) -> Result<u32, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.select_count(owner)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_datas_default(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.query_datas(owner, alias, condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_columns_default(
        &self,
        columns: &Vec<&str>,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<AdvancedResultSet, ErrCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.query_columns(columns, owner, alias, condition)
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
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.update_datas_default(owner, alias, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_datas_default_once(
        userid: i32,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.insert_datas_default(owner, alias, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn insert_multi_datas_default_once(
        userid: i32,
        columns: &Vec<&str>,
        datas: &Vec<Vec<DataValue>>,
    ) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.insert_multi_datas_default(columns, datas)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn delete_datas_default_once(
        userid: i32,
        owner: &str,
        alias: &str,
        cond: &Condition,
    ) -> Result<i32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.delete_datas_default(owner, alias, cond)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn is_data_exists_default_once(
        userid: i32,
        owner: &str,
        alias: &str,
    ) -> Result<bool, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.is_data_exists_default(owner, alias)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn select_count_default_once(userid: i32, owner: &str) -> Result<u32, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.select_count_default(owner)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_datas_default_once(
        userid: i32,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.query_datas_default(owner, alias, condition)
    }

    /// see TableHelper
    #[inline(always)]
    pub fn query_columns_default_once(
        userid: i32,
        columns: &Vec<&str>,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<AdvancedResultSet, ErrCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        let _lock = db.file.mtx.lock().unwrap();
        db.query_columns_default(columns, owner, alias, condition)
    }
}

/// transaction callback func, do NOT lock database in this callback function
/// return true if want to commit, false if want to rollback
pub type TransactionCallback = fn(db: &Database) -> bool;

/// do transaction
/// if commit, return true
/// if rollback, return false
pub fn do_transaction(userid: i32, callback: TransactionCallback) -> Result<bool, ErrCode> {
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
