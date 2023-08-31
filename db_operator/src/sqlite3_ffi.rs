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

/// sqlite error type
pub type SqliteErrcode = i32;

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

extern {
    /// c wrap func
    pub fn sqlite3_open(
        filename: *const u8,    /* Database filename (UTF-8) */
        ppDb: *mut *mut c_void, /* OUT: SQLite db handle */
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_open_v2(
        filename: *const u8,    /* Database filename (UTF-8) */
        ppDb: *mut *mut c_void, /* OUT: SQLite db handle */
        flags: i32,             /* Flags */
        zVfs: *const u8,        /* Name of VFS module to use */
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_close(db: *mut c_void) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_close_v2(db: *mut c_void) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_exec(
        db: *mut c_void,                    /* An open database */
        sql: *const u8,                     /* SQL to be evaluated */
        callback: Option<sqlite3_callback>, /* Callback function */
        data: *mut c_void,                  /* 1st argument to callback */
        msg: *mut *mut u8,                  /* Error msg written here */
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_finalize(stmt: *mut c_void) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_free(data: *mut c_void);

    /// c wrap func
    pub fn sqlite3_changes(db: *mut c_void) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_prepare(
        db: *mut c_void,          /* Database handle */
        zSql: *const u8,          /* SQL statement, UTF-8 encoded */
        nByte: i32,               /* Maximum length of zSql in bytes. */
        ppStmt: *mut *mut c_void, /* OUT: Statement handle */
        pzTail: *mut *mut u8,     /* OUT: Pointer to unused portion of zSql */
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_prepare_v2(
        db: *mut c_void,          /* Database handle */
        zSql: *const u8,          /* SQL statement, UTF-8 encoded */
        nByte: i32,               /* Maximum length of zSql in bytes. */
        ppStmt: *mut *mut c_void, /* OUT: Statement handle */
        pzTail: *mut *mut u8,     /* OUT: Pointer to unused portion of zSql */
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_bind_text(
        stmt: *mut c_void,
        index: i32,
        text: *const u8,
        size: i32,
        callback: Option<bind_callback>,
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_bind_blob(
        stmt: *mut c_void,
        index: i32,
        blob: *const u8,
        n: i32,
        callback: Option<bind_callback>,
    ) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_bind_double(stmt: *mut c_void, index: i32, value: f64) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_bind_int(stmt: *mut c_void, index: i32, value: i32) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_bind_null(stmt: *mut c_void, index: i32) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_bind_zeroblob(stmt: *mut c_void, index: i32, n: i32) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_errmsg(db: *mut c_void) -> *const u8;

    /// c wrap func
    pub fn sqlite3_step(stmt: *mut c_void) -> SqliteErrcode;

    /// c wrap func
    pub fn sqlite3_column_count(stmt: *mut c_void) -> i32;

    /// c wrap func
    pub fn sqlite3_column_name(stmt: *mut c_void, N: i32) -> *const u8;

    /// c wrap func
    pub fn sqlite3_data_count(stmt: *mut c_void) -> i32;

    /// c wrap func
    pub fn sqlite3_column_blob(stmt: *mut c_void, iCol: i32) -> *const u8;

    /// c wrap func
    pub fn sqlite3_column_double(stmt: *mut c_void, iCol: i32) -> f64;

    /// c wrap func
    pub fn sqlite3_column_int(stmt: *mut c_void, iCol: i32) -> i32;

    /// c wrap func
    pub fn sqlite3_column_text(stmt: *mut c_void, iCol: i32) -> *const u8;

    /// c wrap func
    pub fn sqlite3_column_bytes(stmt: *mut c_void, iCol: i32) -> i32;

    /// c wrap func
    pub fn sqlite3_column_type(stmt: *mut c_void, iCol: i32) -> i32;

    /// c wrap func
    pub fn sqlite3_reset(stmt: *mut c_void) -> SqliteErrcode;
}

/// rust ffi func for C func
pub fn sqlite3_open_func(
    filename: &str,   /* Database filename (UTF-8) */
    ppDb: &mut usize, /* OUT: SQLite db handle */
) -> SqliteErrcode {
    unsafe { sqlite3_open(filename.as_ptr(), ppDb as *mut usize as _) }
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
    unsafe { sqlite3_open_v2(filename.as_ptr(), ppDb as *mut usize as _, flags, addr) }
}

/// rust ffi func for C func
pub fn sqlite3_close_func(db: usize) -> SqliteErrcode {
    unsafe { sqlite3_close(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_close_v2_func(db: usize) -> SqliteErrcode {
    unsafe { sqlite3_close_v2(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_exec_func(
    db: usize,                          /* An open database */
    sql: &str,                          /* SQL to be evaluated */
    callback: Option<sqlite3_callback>, /* Callback function */
    data: usize,                        /* 1st argument to callback */
    msg: &mut *mut u8,                  /* Error msg written here */
) -> SqliteErrcode {
    unsafe { sqlite3_exec(db as _, sql.as_ptr(), callback, data as _, msg as _) }
}

/// rust ffi func for C func
pub fn sqlite3_finalize_func(stat: usize) -> SqliteErrcode {
    unsafe { sqlite3_finalize(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_free_func(data: usize) {
    unsafe { sqlite3_free(data as _) }
}

/// rust ffi func for C func
pub fn sqlite3_changes_func(db: usize) -> SqliteErrcode {
    unsafe { sqlite3_changes(db as _) }
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
        sqlite3_prepare(
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
        sqlite3_prepare_v2(
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
    unsafe { sqlite3_bind_text(stat as _, index, text.as_ptr(), size, callback) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_blob_func(
    stat: usize,
    index: i32,
    blob: &[u8],
    n: i32,
    callback: Option<bind_callback>,
) -> SqliteErrcode {
    unsafe { sqlite3_bind_blob(stat as _, index, blob.as_ptr(), n, callback) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_double_func(stat: usize, index: i32, value: f64) -> SqliteErrcode {
    unsafe { sqlite3_bind_double(stat as _, index, value) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_int_func(stat: usize, index: i32, value: i32) -> SqliteErrcode {
    unsafe { sqlite3_bind_int(stat as _, index, value) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_null_func(stat: usize, index: i32) -> SqliteErrcode {
    unsafe { sqlite3_bind_null(stat as _, index) }
}

/// rust ffi func for C func
pub fn sqlite3_bind_zeroblob_func(stat: usize, index: i32, n: i32) -> SqliteErrcode {
    unsafe { sqlite3_bind_zeroblob(stat as _, index, n) }
}

/// rust ffi func for C func
pub fn sqlite3_errmsg_func(db: usize) -> *const u8 {
    unsafe { sqlite3_errmsg(db as _) }
}

/// rust ffi func for C func
pub fn sqlite3_step_func(stat: usize) -> SqliteErrcode {
    unsafe { sqlite3_step(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_column_count_func(stat: usize) -> i32 {
    unsafe { sqlite3_column_count(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_column_name_func(stat: usize, n: i32) -> *const u8 {
    unsafe { sqlite3_column_name(stat as _, n) }
}

/// rust ffi func for C func
pub fn sqlite3_data_count_func(stat: usize) -> i32 {
    unsafe { sqlite3_data_count(stat as _) }
}

/// rust ffi func for C func
pub fn sqlite3_column_blob_func(stmt: usize, iCol: i32) -> *const u8 {
    unsafe { sqlite3_column_blob(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_double_func(stmt: usize, iCol: i32) -> f64 {
    unsafe { sqlite3_column_double(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_int_func(stmt: usize, iCol: i32) -> i32 {
    unsafe { sqlite3_column_int(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_text_func(stmt: usize, iCol: i32) -> *const u8 {
    unsafe { sqlite3_column_text(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_bytes_func(stmt: usize, iCol: i32) -> i32 {
    unsafe { sqlite3_column_bytes(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_column_type_func(stmt: usize, iCol: i32) -> i32 {
    unsafe { sqlite3_column_type(stmt as _, iCol) }
}

/// rust ffi func for C func
pub fn sqlite3_reset_func(stmt: usize) -> SqliteErrcode {
    unsafe { sqlite3_reset(stmt as _) }
}
