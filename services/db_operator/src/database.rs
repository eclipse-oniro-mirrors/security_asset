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

//! This module provides interfaces for database management.
//! Databases are isolated based on users and protected by locks.

use core::ffi::c_void;
use std::{collections::{HashMap, HashSet}, ffi::CStr, fs, ptr::null_mut, sync::{Arc, Mutex}};

use asset_common::{CallingInfo, OwnerType};
use asset_crypto_manager::{
    crypto::Crypto, db_key_operator::generate_secret_key_if_needed, secret_key::{SecretKey, rename_key_alias}
};
use asset_definition::{
    macros_lib, ErrCode, Extension, Result, Value, AssetMap,
    Tag, SyncType, SyncStatus, ConflictResolution
};
use asset_log::{loge, logi};
use lazy_static::lazy_static;

use crate::{
    common::{build_secret_key, build_aad, get_query_condition},
    database_file_upgrade::{check_and_split_db, construct_splited_db_name},
    statement::Statement,
    table::Table,
    types::{
        column, sqlite_err_handle, DbMap, QueryOptions, SQLITE_OK, TABLE_NAME,
        ADAPT_CLOUD_COLUMN_INFO, ADAPT_CLOUD_TABLE, COLUMN_INFO, COMBINE_COLUMN_INFO,
        DB_UPGRADE_VERSION, DB_UPGRADE_VERSION_V0, DB_UPGRADE_VERSION_V1, DB_UPGRADE_VERSION_V2, DB_UPGRADE_VERSION_V3,
        UPGRADE_COLUMN_INFO, UPGRADE_COLUMN_INFO_V2, UPGRADE_COLUMN_INFO_V3, UPGRADE_COLUMN_INFO_V4
    },
    process_batch_data::{parse_attr_in_array, add_not_null_column}
};

extern "C" {
    fn SqliteOpen(file_name: *const u8, pp_db: *mut *mut c_void) -> i32;
    fn SqliteCloseV2(db: *mut c_void) -> i32;
    fn SqliteExec(db: *mut c_void, sql: *const u8, msg: *mut *mut u8) -> i32;
    fn SqliteFree(data: *mut c_void);
    fn SqliteErrMsg(db: *mut c_void) -> *const u8;
    fn SqliteKey(db: *mut c_void, pKey: *const c_void, nKey: i32) -> i32;
}

/// each user have a Database file
pub(crate) struct UserDbLock {
    pub(crate) mtx: Arc<Mutex<i32>>,
}

struct AdditionalInfo<'a> {
    attributes_array: &'a [AssetMap],
    db_map: &'a DbMap,
    calling_info: &'a CallingInfo,
    secret_key: &'a SecretKey,
}

pub(crate) static OLD_DB_NAME: &str = "asset";

lazy_static! {
    static ref SPLIT_DB_LOCK_MAP: Mutex<HashMap<i32, &'static UserDbLock>> = Mutex::new(HashMap::new());
    static ref USER_DB_LOCK_MAP: Mutex<HashMap<(i32, String), &'static UserDbLock>> = Mutex::new(HashMap::new());
}

pub(crate) fn get_split_db_lock_by_user_id(user_id: i32) -> &'static UserDbLock {
    let mut map = SPLIT_DB_LOCK_MAP.lock().unwrap();
    if let Some(&lock) = map.get(&user_id) {
        return lock;
    }

    let nf = Box::new(UserDbLock { mtx: Arc::new(Mutex::new(user_id)) });
    // SAFETY: We just push item into SPLIT_DB_LOCK_MAP, never remove item or modify item,
    // so return a reference of leak item is safe.
    let nf: &'static UserDbLock = Box::leak(nf);
    map.insert(user_id, nf);
    nf
}

/// If the user exists, the reference to the lock is returned.
/// Otherwise, a new lock is created and its reference is returned.
pub(crate) fn get_file_lock_by_user_id_db_file_name(user_id: i32, db_file_name: String) -> &'static UserDbLock {
    let mut map = USER_DB_LOCK_MAP.lock().unwrap();

    if let Some(&lock) = map.get(&(user_id, db_file_name.clone())) {
        return lock;
    }

    let nf = Box::new(UserDbLock { mtx: Arc::new(Mutex::new(user_id)) });
    // SAFETY: We just push item into USER_DB_LOCK_MAP, never remove item or modify item,
    // so return a reference of leak item is safe.
    let nf: &'static UserDbLock = Box::leak(nf);
    map.insert((user_id, db_file_name), nf);
    nf
}

/// Struct used to store database files and connection information.
#[repr(C)]
pub struct Database {
    pub(crate) path: String,
    pub(crate) backup_path: String,
    pub(crate) handle: usize, // Pointer to the database connection.
    pub(crate) db_lock: &'static UserDbLock,
    pub(crate) db_name: String,
    pub(crate) use_lock: bool,
}

/// Callback for database upgrade.
pub type UpgradeDbCallback = fn(db: &Database, old_ver: u32, new_ver: u32) -> Result<()>;

#[cfg(not(test))]
pub(crate) const DE_ROOT_PATH: &str = "/data/service/el1/public/asset_service";
#[cfg(test)]
pub(crate) const DE_ROOT_PATH: &str = "/data/asset_test";

pub(crate) const CE_ROOT_PATH: &str = "/data/service/el2";

#[inline(always)]
pub(crate) fn fmt_backup_path(path: &str) -> String {
    let mut bp = path.to_string();
    bp.push_str(".backup");
    bp
}

/// Get asset storage path.
pub fn get_path() -> String {
    DE_ROOT_PATH.to_string()
}

#[inline(always)]
pub(crate) fn fmt_ce_db_path_with_name(user_id: i32, db_name: &str) -> String {
    format!("data/service/el2/{}/asset_service/{}.db", user_id, db_name)
}

/// Fmt de path.
pub fn fmt_de_db_path_with_name(user_id: i32, db_name: &str) -> String {
    format!("{}/{}/{}.db", DE_ROOT_PATH, user_id, db_name)
}

fn fmt_db_path(user_id: i32, db_name: &str, db_key: &Option<Vec<u8>>) -> String {
    match db_key {
        Some(_db_key) => fmt_ce_db_path_with_name(user_id, db_name),
        _ => fmt_de_db_path_with_name(user_id, db_name),
    }
}

pub(crate) fn get_db_by_type_without_lock(
    user_id: i32,
    db_name: &str,
    db_path: String,
    db_key: Option<&Vec<u8>>,
) -> Result<Database> {
    let backup_path = fmt_backup_path(&db_path);
    let lock = get_file_lock_by_user_id_db_file_name(user_id, db_name.to_string().clone());
    let mut db = Database { path: db_path, backup_path, handle: 0, db_lock: lock, db_name: db_name.to_string(), use_lock: false };
    db.open_and_restore(db_key)?;
    // when create db table always use newest version.
    db.restore_if_exec_fail(|e: &Table| e.create_with_version(COLUMN_INFO, DB_UPGRADE_VERSION))?;
    db.upgrade(user_id, DB_UPGRADE_VERSION, |_, _, _| Ok(()))?;
    Ok(db)
}

pub(crate) fn get_db_by_type(
    user_id: i32,
    db_name: &str,
    db_path: String,
    db_key: Option<&Vec<u8>>,
) -> Result<Database> {
    let backup_path = fmt_backup_path(&db_path);
    let lock = get_file_lock_by_user_id_db_file_name(user_id, db_name.to_string().clone());
    let mut db = Database { path: db_path, backup_path, handle: 0, db_lock: lock, db_name: db_name.to_string(), use_lock: true };
    let _lock = db.db_lock.mtx.lock().unwrap();
    db.open_and_restore(db_key)?;
    // when create db table always use newest version.
    db.restore_if_exec_fail(|e: &Table| e.create_with_version(COLUMN_INFO, DB_UPGRADE_VERSION))?;
    db.upgrade(user_id, DB_UPGRADE_VERSION, |_, _, _| Ok(()))?;
    Ok(db)
}

pub(crate) fn get_specific_db_version(user_id: i32, db_name: &str, db_path: String) -> Result<u32> {
    let backup_path = fmt_backup_path(&db_path);
    let lock = get_file_lock_by_user_id_db_file_name(user_id, db_name.to_string().clone());
    let mut db = Database { path: db_path, backup_path, handle: 0, db_lock: lock, db_name: db_name.to_string(), use_lock: true };
    let _lock = db.db_lock.mtx.lock().unwrap();
    db.open()?;
    db.get_db_version()
}

pub(crate) fn get_db(user_id: i32, db_name: &str, db_key: &Option<Vec<u8>>) -> Result<Database> {
    let db_path = fmt_db_path(user_id, db_name, db_key);
    get_db_by_type(user_id, db_name, db_path, db_key.as_ref())
}

pub(crate) fn get_db_without_lock(user_id: i32, db_name: &str, db_key: &Option<Vec<u8>>) -> Result<Database> {
    let db_path = fmt_db_path(user_id, db_name, db_key);
    get_db_by_type_without_lock(user_id, db_name, db_path, db_key.as_ref())
}

impl Database {
    /// Create a database without a given file name.
    pub fn build(calling_info: &CallingInfo, db_key: Option<Vec<u8>>) -> Result<Database> {
        let is_ce: bool = db_key.is_some();
        if !is_ce {
            // DE database needs trigger the upgrade action.
            check_and_split_db(calling_info.user_id())?;
        }
        get_db(calling_info.user_id(), &construct_splited_db_name(calling_info, is_ce)?, &db_key)
    }

    /// Create a database from a file name.
    pub fn build_with_file_name(user_id: i32, db_name: &str, db_key: &Option<Vec<u8>>) -> Result<Database> {
        check_and_split_db(user_id)?;
        get_db(user_id, db_name, db_key)
    }

    /// Create a database from a file name without lock in full process.
    pub fn build_with_file_name_without_lock(user_id: i32, db_name: &str, db_key: &Option<Vec<u8>>) -> Result<Database> {
        // run here db must has been splited.
        get_db_without_lock(user_id, db_name, db_key)
    }

    /// Check whether db is ok
    pub fn check_db_accessible(path: String, user_id: i32, db_name: String, db_key: Option<&Vec<u8>>) -> Result<()> {
        let lock = get_file_lock_by_user_id_db_file_name(user_id, db_name.clone());
        let mut db = Database { path: path.clone(), backup_path: path, handle: 0, db_lock: lock, db_name, use_lock: true };
        if db_key.is_some() {
            db.open_and_restore(db_key)?
        } else {
            db.open()?;
        }
        let table = Table::new(TABLE_NAME, &db);
        table.create(COLUMN_INFO)
    }

    /// Open database connection.
    pub(crate) fn open(&mut self) -> Result<()> {
        let mut path_c = self.path.clone();
        path_c.push('\0');

        let ret = unsafe { SqliteOpen(path_c.as_ptr(), &mut self.handle as *mut usize as _) };
        if ret == SQLITE_OK {
            Ok(())
        } else {
            self.close();
            macros_lib::log_throw_error!(sqlite_err_handle(ret), "[FATAL][DB]Open database failed, err={}", ret)
        }
    }

    /// Open the database connection and restore the database if the connection fails.
    pub(crate) fn open_and_restore(&mut self, db_key: Option<&Vec<u8>>) -> Result<()> {
        let result = self.open();
        if let Some(db_key) = db_key {
            self.set_db_key(db_key)?;
        }
        let result = match result {
            Err(ret) if ret.code == ErrCode::DataCorrupted => self.restore(),
            ret => ret,
        };
        result
    }

    /// Get db name.
    pub(crate) fn get_db_name(&mut self) -> &str {
        &self.db_name
    }

    /// Close database connection.
    fn close(&mut self) {
        if self.handle != 0 {
            unsafe { SqliteCloseV2(self.handle as _) };
            self.handle = 0;
        }
    }

    /// Close database connection.
    pub(crate) fn close_db(&mut self) {
        if self.use_lock {
            let _lock = self.db_lock.mtx.lock().unwrap();
            self.close()
        } else {
            self.close()
        }
    }

    /// Encrypt/Decrypt CE database.
    pub fn set_db_key(&mut self, p_key: &Vec<u8>) -> Result<()> {
        let ret =
            unsafe { SqliteKey(self.handle as _, p_key.as_ptr() as *const c_void, p_key.len() as i32) };
        if ret == SQLITE_OK {
            Ok(())
        } else {
            macros_lib::log_throw_error!(sqlite_err_handle(ret), "[FATAL][DB]Set database key failed, err={}", ret)
        }
    }

    // Recovery the corrupt database and reopen it.
    pub(crate) fn restore(&mut self) -> Result<()> {
        loge!("[WARNING]Database is corrupt, start to restore");
        self.close();
        if let Err(e) = fs::copy(&self.backup_path, &self.path) {
            return macros_lib::log_throw_error!(ErrCode::FileOperationError, "[FATAL][DB]Recovery database failed, err={}", e);
        }
        self.open()
    }

    /// Get database version, default is 0.
    fn get_db_version(&self) -> Result<u32> {
        let stmt = Statement::prepare("pragma user_version", self)?;
        stmt.step()?;
        let version = stmt.query_column_int(0);
        Ok(version)
    }

    /// Get database version, default is 0.
    #[allow(dead_code)]
    pub(crate) fn get_version(&self) -> Result<u32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        self.get_db_version()
    }

    /// Update the database version for database upgrade.
    #[allow(dead_code)]
    pub(crate) fn set_version(&self, ver: u32) -> Result<()> {
        let sql = format!("pragma user_version = {}", ver);
        self.exec(sql.as_str())
    }

    /// Upgrade database to new version.
    #[allow(dead_code)]
    pub fn upgrade(&mut self, user_id: i32, target_ver: u32, callback: UpgradeDbCallback) -> Result<()> {
        let mut current_ver = self.get_db_version()?;
        if current_ver >= target_ver {
            return Ok(());
        }
        logi!("current database version: {}, target version: {}", current_ver, target_ver);
        while current_ver < target_ver {
            match current_ver {
                DB_UPGRADE_VERSION_V0 => {
                    self.restore_if_exec_fail(|e: &Table| e.upgrade(DB_UPGRADE_VERSION_V1, UPGRADE_COLUMN_INFO_V2))?;
                    current_ver += 1;
                },
                DB_UPGRADE_VERSION_V1 => {
                    self.restore_if_exec_fail(|e: &Table| e.upgrade(DB_UPGRADE_VERSION_V2, UPGRADE_COLUMN_INFO_V3))?;
                    current_ver += 1;
                },
                DB_UPGRADE_VERSION_V2 => {
                    if self.upgrade_key_alias(user_id)? {
                        self.restore_if_exec_fail(|e: &Table| {
                            e.upgrade(DB_UPGRADE_VERSION_V3, UPGRADE_COLUMN_INFO_V4)
                        })?;
                        current_ver += 1;
                    } else {
                        break;
                    }
                },
                DB_UPGRADE_VERSION_V3 => {
                    self.restore_if_exec_fail(|e: &Table| e.upgrade(DB_UPGRADE_VERSION, UPGRADE_COLUMN_INFO))?;
                    current_ver += 1;
                },
                _ => break,
            }
        }

        callback(self, current_ver, target_ver)
    }

    fn upgrade_key_alias(&mut self, user_id: i32) -> Result<bool> {
        let query_results = self.query_data_without_lock(
            &vec![
                column::OWNER_TYPE,
                column::OWNER,
                column::AUTH_TYPE,
                column::ACCESSIBILITY,
                column::REQUIRE_PASSWORD_SET,
            ],
            &DbMap::new(),
            None,
            true,
        )?;

        let mut upgrade_result = true;
        for query_result in query_results {
            let owner_type = query_result.get_enum_attr(&column::OWNER_TYPE)?;
            let owner_info = query_result.get_bytes_attr(&column::OWNER)?;
            let calling_info = CallingInfo::new(user_id, owner_type, owner_info.to_vec(), None);
            let auth_type = query_result.get_enum_attr(&column::AUTH_TYPE)?;
            let access_type = query_result.get_enum_attr(&column::ACCESSIBILITY)?;
            let require_password_set = query_result.get_bool_attr(&column::REQUIRE_PASSWORD_SET)?;
            // upgrade_result is set to false as long as any call in the loop for renaming key alias returned false.
            upgrade_result &= rename_key_alias(&calling_info, auth_type, access_type, require_password_set);
        }

        Ok(upgrade_result)
    }

    /// Delete database file.
    #[allow(dead_code)]
    pub(crate) fn delete(user_id: i32, db_name: &str) -> Result<()> {
        let path = fmt_de_db_path_with_name(user_id, db_name);
        let backup_path = fmt_backup_path(&path);
        if let Err(e) = fs::remove_file(path) {
            return macros_lib::log_throw_error!(ErrCode::FileOperationError, "[FATAL][DB]Delete database failed, err={}", e);
        }

        if let Err(e) = fs::remove_file(backup_path) {
            return macros_lib::log_throw_error!(
                ErrCode::FileOperationError,
                "[FATAL][DB]Delete backup database failed, err={}",
                e
            );
        }
        Ok(())
    }

    /// Print the error message of database.
    pub(crate) fn print_db_msg(&self) {
        let msg = unsafe { SqliteErrMsg(self.handle as _) };
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            if let Ok(rs) = s.to_str() {
                loge!("[FATAL][DB]Database error message: {}", rs);
            }
        }
    }

    /// execute sql without prepare
    pub fn exec(&self, sql: &str) -> Result<()> {
        let mut sql_s = sql.to_string();
        sql_s.push('\0');
        let mut msg: *mut u8 = null_mut();
        let ret = unsafe { SqliteExec(self.handle as _, sql_s.as_ptr(), &mut msg as _) };
        if !msg.is_null() {
            let s = unsafe { CStr::from_ptr(msg as _) };
            if let Ok(rs) = s.to_str() {
                return macros_lib::log_throw_error!(
                    sqlite_err_handle(ret),
                    "[FATAL]Database execute sql failed. error code={}, error msg={}",
                    ret,
                    rs
                );
            }
            unsafe { SqliteFree(msg as _) };
        }
        if ret == SQLITE_OK {
            Ok(())
        } else {
            macros_lib::log_throw_error!(sqlite_err_handle(ret), "[FATAL]Database execute sql failed. error code={}", ret)
        }
    }

    /// execute func in db, if failed and error code is data corrupted then restore
    pub(crate) fn restore_if_exec_fail<T, F: Fn(&Table) -> Result<T>>(&mut self, func: F) -> Result<T> {
        let table = Table::new(TABLE_NAME, self);
        let result = func(&table);
        match result {
            Err(ret) if ret.code == ErrCode::DataCorrupted => {
                self.restore()?;
                let table = Table::new(TABLE_NAME, self); // Database handle will be changed.
                func(&table)
            },
            ret => ret,
        }
    }

    /// Create adapt cloud table for adaptation.
    pub fn create_adapt_cloud_table(&mut self) -> Result<()> {
        let table = Table::new(ADAPT_CLOUD_TABLE, self);
        if table.exist()? {
            return Ok(())
        }
        table.create(ADAPT_CLOUD_COLUMN_INFO)
    }

    /// Insert datas into database.
    /// The datas is a map of column-data pair.
    /// If the operation is successful, the number of inserted data is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: insert into table_name(Owner,OwnerType,Alias,value) values('owner',1,'alias','insert_value')
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// datas.insert("value", Value::Bytes(b"insert_value".to_vec()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.insert_datas(&datas);
    /// ```
    ///
    #[inline(always)]
    pub fn insert_datas(&mut self, datas: &DbMap) -> Result<i32> {
        let _lock: std::sync::MutexGuard<'_, i32> = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| {
            let mut query = DbMap::new();
            query.insert_attr(column::ALIAS, datas.get_bytes_attr(&column::ALIAS)?.clone());
            query.insert_attr(column::OWNER, datas.get_bytes_attr(&column::OWNER)?.clone());
            query.insert_attr(column::OWNER_TYPE, datas.get_enum_attr::<OwnerType>(&column::OWNER_TYPE)?);
            if e.is_data_exists(&query, false)? {
                macros_lib::log_throw_error!(ErrCode::Duplicated, "[FATAL]The data with the specified alias already exists.")
            } else {
                e.insert_row(datas)
            }
        };
        self.restore_if_exec_fail(closure)
    }

    /// Insert datas in database with specific condition.
    /// If the operation is successful, the array of indexes failed to insert and corresponding reason is returned.
    #[inline(always)]
    pub fn insert_batch_datas(
        &mut self,
        db_map: &DbMap,
        attributes_array: &[AssetMap],
        calling_info: &CallingInfo,
    ) -> Result<Vec<(u32, u32)>> {
        let secret_key = build_secret_key(calling_info, db_map)?;
        generate_secret_key_if_needed(&secret_key)?;

        let mut db_datas = Vec::new();
        let mut err_info = Vec::new();
        let mut aliases = Vec::new();
        let info = AdditionalInfo {
            attributes_array,
            db_map,
            calling_info,
            secret_key: &secret_key
        };
	    let _lock = self.db_lock.mtx.lock().unwrap();
        let column_names = self.parse_attr_array(&mut db_datas, &mut err_info, &mut aliases, &info)?;
        if db_datas.is_empty() {
            return Ok(err_info);
        }

        let column_names = Vec::from_iter(column_names);
        let closure = |e: &Table| e.local_insert_batch_datas(&db_datas, db_map, &aliases, &column_names);
        self.restore_if_exec_fail(closure)?;
        Ok(err_info)
    }

    fn parse_attr_array(&mut self,
        db_datas: &mut Vec<DbMap>,
        err_info: &mut Vec<(u32, u32)>,
        aliases: &mut Vec<Vec<u8>>,
        info: &AdditionalInfo
    ) -> Result<HashSet<String>> {
        let mut column_names = HashSet::new();
        add_not_null_column(&mut column_names);
        for (index, attr) in info.attributes_array.iter().enumerate() {
            let mut db_data = parse_attr_in_array(attr, info.calling_info, &mut column_names)?;
            let query = get_query_condition(attr, info.calling_info)?;
            let mut condition = query.clone();
            condition.insert(column::SYNC_TYPE, Value::Number(SyncType::TrustedAccount as u32));
            condition.insert(column::SYNC_STATUS, Value::Number(SyncStatus::SyncDel as u32));
            if self.is_data_exists_without_lock(&query, false)? {
                match attr.get(&Tag::ConflictResolution) {
                    Some(Value::Number(num)) if *num == ConflictResolution::Overwrite as u32 => {},
                    _ => {
                        if !self.is_data_exists_without_lock(&condition, false)? {
                            err_info.push((ErrCode::Duplicated as u32, index as u32));
                            continue;
                        }
                    }
                }
            }
            if self.encrypt_single_data(&mut db_data, info.secret_key, aliases).is_err() {
                err_info.push((ErrCode::CryptoError as u32, index as u32));
                continue;
            }
            db_data.extend(info.db_map.clone());
            db_datas.push(db_data);
        }
        Ok(column_names)
    }

    fn encrypt_single_data(
        &mut self,
        db_data: &mut DbMap,
        secret_key: &SecretKey,
        aliases: &mut Vec<Vec<u8>>
    ) -> Result<()> {
        let secret = db_data.get_bytes_attr(&column::SECRET)?;
        let cipher = Crypto::encrypt(secret_key, secret, &build_aad(db_data)?)?;
        db_data.insert(column::SECRET, Value::Bytes(cipher));
        aliases.push(db_data.get_bytes_attr(&column::ALIAS)?.to_vec());
        Ok(())
    }

    /// Insert data in asset and adapt table.
    pub fn insert_cloud_adapt_data_without_lock(&mut self, datas: &DbMap, adapt_attributes: &DbMap) -> Result<i32> {
        let closure = |e: &Table| {
            let mut query = DbMap::new();
            query.insert_attr(column::ALIAS, datas.get_bytes_attr(&column::ALIAS)?.clone());
            query.insert_attr(column::OWNER, datas.get_bytes_attr(&column::OWNER)?.clone());
            query.insert_attr(column::OWNER_TYPE, datas.get_enum_attr::<OwnerType>(&column::OWNER_TYPE)?);
            if e.is_data_exists(&query, false)? {
                macros_lib::log_throw_error!(ErrCode::Duplicated, "[FATAL]The data with the specified alias already exists.")
            } else {
                e.insert_adapt_data_row(datas, adapt_attributes)
            }
        };
        self.restore_if_exec_fail(closure)
    }

    /// Insert data in asset and adapt table.
    pub fn insert_cloud_adapt_data(&mut self, datas: &DbMap, adapt_attributes: &DbMap) -> Result<i32> {
        let _lock: std::sync::MutexGuard<'_, i32> = self.db_lock.mtx.lock().unwrap();
        self.insert_cloud_adapt_data_without_lock(datas, adapt_attributes)
    }

    /// Delete datas from database.
    /// The condition is a map of column-data pair.
    /// If the operation is successful, the number of deleted data is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: delete from table_name where Owner='owner' and OwnerType=1 and Alias='alias' and value='delete_value'
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// datas.insert("value", Value::Bytes(b"delete_value".to_vec()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.delete_datas(&datas, None, false);
    /// ```
    ///
    ///
    #[inline(always)]
    pub fn delete_datas(
        &mut self,
        condition: &DbMap,
        reverse_condition: Option<&DbMap>,
        is_filter_sync: bool,
    ) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.delete_row(condition, reverse_condition, is_filter_sync);
        self.restore_if_exec_fail(closure)
    }

    /// Delete datas from database with specific condition.
    /// If the operation is successful, the number of deleted data is returned.
    #[inline(always)]
    pub fn delete_specific_condition_datas(&mut self, specific_cond: &str, condition_value: &[Value]) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.delete_with_specific_cond(specific_cond, condition_value);
        self.restore_if_exec_fail(closure)
    }

    /// Delete datas from database with specific condition.
    /// If the operation is successful, the number of deleted data is returned.
    #[inline(always)]
    pub fn delete_batch_datas(&mut self, condition: &DbMap, update_datas: &DbMap, aliases: &[Vec<u8>]) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.local_delete_batch_datas(condition, update_datas, aliases, true);
        self.restore_if_exec_fail(closure)
    }

    /// Delete datas from database with specific condition.
    pub fn delete_adapt_data_without_lock(
        &mut self,
        condition: Option<&DbMap>,
        adapt_attributes: Option<&DbMap>,
    ) -> Result<i32> {
        let closure = |e: &Table| e.delete_adapt_data_row(condition, adapt_attributes);
        self.restore_if_exec_fail(closure)
    }

    /// Delete datas from database with specific condition.
    pub fn delete_adapt_data(
        &mut self,
        condition: Option<&DbMap>,
        adapt_attributes: Option<&DbMap>,
    ) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        self.delete_adapt_data_without_lock(condition, adapt_attributes)
    }

    /// Update datas in database.
    /// The datas is a map of column-data pair.
    /// If the operation is successful, the number of updated data is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: update table_name set alias='update_value' where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let cond = DbMap.new();
    /// cond.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// cond.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let datas = DbMap::from([("alias", Value::Bytes(b"update_value".to_vec()))]);
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.update_datas(&condition, true, &datas);
    /// ```
    #[inline(always)]
    pub fn update_datas(&mut self, condition: &DbMap, is_filter_sync: bool, datas: &DbMap) -> Result<i32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.update_row(condition, is_filter_sync, datas);
        self.restore_if_exec_fail(closure)
    }

    /// Check whether data exists in the database.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: select count(*) as count from table_name where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let datas = DbMap::new();
    /// datas.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// datas.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// datas.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let exist = Database::build(user_id)?.is_data_exists(&datas, false);
    /// ```
    #[inline(always)]
    pub fn is_data_exists(&mut self, condition: &DbMap, is_filter_sync: bool) -> Result<bool> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.is_data_exists(condition, is_filter_sync);
        self.restore_if_exec_fail(closure)
    }

    /// Check whether data exists in the database without lock. 
    #[inline(always)]
    pub fn is_data_exists_without_lock(&mut self, condition: &DbMap, is_filter_sync: bool) -> Result<bool> {
        let closure = |e: &Table| e.is_data_exists(condition, is_filter_sync);
        self.restore_if_exec_fail(closure)
    }

    /// Query data that meets specified conditions(can be empty) from the database.
    /// If the operation is successful, the resultSet is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: select * from table_name where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let cond = DbMap::new();
    /// cond.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// cond.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.query_datas(&vec![], &cond, None, false);
    /// ```
    #[inline(always)]
    pub fn query_datas(
        &mut self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
        is_filter_sync: bool,
    ) -> Result<Vec<DbMap>> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.query_row(columns, condition, query_options, is_filter_sync, COLUMN_INFO);
        self.restore_if_exec_fail(closure)
    }

    /// Query datas from database with connect table.
    #[inline(always)]
    pub fn query_datas_with_connect_table_without_lock(&mut self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
        is_filter_sync: bool
    ) -> Result<Vec<DbMap>> {
        let closure = |e: &Table| e.query_connect_table_row(columns, condition, query_options, is_filter_sync, COMBINE_COLUMN_INFO);
        self.restore_if_exec_fail(closure)
    }

    /// Query datas from database with connect table.
    #[inline(always)]
    pub fn query_datas_with_connect_table(&mut self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
        is_filter_sync: bool
    ) -> Result<Vec<DbMap>> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        self.query_datas_with_connect_table_without_lock(columns, condition, query_options, is_filter_sync)
    }

    /// Query data that meets specified conditions(can be empty) from the database.
    /// If the operation is successful, the resultSet is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use asset_definition::Value;
    /// use asset_db_operator::{database::Database, types::{column, DbMap}};
    ///
    /// // SQL: select * from table_name where Owner='owner' and OwnerType=1 and Alias='alias'
    /// let cond = DbMap::new();
    /// cond.insert(column::OWNER, Value::Bytes(b"owner".to_ver()));
    /// cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Native as u32));
    /// cond.insert(column::ALIAS, Value::Bytes(b"alias".to_ver()));
    /// let user_id = 100;
    /// let ret = Database::build(user_id)?.query_data_without_lock(&vec![], &cond, None, false);
    /// ```
    pub fn query_data_without_lock(
        &mut self,
        columns: &Vec<&'static str>,
        condition: &DbMap,
        query_options: Option<&QueryOptions>,
        is_filter_sync: bool,
    ) -> Result<Vec<DbMap>> {
        let closure = |e: &Table| e.query_row(columns, condition, query_options, is_filter_sync, COLUMN_INFO);
        self.restore_if_exec_fail(closure)
    }

    /// query how many data fit the query condition
    pub fn query_data_count(&mut self, condition: &DbMap) -> Result<u32> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.count_datas(condition, false);
        self.restore_if_exec_fail(closure)
    }

    /// Delete old data and insert new data.
    pub fn replace_datas(&mut self, condition: &DbMap, is_filter_sync: bool, datas: &DbMap) -> Result<()> {
        let _lock = self.db_lock.mtx.lock().unwrap();
        let closure = |e: &Table| e.replace_row(condition, is_filter_sync, datas);
        self.restore_if_exec_fail(closure)
    }

    /// Get db lock.
    pub fn get_db_lock(&self) -> Result<Arc<Mutex<i32>>> {
        Ok(Arc::clone(&self.db_lock.mtx))
    }
}

impl Drop for Database {
    fn drop(&mut self) {
        self.close_db()
    }
}
