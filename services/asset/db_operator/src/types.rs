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

use asset_definition::{DataType, ErrCode, Value, WrapType};
use asset_log::{loge, logw};

/// A Map type containing tag-value pairs that describe the attributes of an DB field.
pub type DbMap = HashMap<&'static str, Value>;

/// Table name of asset database.
pub const TABLE_NAME: &str = "asset_table";

/// Table name for cloud adapt.
pub(crate) const ADAPT_CLOUD_TABLE: &str = "adapt_cloud_table";

/// Version V0 number for upgrade database
pub const DB_UPGRADE_VERSION_V0: u32 = 0;
/// Version V1 number for upgrade database
pub const DB_UPGRADE_VERSION_V1: u32 = 1;
/// Version V2 number for upgrade database
pub const DB_UPGRADE_VERSION_V2: u32 = 2;
/// Version V3 number for upgrade database
pub const DB_UPGRADE_VERSION_V3: u32 = 3;
/// Latest version number for upgrade database
pub const DB_UPGRADE_VERSION: u32 = 4;

/// Version 1 number
pub const DB_DATA_VERSION_V1: u32 = 1;
/// Version 2 number
pub const DB_DATA_VERSION_V2: u32 = 2;
/// Version 3 number
pub const DB_DATA_VERSION_V3: u32 = 3;
/// Latest data version number.
pub const DB_DATA_VERSION: u32 = 4;
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
    /// Column name of (developerId + "," + groupId) of a group.
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
    /// Column name of the wrap type info.
    pub const WRAP_TYPE: &str = "WrapType";
}

/// Column name of asset database.
pub mod adapt_column {
    /// Column name of the global id.
    pub const OLD_GLOBAL_ID: &str = "OldGlobalId";
    /// Column name of the new global id.
    pub const NEW_GLOBAL_ID: &str = "NewGlobalId";
    /// Column name of the new data CloudVersion.
    pub const NEW_CLOUD_VERSION: &str = "NewCloudVersion";
}

#[repr(C)]
#[derive(Clone, Copy)]
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
    ColumnInfo { name: column::WRAP_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
];

pub(crate) const ADAPT_CLOUD_COLUMN_INFO: &[ColumnInfo] = &[
    ColumnInfo { name: adapt_column::OLD_GLOBAL_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: adapt_column::NEW_GLOBAL_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: adapt_column::NEW_CLOUD_VERSION, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
];

pub(crate) const COMBINE_COLUMN_INFO: &[ColumnInfo] = &[
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
    ColumnInfo { name: column::WRAP_TYPE, data_type: DataType::Number, is_primary_key: false, not_null: true },
    ColumnInfo { name: adapt_column::OLD_GLOBAL_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: adapt_column::NEW_GLOBAL_ID, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
    ColumnInfo { name: adapt_column::NEW_CLOUD_VERSION, data_type: DataType::Bytes, is_primary_key: false, not_null: false },
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

pub(crate) const UPGRADE_COLUMN_INFO_V4: &[UpgradeColumnInfo] = &[];

pub(crate) const UPGRADE_COLUMN_INFO: &[UpgradeColumnInfo] = &[UpgradeColumnInfo {
    base_info: ColumnInfo {
        name: column::WRAP_TYPE,
        data_type: DataType::Number,
        is_primary_key: false,
        not_null: true,
    },
    default_value: Some(Value::Number(WrapType::Never as u32)),
}];

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
    /// Add extra to where.
    pub amend: Option<String>,
}

/// SQLite error codes used by asset database operations.
pub(crate) struct SqliteErr;

impl SqliteErr {
    pub(crate) const OK: i32 = 0;
    pub(crate) const BUSY: i32 = 5;
    pub(crate) const NOMEM: i32 = 7;
    pub(crate) const READONLY: i32 = 8;
    pub(crate) const IOERR: i32 = 10;
    pub(crate) const CORRUPT: i32 = 11;
    pub(crate) const FULL: i32 = 13;
    pub(crate) const CANTOPEN: i32 = 14;
    pub(crate) const NOLFS: i32 = 22;
    pub(crate) const NOTADB: i32 = 26;
    /// Another row queried by function: sqlite3_step().
    pub(crate) const ROW: i32 = 100;
    /// End the execution of function: sqlite3_step().
    pub(crate) const DONE: i32 = 101;
}

/// Returns the symbolic name of a SQLite error code for logging.
pub(crate) fn sqlite_err_name(ret: i32) -> &'static str {
    match ret {
        SqliteErr::OK => "SQLITE_OK",
        SqliteErr::BUSY => "SQLITE_BUSY",
        SqliteErr::READONLY => "SQLITE_READONLY",
        SqliteErr::IOERR => "SQLITE_IOERR",
        SqliteErr::CORRUPT => "SQLITE_CORRUPT",
        SqliteErr::FULL => "SQLITE_FULL",
        SqliteErr::CANTOPEN => "SQLITE_CANTOPEN",
        SqliteErr::NOLFS => "SQLITE_NOLFS",
        SqliteErr::NOTADB => "SQLITE_NOTADB",
        SqliteErr::NOMEM => "SQLITE_NOMEM",
        SqliteErr::ROW => "SQLITE_ROW",
        SqliteErr::DONE => "SQLITE_DONE",
        _ => "SQLITE_UNKNOWN",
    }
}

pub(crate) fn sqlite_err_handle(ret: i32) -> ErrCode {
    match ret {
        SqliteErr::CORRUPT | SqliteErr::NOTADB => ErrCode::DataCorrupted,
        SqliteErr::NOMEM => ErrCode::OutOfMemory,
        code => {
            if matches!(code, SqliteErr::BUSY | SqliteErr::READONLY | SqliteErr::FULL) {
                logw!("[DB]SQLite error {} mapped to DatabaseError", sqlite_err_name(code));
            } else {
                loge!("[DB]SQLite error {} mapped to DatabaseError", sqlite_err_name(code));
            }
            ErrCode::DatabaseError
        }
    }
}
