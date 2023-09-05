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
    database::{Database, UpdateDatabaseCallbackFunc},
    table::Table,
    types::{ColumnInfo, Condition, DataType, DataValue, Pair, ResultSet},
    SqliteErrcode,
};

/// just use database
pub type DatabaseHelper = Database;
/// just use database
pub type DefaultDatabaseHelper = Database;
/// just use table
pub type TableHelper<'a> = Table<'a>;

/// default table name
pub const G_ASSET_TABLE_NAME: &str = "asset_table";
/// default owner column name
pub const G_COLUMN_OWNER: &str = "Owner";
/// default alias column name
pub const G_COLUMN_ALIAS: &str = "Alias";

impl<'a> TableHelper<'a> {
    #[cfg(not(doctest))]
    ///
    /// update datas in asset db table.
    /// owner and alias is the primary-key for resources.
    /// the datas is a list of column-data Pair.
    /// if success, return line changes.
    /// if fail, return errcode.
    ///
    /// the code like:
    ///```
    /// let datas = &vec![Pair {
    ///     column_name: "alias",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = helper.update_datas("owner", "alias", datas);
    ///```
    /// sql like:
    /// update table_name set alias='test_update' where AppId='owner' and Alias='alias'
    ///
    pub fn update_datas(
        &self,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, SqliteErrcode> {
        let conditions = &vec![
            Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) },
            Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) },
        ];
        self.update_row(conditions, datas)
    }

    #[cfg(not(doctest))]
    ///
    /// insert datas into asset db table.
    /// owner and alias is the primary-key for resources.
    /// the datas is a list of column-data Pair.
    /// if success, return line changes.
    /// if fail, return errcode.
    ///
    /// the code like:
    ///```
    /// let datas = vec![Pair {
    ///     column_name: "value",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = helper.insert_data(owner, alias, datas);
    ///```
    ///
    /// sql like:
    /// insert into table_name(Owner,Alias,value) values(owner,alias,'test_update')
    ///
    pub fn insert_datas(
        &self,
        owner: &str,
        alias: &str,
        datas: Vec<Pair>,
    ) -> Result<i32, SqliteErrcode> {
        let mut v = Vec::<Pair>::with_capacity(datas.len() + 2);
        v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        for data in datas {
            v.push(data);
        }
        self.insert_row(&v)
    }

    #[cfg(not(doctest))]
    ///
    /// delete datas from asset db table.
    /// owner and alias is the primary-key for resources.
    /// the cond is a list of column-data Pair.
    /// if success, return line changes.
    /// if fail, return errcode.
    ///
    /// the code like:
    ///```
    /// let cond = vec![Pair {
    ///     column_name: "value",
    ///     value: DataValue::Text(b"test_update"),
    /// }];
    ///
    /// let ret = helper.delete_datas(owner, alias, cond);
    ///```
    ///
    /// sql like:
    /// delete from table_name where Owner=owner and Alias=alias and value='test_update'
    ///
    pub fn delete_datas(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<i32, SqliteErrcode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        for c in condition {
            v.push(*c);
        }
        self.delete_row(&v)
    }

    #[cfg(not(doctest))]
    ///
    /// return if data exists.
    /// if fail, return errcode.
    ///
    /// code like:
    ///```
    /// let exist = helper.is_data_exist("owner1", "alias1");
    ///```
    ///
    /// sql like:
    /// select count(*) as count from table_name where Owner='owner1' and Alias='alias1'
    ///
    pub fn is_data_exist(&self, owner: &str, alias: &str) -> Result<bool, SqliteErrcode> {
        self.is_data_exists(&vec![
            Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) },
            Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) },
        ])
    }

    #[cfg(not(doctest))]
    ///
    /// return select count for owner.
    /// if fail, return errcode.
    ///
    /// code like:
    ///```
    /// let count = helper.select_count("owner2");
    ///```
    /// sql like:
    /// select count(*) as count from table_name where AppId='owner2'
    ///
    pub fn select_count(&self, owner: &str) -> Result<i32, SqliteErrcode> {
        self.count_datas(&vec![Pair {
            column_name: G_COLUMN_OWNER,
            value: DataValue::Text(owner.as_bytes()),
        }])
    }

    #[cfg(not(doctest))]
    ///
    /// query all datas for owner and alias with condition(condition could be empty).
    /// if success, return result set.
    /// if fail, return errcode.
    ///
    /// code like:
    ///```
    /// let result = helper.query_datas(owner, alias, &vec![]);
    ///```
    /// sql like:
    /// select * from table_name where AppId='owner' and Alias='alias'
    ///
    pub fn query_datas(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, SqliteErrcode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        for c in condition {
            v.push(*c);
        }
        self.query_row(&vec![], &v)
    }
}

///
/// if table not exist, create default asset table
///
fn create_default_table(db: &Database) -> Result<Table, SqliteErrcode> {
    let columns = &[
        ColumnInfo {
            name: "Id",
            data_type: DataType::INTEGER,
            is_primary_key: true,
            not_null: true,
        },
        ColumnInfo {
            name: "Secret",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
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
            name: "OwnerType",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "GroupId",
            data_type: DataType::TEXT,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "SyncType",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "AccessType",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "AuthType",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "CreateTime",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "UpdateTime",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DeleteType",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "Version",
            data_type: DataType::INTEGER,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelCritical_1",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelCritical_2",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelCritical_3",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelCritical_4",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelNormal_1",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelNormal_2",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelNormal_3",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
        ColumnInfo {
            name: "DataLabelNormal_4",
            data_type: DataType::BLOB,
            is_primary_key: false,
            not_null: false,
        },
    ];
    db.create_table(G_ASSET_TABLE_NAME, columns)
}

impl DefaultDatabaseHelper {
    ///
    /// see TableHelper
    ///
    pub fn update_datas_default(
        &self,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, SqliteErrcode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.update_datas(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    pub fn insert_datas_default(
        &self,
        owner: &str,
        alias: &str,
        datas: Vec<Pair>,
    ) -> Result<i32, SqliteErrcode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.insert_datas(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    pub fn delete_datas_default(
        &self,
        owner: &str,
        alias: &str,
        cond: &Condition,
    ) -> Result<i32, SqliteErrcode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.delete_datas(owner, alias, cond)
    }

    ///
    /// see TableHelper
    ///
    pub fn is_data_exists_default(&self, owner: &str, alias: &str) -> Result<bool, SqliteErrcode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.is_data_exist(owner, alias)
    }

    ///
    /// see TableHelper
    ///
    pub fn select_count_default(&self, owner: &str) -> Result<i32, SqliteErrcode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.select_count(owner)
    }

    ///
    /// see TableHelper
    ///
    pub fn query_datas_default(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, SqliteErrcode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.query_datas(owner, alias, condition)
    }
}

impl DefaultDatabaseHelper {
    ///
    /// open default database and table
    ///
    pub fn open_default_database_table(
        userid: &str,
        el: &str,
    ) -> Result<DefaultDatabaseHelper, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let _table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        Ok(db)
    }

    ///
    /// open default database and table, if need update db version, input callback
    ///
    pub fn open_default_database_table_with_version_update(
        userid: &str,
        el: &str,
        version_new: i32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<DefaultDatabaseHelper, SqliteErrcode> {
        let db = Database::default_new_with_version_update(userid, el, version_new, callback)?;
        let _table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        Ok(db)
    }

    ///
    /// see TableHelper
    ///
    pub fn update_datas_default_once(
        userid: &str,
        el: &str,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        table.update_datas(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    pub fn insert_datas_default_once(
        userid: &str,
        el: &str,
        owner: &str,
        alias: &str,
        datas: Vec<Pair>,
    ) -> Result<i32, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        table.insert_datas(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    pub fn delete_datas_default_once(
        userid: &str,
        el: &str,
        owner: &str,
        alias: &str,
        cond: &Condition,
    ) -> Result<i32, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        table.delete_datas(owner, alias, cond)
    }

    ///
    /// see TableHelper
    ///
    pub fn is_data_exists_default_once(
        userid: &str,
        el: &str,
        owner: &str,
        alias: &str,
    ) -> Result<bool, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        table.is_data_exist(owner, alias)
    }

    ///
    /// see TableHelper
    ///
    pub fn select_count_default_once(
        userid: &str,
        el: &str,
        owner: &str,
    ) -> Result<i32, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        table.select_count(owner)
    }

    ///
    /// see TableHelper
    ///
    pub fn query_datas_default_once(
        userid: &str,
        el: &str,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, SqliteErrcode> {
        let db = Database::default_new(userid, el)?;
        let table = match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(t) => t,
            Err(_) => create_default_table(&db)?,
        };
        table.query_datas(owner, alias, condition)
    }
}
