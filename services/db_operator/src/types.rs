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

//! This module defines the common data structure of the database module.

use std::{cmp::Ordering, collections::HashMap};

use asset_definition::{DataType, ErrCode, Value};

/// A Map type containing tag-value pairs that describe the attributes of an DB field.
pub type DbMap = HashMap<&'static str, Value>;

/// Table name of asset database.
pub const TABLE_NAME: &str = "asset_table";

/// Version V1 number for upgrade database
pub const DB_UPGRADE_VERSION_V1: u32 = 0;
/// Version V2 number for upgrade database
pub const DB_UPGRADE_VERSION_V2: u32 = 1;
/// Version V3 number for upgrade database
pub const DB_UPGRADE_VERSION_V3: u32 = 2;
/// Latest version number for upgrade database
pub const DB_UPGRADE_VERSION: u32 = 3;

/// Version 1 number
pub const DB_DATA_VERSION_V1: u32 = 1;
/// Version 2 number
pub const DB_DATA_VERSION_V2: u32 = 2;
/// Latest data version number.
pub const DB_DATA_VERSION: u32 = 3;
/// Column name of asset database.
pub mod column {
    /// Column name of the primary key Id.
    pub const ID: &str = "Id";
    /// Column name of secret cipher.
    pub const SECRET: &str = "Secret";
    /// Column name of data alias.
    pub const ALIAS: &str = "Alias";
    /// Column name of data owner.
    pub const OWNER: &str = "Owner";
    /// Column name of owner type.
    pub const OWNER_TYPE: &str = "OwnerType";
    /// Column name of unique id of a group. (reserved)
    pub const GROUP_ID: &str = "GroupId";
    /// Column name of data synchronization type.
    pub const SYNC_TYPE: &str = "SyncType";
    /// Column name of data accessibility
    pub const ACCESSIBILITY: &str = "Accessibility";
    /// Column name of the user authentication type supported by the data
    pub const AUTH_TYPE: &str = "AuthType";
    /// Column name of data creation time.
    pub const CREATE_TIME: &str = "CreateTime";
    /// Column name of the data update time.
    pub const UPDATE_TIME: &str = "UpdateTime";
    /// Column name of the data persistence attribute.
    pub const IS_PERSISTENT: &str = "IsPersistent";
    /// Column name of the data version number.
    pub const VERSION: &str = "Version";
    /// Column name of if data require password set
    pub const REQUIRE_PASSWORD_SET: &str = "RequirePasswordSet";
    /// Column name of the first critical data label.
    pub const CRITICAL1: &str = "DataLabelCritical_1";
    /// Column name of the second critical data label.
    pub const CRITICAL2: &str = "DataLabelCritical_2";
    /// Column name of the third critical data label.
    pub const CRITICAL3: &str = "DataLabelCritical_3";
    /// Column name of the fourth critical data label.
    pub const CRITICAL4: &str = "DataLabelCritical_4";
    /// Column name of the first normal data label.
    pub const NORMAL1: &str = "DataLabelNormal_1";
    /// Column name of the second normal data label.
    pub const NORMAL2: &str = "DataLabelNormal_2";
    /// Column name of the third normal data label.
    pub const NORMAL3: &str = "DataLabelNormal_3";
    /// Column name of the fourth normal data label.
    pub const NORMAL4: &str = "DataLabelNormal_4";
    /// Column name of the first normal local data label.
    pub const NORMAL_LOCAL1: &str = "DataLabelNormalLocal_1";
    /// Column name of the second normal local data label.
    pub const NORMAL_LOCAL2: &str = "DataLabelNormalLocal_2";
    /// Column name of the third normal local data label.
    pub const NORMAL_LOCAL3: &str = "DataLabelNormalLocal_3";
    /// Column name of the fourth normal local data label.
    pub const NORMAL_LOCAL4: &str = "DataLabelNormalLocal_4";
    /// Column name of the first normal local data label.
    pub const GLOBAL_ID: &str = "GlobalId";
    /// Column name of the second normal local data label.
    pub const CLOUD_VERSION: &str = "CloudVersion";
    /// Column name of the third normal local data label.
    pub const LOCAL_STATUS: &str = "LocalStatus";
    /// Column name of the fourth normal local data label.
    pub const SYNC_STATUS: &str = "SyncStatus";
    /// Column name of the ext data info.
    pub const EXT_INFO: &str = "ExtInfo";
}

#[repr(C)]
pub(crate) struct ColumnInfo {
    pub(crate) name: &'static str,
    pub(crate) data_type: DataType,
    pub(crate) is_primary_key: bool,
    pub(crate) not_null: bool,
}

pub(crate) const COLUMN_INFO: &[ColumnInfo] = &[
    ColumnInfo { name: column::ID, data_type: DataType::Number, is_primary_key: true, not_null: true },
    ColumnInfo { name: column::SECRET, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::ALIAS, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::OWNER, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::OWNER_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::GROUP_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::SYNC_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::ACCESSIBILITY, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::AUTH_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::CREATE_TIME, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::UPDATE_TIME, data_type: DataType::Bytes, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::IS_PERSISTENT, data_type: DataType::Bool, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::VERSION, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::REQUIRE_PASSWORD_SET, data_type: DataType::Bool, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::CRITICAL1, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CRITICAL2, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CRITICAL3, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CRITICAL4, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL1, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL2, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL3, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL4, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL_LOCAL1, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL_LOCAL2, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL_LOCAL3, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::NORMAL_LOCAL4, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::GLOBAL_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::CLOUD_VERSION, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: column::LOCAL_STATUS, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::SYNC_STATUS, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: column::EXT_INFO, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
];

pub(crate) struct UpgradeColumnInfo {
    pub(crate) base_info: ColumnInfo,
    pub(crate) default_value: Option<Value>,
}

pub(crate) const UPGRADE_COLUMN_INFO_V2: &[UpgradeColumnInfo] = &[
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::NORMAL_LOCAL1,
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: false,
        },
        default_value: None,
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::NORMAL_LOCAL2,
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: false,
        },
        default_value: None,
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::NORMAL_LOCAL3,
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: false,
        },
        default_value: None,
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::NORMAL_LOCAL4,
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: false,
        },
        default_value: None,
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::GLOBAL_ID,
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: false,
        },
        default_value: None,
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::CLOUD_VERSION,
            data_type: DataType::Bytes,
            is_primary_key: false,
            not_null: false,
        },
        default_value: None,
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::LOCAL_STATUS,
            data_type: DataType::Number,
            is_primary_key: false,
            not_null: true,
        },
        default_value: Some(Value::Number(0)),
    },
    UpgradeColumnInfo {
        base_info: ColumnInfo {
            name: column::SYNC_STATUS,
            data_type: DataType::Number,
            is_primary_key: false,
            not_null: true,
        },
        default_value: Some(Value::Number(0)),
    },
];

pub(crate) const UPGRADE_COLUMN_INFO_V3: &[UpgradeColumnInfo] = &[UpgradeColumnInfo {
    base_info: ColumnInfo {
        name: column::EXT_INFO,
        data_type: DataType::Bytes,
        is_primary_key: false,
        not_null: false,
    },
    default_value: None,
}];

pub(crate) const UPGRADE_COLUMN_INFO: &[UpgradeColumnInfo] = &[];

/// Options for batch query.
#[repr(C)]
pub struct QueryOptions {
    /// The offset of the query result.
    pub offset: Option<u32>,
    /// Maximum number of query results.
    pub limit: Option<u32>,
    /// ordering: Ordering::Greater => ASC and Ordering::Less => DESC
    pub order: Option<Ordering>,
    /// Columns used for sorting.
    pub order_by: Option<Vec<&'static str>>,
}

pub(crate) const SQLITE_OK: i32 = 0;
pub(crate) const SQLITE_NOMEM: i32 = 7;
pub(crate) const SQLITE_CORRUPT: i32 = 11;
pub(crate) const SQLITE_NOTADB: i32 = 26;
/// Another row willed queried by function: sqlite3_step().
pub(crate) const SQLITE_ROW: i32 = 100;
/// End the execution of function sqlite3_step().
pub(crate) const SQLITE_DONE: i32 = 101;

pub(crate) fn sqlite_err_handle(ret: i32) -> ErrCode {
    match ret {
        SQLITE_CORRUPT | SQLITE_NOTADB => ErrCode::DataCorrupted,
        SQLITE_NOMEM => ErrCode::OutOfMemory,
        _ => ErrCode::DatabaseError,
    }
}
