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

use std::{fs, io::ErrorKind, path::Path, time::Instant};

use asset_definition::{log_throw_error, AssetError, AssetMap, ErrCode, Result};
use asset_db_operator::{
    database::{self, Database}, database_file_upgrade::{self, UpgradeData},
    types::{column, DbMap},
};
use asset_crypto_manager::db_key_operator;
use asset_log::{logw, logi};
use asset_file_operator::common::BACKUP_SUFFIX;
use asset_common::CallingInfo;

use crate::{get_ce_upgrade_info, sys_event::upload_system_event, UPGRADE_CE_MUTEX};

extern "C" {
    fn StoreUpgradeInSetting(user_id: i32, status: i32) -> bool;
}

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
        return log_throw_error!(ErrCode::DatabaseError, "Upgrade ce data failed.");
    }
    ce_db.exec("commit")?;
    remove_db(&path_str)
}

fn store_upgrade_info_in_settings(user_id: i32, status: CeUpgradeStatus) -> Result<()> {
    match unsafe{ StoreUpgradeInSetting(user_id, status as i32) } {
        true => Ok(()),
        false => log_throw_error!(ErrCode::DatabaseError, "store data in setting failed."),
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
