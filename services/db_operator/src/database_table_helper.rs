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

use asset_common_lib::asset_type::AssetStatusCode;

use crate::{
    database::{Database, UpdateDatabaseCallbackFunc},
    from_sqlitecode_to_assetcode,
    table::Table,
    types::{ColumnInfo, Condition, DataType, DataValue, Pair, ResultSet},
};

/// just use database
pub type DatabaseHelper = Database;
/// just use database
pub type DefaultDatabaseHelper = Database;
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
pub const G_COLUMN_OWNERTYPE: &str = "OwnerType";
/// default column name
pub const G_COLUMN_GROUPID: &str = "GroupId";
/// default column name
pub const G_COLUMN_SYNCTYPE: &str = "SyncType";
/// default column name
pub const G_COLUMN_ACCESSTYPE: &str = "AccessType";
/// default column name
pub const G_COLUMN_AUTHTYPE: &str = "AuthType";
/// default column name
pub const G_COLUMN_CREATETIME: &str = "CreateTime";
/// default column name
pub const G_COLUMN_UPDATETIME: &str = "UpdateTime";
/// default column name
pub const G_COLUMN_DELETETYPE: &str = "DeleteType";
/// default column name
pub const G_COLUMN_VERSION: &str = "Version";
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
        name: G_COLUMN_OWNERTYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_GROUPID,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: false,
    },
    ColumnInfo {
        name: G_COLUMN_SYNCTYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_ACCESSTYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_AUTHTYPE,
        data_type: DataType::INTEGER,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_CREATETIME,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_UPDATETIME,
        data_type: DataType::TEXT,
        is_primary_key: false,
        not_null: true,
    },
    ColumnInfo {
        name: G_COLUMN_DELETETYPE,
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
    ) -> Result<i32, AssetStatusCode> {
        let conditions = &vec![
            Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) },
            Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) },
        ];
        self.update_row(conditions, datas).map_err(from_sqlitecode_to_assetcode)
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
    ) -> Result<i32, AssetStatusCode> {
        let mut v = Vec::<Pair>::with_capacity(datas.len() + 2);
        v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        for data in datas {
            v.push(data);
        }
        self.insert_row(&v).map_err(from_sqlitecode_to_assetcode)
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
    ) -> Result<i32, AssetStatusCode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        for c in condition {
            v.push(*c);
        }
        self.delete_row(&v).map_err(from_sqlitecode_to_assetcode)
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
    pub fn is_data_exist(&self, owner: &str, alias: &str) -> Result<bool, AssetStatusCode> {
        self.is_data_exists(&vec![
            Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) },
            Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) },
        ])
        .map_err(from_sqlitecode_to_assetcode)
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
    pub fn select_count(&self, owner: &str) -> Result<u32, AssetStatusCode> {
        self.count_datas(&vec![Pair {
            column_name: G_COLUMN_OWNER,
            value: DataValue::Text(owner.as_bytes()),
        }])
        .map_err(from_sqlitecode_to_assetcode)
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
    ) -> Result<ResultSet, AssetStatusCode> {
        let mut v = Vec::<Pair>::with_capacity(condition.len() + 2);
        v.push(Pair { column_name: G_COLUMN_OWNER, value: DataValue::Text(owner.as_bytes()) });
        v.push(Pair { column_name: G_COLUMN_ALIAS, value: DataValue::Text(alias.as_bytes()) });
        for c in condition {
            v.push(*c);
        }
        self.query_row(&vec![], &v).map_err(from_sqlitecode_to_assetcode)
    }
}

///
/// if table not exist, create default asset table
///
#[inline(always)]
fn create_default_table(db: &Database) -> Result<Table, AssetStatusCode> {
    db.create_table(G_ASSET_TABLE_NAME, G_COLUMNS_INFO).map_err(from_sqlitecode_to_assetcode)
}

impl DefaultDatabaseHelper {
    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn update_datas_default(
        &self,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, AssetStatusCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        #[cfg(feature = "auto_insert_time")]
        {
            let mut contain_update_time = false;
            for data in datas {
                if data.column_name == G_COLUMN_UPDATETIME {
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
                    column_name: G_COLUMN_UPDATETIME,
                    value: DataValue::Text(ctime.as_bytes()),
                });
                return table.update_datas(owner, alias, &datas_new);
            }
        }
        table.update_datas(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn insert_datas_default(
        &self,
        owner: &str,
        alias: &str,
        datas: Vec<Pair>,
    ) -> Result<i32, AssetStatusCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        #[cfg(feature = "auto_insert_time")]
        {
            let mut contain_create_time = false;
            let mut contain_update_time = false;
            for data in &datas {
                if data.column_name == G_COLUMN_CREATETIME {
                    contain_create_time = true;
                }
                if data.column_name == G_COLUMN_UPDATETIME {
                    contain_update_time = true;
                }
            }
            if !contain_create_time || !contain_update_time {
                let ctime = std::time::UNIX_EPOCH.elapsed().unwrap().as_secs().to_string();
                let mut datas_new = Vec::<Pair>::with_capacity(datas.len() + 2);
                for data in &datas {
                    datas_new.push(*data);
                }
                if !contain_create_time {
                    datas_new.push(Pair {
                        column_name: G_COLUMN_CREATETIME,
                        value: DataValue::Text(ctime.as_bytes()),
                    });
                }
                if !contain_update_time {
                    datas_new.push(Pair {
                        column_name: G_COLUMN_UPDATETIME,
                        value: DataValue::Text(ctime.as_bytes()),
                    });
                }
                return table.insert_datas(owner, alias, datas_new);
            }
        }
        table.insert_datas(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn delete_datas_default(
        &self,
        owner: &str,
        alias: &str,
        cond: &Condition,
    ) -> Result<i32, AssetStatusCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.delete_datas(owner, alias, cond)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn is_data_exists_default(
        &self,
        owner: &str,
        alias: &str,
    ) -> Result<bool, AssetStatusCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.is_data_exist(owner, alias)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn select_count_default(&self, owner: &str) -> Result<u32, AssetStatusCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.select_count(owner)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn query_datas_default(
        &self,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, AssetStatusCode> {
        let table = Table::new(G_ASSET_TABLE_NAME, self);
        table.query_datas(owner, alias, condition)
    }
}

impl DefaultDatabaseHelper {
    ///
    /// open default database and table
    ///
    pub fn open_default_database_table(
        userid: u32,
    ) -> Result<DefaultDatabaseHelper, AssetStatusCode> {
        let db = Database::default_new(userid).map_err(from_sqlitecode_to_assetcode)?;
        match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(_) => {},
            Err(_) => {
                create_default_table(&db)?;
            },
        };
        Ok(db)
    }

    ///
    /// open default database and table, if need update db version, input callback
    ///
    pub fn open_default_database_table_with_version_update(
        userid: u32,
        version_new: u32,
        callback: UpdateDatabaseCallbackFunc,
    ) -> Result<DefaultDatabaseHelper, AssetStatusCode> {
        let db = Database::default_new_with_version_update(userid, version_new, callback)
            .map_err(from_sqlitecode_to_assetcode)?;
        match db.open_table(G_ASSET_TABLE_NAME) {
            Ok(_) => {},
            Err(_) => {
                create_default_table(&db)?;
            },
        };
        Ok(db)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn update_datas_default_once(
        userid: u32,
        owner: &str,
        alias: &str,
        datas: &Vec<Pair>,
    ) -> Result<i32, AssetStatusCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        db.update_datas_default(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn insert_datas_default_once(
        userid: u32,
        owner: &str,
        alias: &str,
        datas: Vec<Pair>,
    ) -> Result<i32, AssetStatusCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        db.insert_datas_default(owner, alias, datas)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn delete_datas_default_once(
        userid: u32,
        owner: &str,
        alias: &str,
        cond: &Condition,
    ) -> Result<i32, AssetStatusCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        db.delete_datas_default(owner, alias, cond)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn is_data_exists_default_once(
        userid: u32,
        owner: &str,
        alias: &str,
    ) -> Result<bool, AssetStatusCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        db.is_data_exists_default(owner, alias)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn select_count_default_once(userid: u32, owner: &str) -> Result<u32, AssetStatusCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        db.select_count_default(owner)
    }

    ///
    /// see TableHelper
    ///
    #[inline(always)]
    pub fn query_datas_default_once(
        userid: u32,
        owner: &str,
        alias: &str,
        condition: &Condition,
    ) -> Result<ResultSet, AssetStatusCode> {
        let db = DefaultDatabaseHelper::open_default_database_table(userid)?;
        db.query_datas_default(owner, alias, condition)
    }
}
