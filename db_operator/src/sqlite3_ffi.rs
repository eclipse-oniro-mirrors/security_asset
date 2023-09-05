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
use core::ffi::c_void;
use std::ptr::null;

use asset_common_lib::asset_type::AssetStatusCode;

/// sqlite error type
pub type SqliteErrcode = i32;

impl From<SqliteErrcode> for AssetStatusCode {
    fn from(value: SqliteErrcode) -> Self {
        match value {
            SQLITE_OK => AssetStatusCode::Success,
            SQLITE_ERROR => AssetStatusCode::SqliteERROR,
            SQLITE_INTERNAL => AssetStatusCode::SqliteINTERNAL,
            SQLITE_PERM => Self::SqlitePERM,
            SQLITE_ABORT => AssetStatusCode::SqliteABORT,
            SQLITE_BUSY => AssetStatusCode::SqliteBUSY,
            SQLITE_LOCKED => AssetStatusCode::SqliteLOCKED,
            SQLITE_NOMEM => Self::SqliteNOMEM,
            SQLITE_READONLY => AssetStatusCode::SqliteREADONLY,
            SQLITE_INTERRUPT => AssetStatusCode::SqliteINTERRUPT,
            SQLITE_IOERR => AssetStatusCode::SqliteIOERR,
            SQLITE_CORRUPT => AssetStatusCode::SqliteCORRUPT,
            SQLITE_NOTFOUND => AssetStatusCode::SqliteNOTFOUND,
            SQLITE_FULL => AssetStatusCode::SqliteFULL,
            SQLITE_CANTOPEN => AssetStatusCode::SqliteCANTOPEN,
            SQLITE_PROTOCOL => AssetStatusCode::SqlitePROTOCOL,
            SQLITE_EMPTY => AssetStatusCode::SqliteEMPTY,
            SQLITE_SCHEMA => AssetStatusCode::SqliteSCHEMA,
            SQLITE_TOOBIG => AssetStatusCode::SqliteTOOBIG,
            SQLITE_CONSTRAINT => AssetStatusCode::SqliteCONSTRAINT,
            SQLITE_MISMATCH => AssetStatusCode::SqliteMISMATCH,
            SQLITE_MISUSE => AssetStatusCode::SqliteMISUSE,
            SQLITE_NOLFS => AssetStatusCode::SqliteNOLFS,
            SQLITE_AUTH => AssetStatusCode::SqliteAUTH,
            SQLITE_FORMAT => AssetStatusCode::SqliteFORMAT,
            SQLITE_RANGE => AssetStatusCode::SqliteRANGE,
            SQLITE_NOTADB => AssetStatusCode::SqliteNOTADB,
            SQLITE_NOTICE => AssetStatusCode::SqliteNOTICE,
            SQLITE_WARNING => AssetStatusCode::SqliteWARNING,
            SQLITE_ROW => AssetStatusCode::SqliteROW,
            SQLITE_DONE => AssetStatusCode::SqliteDONE,
            _ => AssetStatusCode::SqliteERROR,
        }
    }
}

/// Successful result
pub const SQLITE_OK: i32 = 0;
/// Generic error
pub const SQLITE_ERROR: i32 = 1;
/// Internal logic error in SQLite
pub const SQLITE_INTERNAL: i32 = 2;
/// Access permission denied
pub const SQLITE_PERM: i32 = 3;
/// Callback routine requested an abort
pub const SQLITE_ABORT: i32 = 4;
/// The database file is locked
pub const SQLITE_BUSY: i32 = 5;
/// A table in the database is locked
pub const SQLITE_LOCKED: i32 = 6;
/// A malloc() failed
pub const SQLITE_NOMEM: i32 = 7;
/// Attempt to write a readonly database
pub const SQLITE_READONLY: i32 = 8;
/// Operation terminated by sqlite3_interrupt()
pub const SQLITE_INTERRUPT: i32 = 9;
/// Some kind of disk I/O error occurred
pub const SQLITE_IOERR: i32 = 10;
/// The database disk image is malformed
pub const SQLITE_CORRUPT: i32 = 11;
/// Unknown opcode in sqlite3_file_control()
pub const SQLITE_NOTFOUND: i32 = 12;
/// Insertion failed because database is full
pub const SQLITE_FULL: i32 = 13;
/// Unable to open the database file
pub const SQLITE_CANTOPEN: i32 = 14;
/// Database lock protocol error
pub const SQLITE_PROTOCOL: i32 = 15;
/// Internal use only
pub const SQLITE_EMPTY: i32 = 16;
/// The database schema changed
pub const SQLITE_SCHEMA: i32 = 17;
/// String or BLOB exceeds size limit
pub const SQLITE_TOOBIG: i32 = 18;
/// Abort due to constraint violation
pub const SQLITE_CONSTRAINT: i32 = 19;
/// Data type mismatch
pub const SQLITE_MISMATCH: i32 = 20;
/// Library used incorrectly
pub const SQLITE_MISUSE: i32 = 21;
/// Uses OS features not supported on host
pub const SQLITE_NOLFS: i32 = 22;
/// Authorization denied
pub const SQLITE_AUTH: i32 = 23;
/// Not used
pub const SQLITE_FORMAT: i32 = 24;
/// 2nd parameter to sqlite3_bind out of range
pub const SQLITE_RANGE: i32 = 25;
/// File opened that is not a database file
pub const SQLITE_NOTADB: i32 = 26;
/// Notifications from sqlite3_log()
pub const SQLITE_NOTICE: i32 = 27;
/// Warnings from sqlite3_log()
pub const SQLITE_WARNING: i32 = 28;
/// sqlite3_step() has another row ready
pub const SQLITE_ROW: i32 = 100;
/// sqlite3_step() has finished executing
pub const SQLITE_DONE: i32 = 101;

/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_READONLY: i32 = 0x00000001;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_READWRITE: i32 = 0x00000002;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_CREATE: i32 = 0x00000004;
/// VFS only
pub const SQLITE_OPEN_DELETEONCLOSE: i32 = 0x00000008;
/// VFS only
pub const SQLITE_OPEN_EXCLUSIVE: i32 = 0x00000010;
/// VFS only
pub const SQLITE_OPEN_AUTOPROXY: i32 = 0x00000020;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_URI: i32 = 0x00000040;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_MEMORY: i32 = 0x00000080;
/// VFS only
pub const SQLITE_OPEN_MAIN_DB: i32 = 0x00000100;
/// VFS only
pub const SQLITE_OPEN_TEMP_DB: i32 = 0x00000200;
/// VFS only
pub const SQLITE_OPEN_TRANSIENT_DB: i32 = 0x00000400;
/// VFS only
pub const SQLITE_OPEN_MAIN_JOURNAL: i32 = 0x00000800;
/// VFS only
pub const SQLITE_OPEN_TEMP_JOURNAL: i32 = 0x00001000;
/// VFS only
pub const SQLITE_OPEN_SUBJOURNAL: i32 = 0x00002000;
/// VFS only
pub const SQLITE_OPEN_MASTER_JOURNAL: i32 = 0x00004000;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_NOMUTEX: i32 = 0x00008000;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_FULLMUTEX: i32 = 0x00010000;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_SHAREDCACHE: i32 = 0x00020000;
/// Ok for sqlite3_open_v2()
pub const SQLITE_OPEN_PRIVATECACHE: i32 = 0x00040000;
/// VFS only
pub const SQLITE_OPEN_WAL: i32 = 0x00080000;

/// interger type
pub const SQLITE_INTEGER: i32 = 1;
/// float type
pub const SQLITE_FLOAT: i32 = 2;
/// blob type
pub const SQLITE_BLOB: i32 = 4;
/// null data
pub const SQLITE_NULL: i32 = 5;
/// string type
pub const SQLITE_TEXT: i32 = 3;
/// stirng type
pub const SQLITE3_TEXT: i32 = 3;

///
/// data: pointer passed by sqlite3_exec
/// argc: count of ResultSet
/// argv: Result
/// azColName: Column names
///
pub type sqlite3_callback = extern fn(
    data: *mut c_void,
    argc: i32,
    argv: *const *const u8,
    azColName: *const *const u8,
) -> SqliteErrcode;

/// callback func for bind data
pub type bind_callback = extern fn(p: *mut c_void);

/// c wrap func
pub type sqlite3_open_ctype = extern fn(
    filename: *const u8,    /* Database filename (UTF-8) */
    ppDb: *mut *mut c_void, /* OUT: SQLite db handle */
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_open_v2_ctype = extern fn(
    filename: *const u8,    /* Database filename (UTF-8) */
    ppDb: *mut *mut c_void, /* OUT: SQLite db handle */
    flags: i32,             /* Flags */
    zVfs: *const u8,        /* Name of VFS module to use */
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_close_ctype = extern fn(db: *mut c_void) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_close_v2_ctype = extern fn(db: *mut c_void) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_exec_ctype = extern fn(
    db: *mut c_void,                    /* An open database */
    sql: *const u8,                     /* SQL to be evaluated */
    callback: Option<sqlite3_callback>, /* Callback function */
    data: *mut c_void,                  /* 1st argument to callback */
    msg: *mut *mut u8,                  /* Error msg written here */
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_finalize_ctype = extern fn(stmt: *mut c_void) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_free_ctype = extern fn(data: *mut c_void);

/// c wrap func
pub type sqlite3_changes_ctype = extern fn(db: *mut c_void) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_prepare_ctype = extern fn(
    db: *mut c_void,          /* Database handle */
    zSql: *const u8,          /* SQL statement, UTF-8 encoded */
    nByte: i32,               /* Maximum length of zSql in bytes. */
    ppStmt: *mut *mut c_void, /* OUT: Statement handle */
    pzTail: *mut *mut u8,     /* OUT: Pointer to unused portion of zSql */
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_prepare_v2_ctype = extern fn(
    db: *mut c_void,          /* Database handle */
    zSql: *const u8,          /* SQL statement, UTF-8 encoded */
    nByte: i32,               /* Maximum length of zSql in bytes. */
    ppStmt: *mut *mut c_void, /* OUT: Statement handle */
    pzTail: *mut *mut u8,     /* OUT: Pointer to unused portion of zSql */
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_bind_text_ctype = extern fn(
    stmt: *mut c_void,
    index: i32,
    text: *const u8,
    size: i32,
    callback: Option<bind_callback>,
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_bind_blob_ctype = extern fn(
    stmt: *mut c_void,
    index: i32,
    blob: *const u8,
    n: i32,
    callback: Option<bind_callback>,
) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_bind_double_ctype =
    extern fn(stmt: *mut c_void, index: i32, value: f64) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_bind_int_ctype =
    extern fn(stmt: *mut c_void, index: i32, value: i32) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_bind_null_ctype = extern fn(stmt: *mut c_void, index: i32) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_bind_zeroblob_ctype =
    extern fn(stmt: *mut c_void, index: i32, n: i32) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_errmsg_ctype = extern fn(db: *mut c_void) -> *const u8;

/// c wrap func
pub type sqlite3_step_ctype = extern fn(stmt: *mut c_void) -> SqliteErrcode;

/// c wrap func
pub type sqlite3_column_count_ctype = extern fn(stmt: *mut c_void) -> i32;

/// c wrap func
pub type sqlite3_column_name_ctype = extern fn(stmt: *mut c_void, N: i32) -> *const u8;

/// c wrap func
pub type sqlite3_data_count_ctype = extern fn(stmt: *mut c_void) -> i32;

/// c wrap func
pub type sqlite3_column_blob_ctype = extern fn(stmt: *mut c_void, iCol: i32) -> *const u8;

/// c wrap func
pub type sqlite3_column_double_ctype = extern fn(stmt: *mut c_void, iCol: i32) -> f64;

/// c wrap func
pub type sqlite3_column_int_ctype = extern fn(stmt: *mut c_void, iCol: i32) -> i32;

/// c wrap func
pub type sqlite3_column_text_ctype = extern fn(stmt: *mut c_void, iCol: i32) -> *const u8;

/// c wrap func
pub type sqlite3_column_bytes_ctype = extern fn(stmt: *mut c_void, iCol: i32) -> i32;

/// c wrap func
pub type sqlite3_column_type_ctype = extern fn(stmt: *mut c_void, iCol: i32) -> i32;

/// c wrap func
pub type sqlite3_reset_ctype = extern fn(stmt: *mut c_void) -> SqliteErrcode;

/// global functions for sqlite3
#[repr(C)]
pub struct sqlite3_api_routines {
    /// sqlite3_c_func
    pub aggregate_context: *const c_void,
    /// sqlite3_c_func
    pub aggregate_count: *const c_void,
    /// sqlite3_c_func
    pub bind_blob: sqlite3_bind_blob_ctype,
    /// sqlite3_c_func
    pub bind_double: sqlite3_bind_double_ctype,
    /// sqlite3_c_func
    pub bind_int: sqlite3_bind_int_ctype,
    /// sqlite3_c_func
    pub bind_int64: *const c_void,
    /// sqlite3_c_func
    pub bind_null: sqlite3_bind_null_ctype,
    /// sqlite3_c_func
    pub bind_parameter_count: *const c_void,
    /// sqlite3_c_func
    pub bind_parameter_index: *const c_void,
    /// sqlite3_c_func
    pub bind_parameter_name: *const c_void,
    /// sqlite3_c_func
    pub bind_text: sqlite3_bind_text_ctype,
    /// sqlite3_c_func
    pub bind_text16: *const c_void,
    /// sqlite3_c_func
    pub bind_value: *const c_void,
    /// sqlite3_c_func
    pub busy_handler: *const c_void,
    /// sqlite3_c_func
    pub busy_timeout: *const c_void,
    /// sqlite3_c_func
    pub changes: sqlite3_changes_ctype,
    /// sqlite3_c_func
    pub close: sqlite3_close_ctype,
    /// sqlite3_c_func
    pub collation_needed: *const c_void,
    /// sqlite3_c_func
    pub collation_needed16: *const c_void,
    /// sqlite3_c_func
    pub column_blob: sqlite3_column_blob_ctype,
    /// sqlite3_c_func
    pub column_bytes: sqlite3_column_bytes_ctype,
    /// sqlite3_c_func
    pub column_bytes16: *const c_void,
    /// sqlite3_c_func
    pub column_count: sqlite3_column_count_ctype,
    /// sqlite3_c_func
    pub column_database_name: *const c_void,
    /// sqlite3_c_func
    pub column_database_name16: *const c_void,
    /// sqlite3_c_func
    pub column_decltype: *const c_void,
    /// sqlite3_c_func
    pub column_decltype16: *const c_void,
    /// sqlite3_c_func
    pub column_double: sqlite3_column_double_ctype,
    /// sqlite3_c_func
    pub column_int: sqlite3_column_int_ctype,
    /// sqlite3_c_func
    pub column_int64: *const c_void,
    /// sqlite3_c_func
    pub column_name: sqlite3_column_name_ctype,
    /// sqlite3_c_func
    pub column_name16: *const c_void,
    /// sqlite3_c_func
    pub column_origin_name: *const c_void,
    /// sqlite3_c_func
    pub column_origin_name16: *const c_void,
    /// sqlite3_c_func
    pub column_table_name: *const c_void,
    /// sqlite3_c_func
    pub column_table_name16: *const c_void,
    /// sqlite3_c_func
    pub column_text: sqlite3_column_text_ctype,
    /// sqlite3_c_func
    pub column_text16: *const c_void,
    /// sqlite3_c_func
    pub column_type: sqlite3_column_type_ctype,
    /// sqlite3_c_func
    pub column_value: *const c_void,
    /// sqlite3_c_func
    pub commit_hook: *const c_void,
    /// sqlite3_c_func
    pub complete: *const c_void,
    /// sqlite3_c_func
    pub complete16: *const c_void,
    /// sqlite3_c_func
    pub create_collation: *const c_void,
    /// sqlite3_c_func
    pub create_collation16: *const c_void,
    /// sqlite3_c_func
    pub create_function: *const c_void,
    /// sqlite3_c_func
    pub create_function16: *const c_void,
    /// sqlite3_c_func
    pub create_module: *const c_void,
    /// sqlite3_c_func
    pub data_count: sqlite3_data_count_ctype,
    /// sqlite3_c_func
    pub db_handle: *const c_void,
    /// sqlite3_c_func
    pub declare_vtab: *const c_void,
    /// sqlite3_c_func
    pub enable_shared_cache: *const c_void,
    /// sqlite3_c_func
    pub errcode: *const c_void,
    /// sqlite3_c_func
    pub errmsg: sqlite3_errmsg_ctype,
    /// sqlite3_c_func
    pub errmsg16: *const c_void,
    /// sqlite3_c_func
    pub exec: sqlite3_exec_ctype,
    /// sqlite3_c_func
    pub expired: *const c_void,
    /// sqlite3_c_func
    pub finalize: sqlite3_finalize_ctype,
    /// sqlite3_c_func
    pub free: sqlite3_free_ctype,
    /// sqlite3_c_func
    pub free_table: *const c_void,
    /// sqlite3_c_func
    pub get_autocommit: *const c_void,
    /// sqlite3_c_func
    pub get_auxdata: *const c_void,
    /// sqlite3_c_func
    pub get_table: *const c_void,
    /// sqlite3_c_func
    pub global_recover: *const c_void,
    /// sqlite3_c_func
    pub interruptx: *const c_void,
    /// sqlite3_c_func
    pub last_insert_rowid: *const c_void,
    /// sqlite3_c_func
    pub libversion: *const c_void,
    /// sqlite3_c_func
    pub libversion_number: *const c_void,
    /// sqlite3_c_func
    pub malloc: *const c_void,
    /// sqlite3_c_func
    pub mprintf: *const c_void,
    /// sqlite3_c_func
    pub open: sqlite3_open_ctype,
    /// sqlite3_c_func
    pub open16: *const c_void,
    /// sqlite3_c_func
    pub prepare: sqlite3_prepare_ctype,
    /// sqlite3_c_func
    pub prepare16: *const c_void,
    /// sqlite3_c_func
    pub profile: *const c_void,
    /// sqlite3_c_func
    pub progress_handler: *const c_void,
    /// sqlite3_c_func
    pub realloc: *const c_void,
    /// sqlite3_c_func
    pub reset: sqlite3_reset_ctype,
    /// sqlite3_c_func
    pub result_blob: *const c_void,
    /// sqlite3_c_func
    pub result_double: *const c_void,
    /// sqlite3_c_func
    pub result_error: *const c_void,
    /// sqlite3_c_func
    pub result_error16: *const c_void,
    /// sqlite3_c_func
    pub result_int: *const c_void,
    /// sqlite3_c_func
    pub result_int64: *const c_void,
    /// sqlite3_c_func
    pub result_null: *const c_void,
    /// sqlite3_c_func
    pub result_text: *const c_void,
    /// sqlite3_c_func
    pub result_text16: *const c_void,
    /// sqlite3_c_func
    pub result_text16be: *const c_void,
    /// sqlite3_c_func
    pub result_text16le: *const c_void,
    /// sqlite3_c_func
    pub result_value: *const c_void,
    /// sqlite3_c_func
    pub rollback_hook: *const c_void,
    /// sqlite3_c_func
    pub set_authorizer: *const c_void,
    /// sqlite3_c_func
    pub set_auxdata: *const c_void,
    /// sqlite3_c_func
    pub xsnprintf: *const c_void,
    /// sqlite3_c_func
    pub step: sqlite3_step_ctype,
    /// sqlite3_c_func
    pub table_column_metadata: *const c_void,
    /// sqlite3_c_func
    pub thread_cleanup: *const c_void,
    /// sqlite3_c_func
    pub total_changes: *const c_void,
    /// sqlite3_c_func
    pub trace: *const c_void,
    /// sqlite3_c_func
    pub transfer_bindings: *const c_void,
    /// sqlite3_c_func
    pub update_hook: *const c_void,
    /// sqlite3_c_func
    pub user_data: *const c_void,
    /// sqlite3_c_func
    pub value_blob: *const c_void,
    /// sqlite3_c_func
    pub value_bytes: *const c_void,
    /// sqlite3_c_func
    pub value_bytes16: *const c_void,
    /// sqlite3_c_func
    pub value_double: *const c_void,
    /// sqlite3_c_func
    pub value_int: *const c_void,
    /// sqlite3_c_func
    pub value_int64: *const c_void,
    /// sqlite3_c_func
    pub value_numeric_type: *const c_void,
    /// sqlite3_c_func
    pub value_text: *const c_void,
    /// sqlite3_c_func
    pub value_text16: *const c_void,
    /// sqlite3_c_func
    pub value_text16be: *const c_void,
    /// sqlite3_c_func
    pub value_text16le: *const c_void,
    /// sqlite3_c_func
    pub value_type: *const c_void,
    /// sqlite3_c_func
    pub vmprintf: *const c_void,
    /// sqlite3_c_func
    pub overload_function: *const c_void,
    /// sqlite3_c_func
    pub prepare_v2: sqlite3_prepare_v2_ctype,
    /// sqlite3_c_func
    pub prepare16_v2: *const c_void,
    /// sqlite3_c_func
    pub clear_bindings: *const c_void,
    /// sqlite3_c_func
    pub create_module_v2: *const c_void,
    /// sqlite3_c_func
    pub bind_zeroblob: sqlite3_bind_zeroblob_ctype,
    /// sqlite3_c_func
    pub blob_bytes: *const c_void,
    /// sqlite3_c_func
    pub blob_close: *const c_void,
    /// sqlite3_c_func
    pub blob_open: *const c_void,
    /// sqlite3_c_func
    pub blob_read: *const c_void,
    /// sqlite3_c_func
    pub blob_write: *const c_void,
    /// sqlite3_c_func
    pub create_collation_v2: *const c_void,
    /// sqlite3_c_func
    pub file_control: *const c_void,
    /// sqlite3_c_func
    pub memory_highwater: *const c_void,
    /// sqlite3_c_func
    pub memory_used: *const c_void,
    /// sqlite3_c_func
    pub mutex_alloc: *const c_void,
    /// sqlite3_c_func
    pub mutex_enter: *const c_void,
    /// sqlite3_c_func
    pub mutex_free: *const c_void,
    /// sqlite3_c_func
    pub mutex_leave: *const c_void,
    /// sqlite3_c_func
    pub mutex_try: *const c_void,
    /// sqlite3_c_func
    pub open_v2: sqlite3_open_v2_ctype,
    /// sqlite3_c_func
    pub release_memory: *const c_void,
    /// sqlite3_c_func
    pub result_error_nomem: *const c_void,
    /// sqlite3_c_func
    pub result_error_toobig: *const c_void,
    /// sqlite3_c_func
    pub sleep: *const c_void,
    /// sqlite3_c_func
    pub soft_heap_limit: *const c_void,
    /// sqlite3_c_func
    pub vfs_find: *const c_void,
    /// sqlite3_c_func
    pub vfs_register: *const c_void,
    /// sqlite3_c_func
    pub vfs_unregister: *const c_void,
    /// sqlite3_c_func
    pub xthreadsafe: *const c_void,
    /// sqlite3_c_func
    pub result_zeroblob: *const c_void,
    /// sqlite3_c_func
    pub result_error_code: *const c_void,
    /// sqlite3_c_func
    pub test_control: *const c_void,
    /// sqlite3_c_func
    pub randomness: *const c_void,
    /// sqlite3_c_func
    pub context_db_handle: *const c_void,
    /// sqlite3_c_func
    pub extended_result_codes: *const c_void,
    /// sqlite3_c_func
    pub limit: *const c_void,
    /// sqlite3_c_func
    pub next_stmt: *const c_void,
    /// sqlite3_c_func
    pub sql: *const c_void,
    /// sqlite3_c_func
    pub status: *const c_void,
    /// sqlite3_c_func
    pub backup_finish: *const c_void,
    /// sqlite3_c_func
    pub backup_init: *const c_void,
    /// sqlite3_c_func
    pub backup_pagecount: *const c_void,
    /// sqlite3_c_func
    pub backup_remaining: *const c_void,
    /// sqlite3_c_func
    pub backup_step: *const c_void,
    /// sqlite3_c_func
    pub compileoption_get: *const c_void,
    /// sqlite3_c_func
    pub compileoption_used: *const c_void,
    /// sqlite3_c_func
    pub create_function_v2: *const c_void,
    /// sqlite3_c_func
    pub db_config: *const c_void,
    /// sqlite3_c_func
    pub db_mutex: *const c_void,
    /// sqlite3_c_func
    pub db_status: *const c_void,
    /// sqlite3_c_func
    pub extended_errcode: *const c_void,
    /// sqlite3_c_func
    pub log: *const c_void,
    /// sqlite3_c_func
    pub soft_heap_limit64: *const c_void,
    /// sqlite3_c_func
    pub sourceid: *const c_void,
    /// sqlite3_c_func
    pub stmt_status: *const c_void,
    /// sqlite3_c_func
    pub strnicmp: *const c_void,
    /// sqlite3_c_func
    pub unlock_notify: *const c_void,
    /// sqlite3_c_func
    pub wal_autocheckpoint: *const c_void,
    /// sqlite3_c_func
    pub wal_checkpoint: *const c_void,
    /// sqlite3_c_func
    pub wal_hook: *const c_void,
    /// sqlite3_c_func
    pub blob_reopen: *const c_void,
    /// sqlite3_c_func
    pub vtab_config: *const c_void,
    /// sqlite3_c_func
    pub vtab_on_conflict: *const c_void,
    /// sqlite3_c_func
    pub close_v2: sqlite3_close_v2_ctype,
    /// sqlite3_c_func
    pub db_filename: *const c_void,
    /// sqlite3_c_func
    pub db_readonly: *const c_void,
    /// sqlite3_c_func
    pub db_release_memory: *const c_void,
    /// sqlite3_c_func
    pub errstr: *const c_void,
    /// sqlite3_c_func
    pub stmt_busy: *const c_void,
    /// sqlite3_c_func
    pub stmt_readonly: *const c_void,
    /// sqlite3_c_func
    pub stricmp: *const c_void,
    /// sqlite3_c_func
    pub uri_boolean: *const c_void,
    /// sqlite3_c_func
    pub uri_int64: *const c_void,
    /// sqlite3_c_func
    pub uri_parameter: *const c_void,
    /// sqlite3_c_func
    pub xvsnprintf: *const c_void,
    /// sqlite3_c_func
    pub wal_checkpoint_v2: *const c_void,
    /// sqlite3_c_func
    pub auto_extension: *const c_void,
    /// sqlite3_c_func
    pub bind_blob64: *const c_void,
    /// sqlite3_c_func
    pub bind_text64: *const c_void,
    /// sqlite3_c_func
    pub cancel_auto_extension: *const c_void,
    /// sqlite3_c_func
    pub load_extension: *const c_void,
    /// sqlite3_c_func
    pub malloc64: *const c_void,
    /// sqlite3_c_func
    pub msize: *const c_void,
    /// sqlite3_c_func
    pub realloc64: *const c_void,
    /// sqlite3_c_func
    pub reset_auto_extension: *const c_void,
    /// sqlite3_c_func
    pub result_blob64: *const c_void,
    /// sqlite3_c_func
    pub result_text64: *const c_void,
    /// sqlite3_c_func
    pub strglob: *const c_void,
    /// sqlite3_c_func
    pub value_dup: *const c_void,
    /// sqlite3_c_func
    pub value_free: *const c_void,
    /// sqlite3_c_func
    pub result_zeroblob64: *const c_void,
    /// sqlite3_c_func
    pub bind_zeroblob64: *const c_void,
    /// sqlite3_c_func
    pub value_subtype: *const c_void,
    /// sqlite3_c_func
    pub result_subtype: *const c_void,
    /// sqlite3_c_func
    pub status64: *const c_void,
    /// sqlite3_c_func
    pub strlike: *const c_void,
    /// sqlite3_c_func
    pub db_cacheflush: *const c_void,
    /// sqlite3_c_func
    pub system_errno: *const c_void,
    /// sqlite3_c_func
    pub trace_v2: *const c_void,
    /// sqlite3_c_func
    pub expanded_sql: *const c_void,
    /// sqlite3_c_func
    pub set_last_insert_rowid: *const c_void,
    /// sqlite3_c_func
    pub prepare_v3: *const c_void,
    /// sqlite3_c_func
    pub prepare16_v3: *const c_void,
    /// sqlite3_c_func
    pub bind_pointer: *const c_void,
    /// sqlite3_c_func
    pub result_pointer: *const c_void,
    /// sqlite3_c_func
    pub value_pointer: *const c_void,
    /// sqlite3_c_func
    pub vtab_nochange: *const c_void,
    /// sqlite3_c_func
    pub value_nochange: *const c_void,
    /// sqlite3_c_func
    pub vtab_collation: *const c_void,
    /// sqlite3_c_func
    pub keyword_count: *const c_void,
    /// sqlite3_c_func
    pub keyword_name: *const c_void,
    /// sqlite3_c_func
    pub keyword_check: *const c_void,
    /// sqlite3_c_func
    pub str_new: *const c_void,
    /// sqlite3_c_func
    pub str_finish: *const c_void,
    /// sqlite3_c_func
    pub str_appendf: *const c_void,
    /// sqlite3_c_func
    pub str_vappendf: *const c_void,
    /// sqlite3_c_func
    pub str_append: *const c_void,
    /// sqlite3_c_func
    pub str_appendall: *const c_void,
    /// sqlite3_c_func
    pub str_appendchar: *const c_void,
    /// sqlite3_c_func
    pub str_reset: *const c_void,
    /// sqlite3_c_func
    pub str_errcode: *const c_void,
    /// sqlite3_c_func
    pub str_length: *const c_void,
    /// sqlite3_c_func
    pub str_value: *const c_void,
    /// sqlite3_c_func
    pub create_window_function: *const c_void,
    /// sqlite3_c_func
    pub normalized_sql: *const c_void,
    /// sqlite3_c_func
    pub stmt_isexplain: *const c_void,
    /// sqlite3_c_func
    pub value_frombind: *const c_void,
    /// sqlite3_c_func
    pub drop_modules: *const c_void,
    /// sqlite3_c_func
    pub hard_heap_limit64: *const c_void,
    /// sqlite3_c_func
    pub uri_key: *const c_void,
    /// sqlite3_c_func
    pub filename_database: *const c_void,
    /// sqlite3_c_func
    pub filename_journal: *const c_void,
    /// sqlite3_c_func
    pub filename_wal: *const c_void,
    /// sqlite3_c_func
    pub create_filename: *const c_void,
    /// sqlite3_c_func
    pub free_filename: *const c_void,
    /// sqlite3_c_func
    pub database_file_object: *const c_void,
    /// sqlite3_c_func
    pub txn_state: *const c_void,
    /// sqlite3_c_func
    pub changes64: *const c_void,
    /// sqlite3_c_func
    pub total_changes64: *const c_void,
    /// sqlite3_c_func
    pub autovacuum_pages: *const c_void,
    /// sqlite3_c_func
    pub error_offset: *const c_void,
    /// sqlite3_c_func
    pub vtab_rhs_value: *const c_void,
    /// sqlite3_c_func
    pub vtab_distinct: *const c_void,
    /// sqlite3_c_func
    pub vtab_in: *const c_void,
    /// sqlite3_c_func
    pub vtab_in_first: *const c_void,
    /// sqlite3_c_func
    pub vtab_in_next: *const c_void,
    /// sqlite3_c_func
    pub deserialize: *const c_void,
    /// sqlite3_c_func
    pub serialize: *const c_void,
    /// sqlite3_c_func
    pub db_name: *const c_void,
    /// sqlite3_c_func
    pub value_encoding: *const c_void,
    /// sqlite3_c_func
    pub set_droptable_handle: *const c_void,
}

extern {
    /// sqlite3 export symbols by struct, the functions is the member.
    static sqlite3_export_symbols: *const sqlite3_api_routines;
}

/// rust ffi func for C func
pub fn sqlite3_open_func(
    filename: &str,   /* Database filename (UTF-8) */
    ppDb: &mut usize, /* OUT: SQLite db handle */
) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).open)(filename.as_ptr(), ppDb as *mut usize as _) }
}

/// rust ffi func for C func
pub fn sqlite3_open_v2_func(
    filename: &str,      /* Database filename (UTF-8) */
    ppDb: &mut usize,    /* OUT: SQLite db handle */
    flags: i32,          /* Flags */
    zVfs: Option<&[u8]>, /* Name of VFS module to use */
) -> SqliteErrcode {
    let addr = match zVfs {
        Some(v) => v.as_ptr(),
        None => null(),
    };
    unsafe {
        ((*sqlite3_export_symbols).open_v2)(filename.as_ptr(), ppDb as *mut usize as _, flags, addr)
    }
}

/// rust ffi func for C func
pub fn sqlite3_close_func(db: usize) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).close)(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_close_v2_func(db: usize) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).close_v2)(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_exec_func(
    db: usize,                          /* An open database */
    sql: &str,                          /* SQL to be evaluated */
    callback: Option<sqlite3_callback>, /* Callback function */
    data: usize,                        /* 1st argument to callback */
    msg: &mut *mut u8,                  /* Error msg written here */
) -> SqliteErrcode {
    unsafe {
        ((*sqlite3_export_symbols).exec)(db as _, sql.as_ptr(), callback, data as _, msg as _)
    }
}

/// rust ffi func for C func
pub fn sqlite3_finalize_func(stat: usize) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).finalize)(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_free_func(data: usize) {
    unsafe { ((*sqlite3_export_symbols).free)(data as _) }
}

/// rust ffi func for C func
pub fn sqlite3_changes_func(db: usize) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).changes)(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_prepare_func(
    db: usize,          /* Database handle */
    zSql: &[u8],        /* SQL statement, UTF-8 encoded */
    nByte: i32,         /* Maximum length of zSql in bytes. */
    ppStmt: &mut usize, /* OUT: Statement handle */
    pzTail: &mut usize, /* OUT: Pointer to unused portion of zSql */
) -> SqliteErrcode {
    unsafe {
        ((*sqlite3_export_symbols).prepare)(
            db as _,
            zSql.as_ptr(),
            nByte,
            ppStmt as *mut usize as _,
            pzTail as *mut usize as _,
        )
    }
}

/// rust ffi func for C func
pub fn sqlite3_prepare_v2_func(
    db: usize,          /* Database handle */
    zSql: &str,         /* SQL statement, UTF-8 encoded */
    nByte: i32,         /* Maximum length of zSql in bytes. */
    ppStmt: &mut usize, /* OUT: Statement handle */
    pzTail: &mut usize, /* OUT: Pointer to unused portion of zSql */
) -> SqliteErrcode {
    unsafe {
        ((*sqlite3_export_symbols).prepare_v2)(
            db as _,
            zSql.as_ptr(),
            nByte,
            ppStmt as *mut usize as _,
            pzTail as *mut usize as _,
        )
    }
}

/// rust ffi func for C func
pub fn sqlite3_bind_text_func(
    stat: usize,
    index: i32,
    text: &[u8],
    size: i32,
    callback: Option<bind_callback>,
) -> SqliteErrcode {
    unsafe {
        ((*sqlite3_export_symbols).bind_text)(stat as _, index, text.as_ptr(), size, callback)
    }
}

/// rust ffi func for C func
pub fn sqlite3_bind_blob_func(
    stat: usize,
    index: i32,
    blob: &[u8],
    n: i32,
    callback: Option<bind_callback>,
) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).bind_blob)(stat as _, index, blob.as_ptr(), n, callback) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_double_func(stat: usize, index: i32, value: f64) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).bind_double)(stat as _, index, value) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_int_func(stat: usize, index: i32, value: i32) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).bind_int)(stat as _, index, value) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_null_func(stat: usize, index: i32) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).bind_null)(stat as _, index) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_zeroblob_func(stat: usize, index: i32, n: i32) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).bind_zeroblob)(stat as _, index, n) }
}

/// rust ffi func for C func
pub fn sqlite3_errmsg_func(db: usize) -> *const u8 {
    unsafe { ((*sqlite3_export_symbols).errmsg)(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_step_func(stat: usize) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).step)(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_column_count_func(stat: usize) -> i32 {
    unsafe { ((*sqlite3_export_symbols).column_count)(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_column_name_func(stat: usize, n: i32) -> *const u8 {
    unsafe { ((*sqlite3_export_symbols).column_name)(stat as _, n) }
}

/// rust ffi func for C func
pub fn sqlite3_data_count_func(stat: usize) -> i32 {
    unsafe { ((*sqlite3_export_symbols).data_count)(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_column_blob_func(stmt: usize, iCol: i32) -> *const u8 {
    unsafe { ((*sqlite3_export_symbols).column_blob)(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_double_func(stmt: usize, iCol: i32) -> f64 {
    unsafe { ((*sqlite3_export_symbols).column_double)(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_int_func(stmt: usize, iCol: i32) -> i32 {
    unsafe { ((*sqlite3_export_symbols).column_int)(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_text_func(stmt: usize, iCol: i32) -> *const u8 {
    unsafe { ((*sqlite3_export_symbols).column_text)(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_bytes_func(stmt: usize, iCol: i32) -> i32 {
    unsafe { ((*sqlite3_export_symbols).column_bytes)(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_type_func(stmt: usize, iCol: i32) -> i32 {
    unsafe { ((*sqlite3_export_symbols).column_type)(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_reset_func(stmt: usize) -> SqliteErrcode {
    unsafe { ((*sqlite3_export_symbols).reset)(stmt as _) }
}
