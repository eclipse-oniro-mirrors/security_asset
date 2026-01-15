/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

//! This module provides interfaces for upgrade clone apps.
//! Databases are isolated based on users and protected by locks.

use std::{fs, io::ErrorKind, path::Path, time::Instant, os::raw::c_char, ffi::CString};

use asset_definition::{macros_lib, AssetError, AssetMap, ErrCode, Result};
use asset_db_operator::{
    database::{self, Database}, database_file_upgrade::{self, UpgradeData},
    types::{column, DbMap},
};
use asset_crypto_manager::db_key_operator;
use asset_log::{logw, logi};
use asset_file_operator::common::BACKUP_SUFFIX;
use asset_common::CallingInfo;

use crate::{
    get_ce_upgrade_info, 
    sys_event::{upload_fault_system_event, upload_statistic_system_event, upload_system_event}, 
    UPGRADE_CE_MUTEX
};

extern "C" {
    fn StoreKeyValue(user_id: i32, column_key: *const c_char, column_value: i32) -> bool;
}

const ASSET_CE_UPGRADE: &str = "ASSET_CE_UPGRADE";

pub(crate) enum CeUpgradeStatus {
    Start = 0,
    Fail = -1,
    End = 1,
}

/// Upgrade data to ce apps.
pub fn upgrade_ce_data(user_id: i32) -> Result<()> {
    let mut upgrade_data = database_file_upgrade::get_file_content(user_id)?;
    upgrade_ce(user_id, &mut upgrade_data)
}

fn remove_db(path: &str) -> Result<()> {
    let backup_db_path = format!("{}{}", path, BACKUP_SUFFIX);
    let mut res = Ok(());
    for path in [path, &backup_db_path] {
        match fs::remove_file(path) {
            Ok(_) => (),
            Err(e) if e.kind() == ErrorKind::NotFound => (),
            Err(e) => {
                logw!("[WARNING]Remove db:[{}] failed, error code:[{}]", path, e);
                res = Err(AssetError { code: ErrCode::DatabaseError, msg: "rmove file failed".to_string() })
            },
        };
    }
    res
}

fn upgrade_ce_data_process(user_id: i32, ce_upgrade_db_name: &str) -> Result<()> {
    // check de exist
    let path_str = database::fmt_de_db_path_with_name(user_id, ce_upgrade_db_name);
    let path = Path::new(&path_str);
    if !path.exists() {
        return Ok(());
    }
    // get data from de
    let mut db_main = Database::build_with_file_name(user_id, ce_upgrade_db_name, &None)?;
    let mut datas: Vec<DbMap> = db_main.query_datas(&vec![], &DbMap::new(), None, false)?;
    if datas.is_empty() {
        remove_db(&path_str)?;
        return Ok(());
    }
    // store data in ce
    let ce_db_name = format!("enc_{}", ce_upgrade_db_name);
    let db_key = db_key_operator::get_db_key(user_id, true)?;
    let mut ce_db = Database::build_with_file_name(user_id, &ce_db_name, &db_key)?;
    ce_db.exec("begin transaction")?;
    let mut need_rollback = false;
    for data in datas.iter_mut() {
        data.remove(column::ID);
        if let Err(e) = ce_db.insert_datas(data) {
            if e.code == ErrCode::Duplicated {
                continue;
            }
            need_rollback = true;
            break;
        }
    }
    // remove de and de backup
    if need_rollback {
        ce_db.exec("rollback")?;
        return macros_lib::log_throw_error!(ErrCode::DatabaseError, "Upgrade ce data failed.");
    }
    ce_db.exec("commit")?;
    remove_db(&path_str)
}

fn store_upgrade_info_in_settings(user_id: i32, status: CeUpgradeStatus) -> Result<()> {
    let key = CString::new(ASSET_CE_UPGRADE).unwrap();
    match unsafe{ StoreKeyValue(user_id, key.as_ptr(), status as i32) } {
        true => Ok(()),
        false => macros_lib::log_throw_error!(ErrCode::DatabaseError, "store data in setting failed."),
    }
}

fn upgrade_ce_process(user_id: i32, upgrade_data: &mut UpgradeData, upgrade_info: &'static [u8]) -> Result<()> {
    store_upgrade_info_in_settings(user_id, CeUpgradeStatus::Start)?;
    let ce_upgrade_db_name = database_file_upgrade::construct_hap_owner_info(upgrade_info)?;
    upgrade_ce_data_process(user_id, &ce_upgrade_db_name)?;
    upgrade_data.ce_upgrade = Some(ce_upgrade_db_name);
    store_upgrade_info_in_settings(user_id, CeUpgradeStatus::End)?;
    database_file_upgrade::save_to_writer(user_id, upgrade_data)
}

fn upgrade_ce(user_id: i32, upgrade_data: &mut UpgradeData) -> Result<()> {
    let _rwlock = UPGRADE_CE_MUTEX.write().unwrap();
    if upgrade_data.ce_upgrade.is_some() {
        return Ok(());
    }
    let upgrade_info = get_ce_upgrade_info();
    if upgrade_info.is_empty() {
        return Ok(());
    }

    logi!("[INFO]start ce upgrade [{}].", user_id);
    let calling_info = CallingInfo::new_self();
    let start = Instant::now();
    let upgrade_res = upgrade_ce_process(user_id, upgrade_data, upgrade_info);
    if upgrade_res.is_err() {
        let _ = store_upgrade_info_in_settings(user_id, CeUpgradeStatus::Fail);
    }
    logi!("[INFO]end ce upgrade [{}].", user_id);
    let _ = upload_system_event(upgrade_res.clone(), &calling_info, start, "upgrade_ce", &AssetMap::new());
    upgrade_res
}

fn get_db_data_count(user_id: i32, db_name: &str, path_str: &str, db_key: Option<Vec<u8>>) -> Result<u32> {
    let path = Path::new(path_str);
    let db_data_count = if !path.exists() { 0 } else {
        let mut db_main = Database::build_with_file_name(user_id, db_name, &db_key)?;
        db_main.query_data_count(&DbMap::new())?
    };
    Ok(db_data_count)
}

fn process_upgrade_count(user_id: i32, upgrade_info: &'static [u8]) -> Result<String> {
    let de_db_name = database_file_upgrade::construct_hap_owner_info(upgrade_info)?;
    let path_str = database::fmt_de_db_path_with_name(user_id, &de_db_name);
    let de_count = get_db_data_count(user_id, &de_db_name, &path_str, None)?;
    let ce_db_name = format!("enc_{}", &de_db_name);
    let ce_path_str = database::fmt_ce_db_path_with_name(user_id, &ce_db_name);
    let db_key = db_key_operator::get_db_key(user_id, true)?;
    let ce_count = get_db_data_count(user_id, &ce_db_name, &ce_path_str, db_key)?;
    Ok(format!("db_name:{} de count: {}, ce count: {}", &de_db_name, de_count, ce_count))
}

pub fn summary_upgrade_data_count(user_id: i32) {
    let upgrade_info = get_ce_upgrade_info();
    if upgrade_info.is_empty() {
        return;
    }

    let calling_info = CallingInfo::new_self();
    let start = Instant::now();
    match process_upgrade_count(user_id, upgrade_info) {
        Ok(ext_info) => upload_statistic_system_event(&calling_info, start, "upgrade_ce_result", &ext_info),
        Err(e) => upload_fault_system_event(&calling_info, start, "upgrade_ce_result", &e)
    }
}
