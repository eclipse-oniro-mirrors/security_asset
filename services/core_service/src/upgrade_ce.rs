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

use std::{fs, path::Path};

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_db_operator::{
    database::{self, Database}, database_file_upgrade::{self, UpgradeData},
    types::{column, DbMap},
};
use asset_crypto_manager::db_key_operator;
use asset_log::{logw, logi};
use asset_file_operator::common::BACKUP_SUFFIX;

use crate::{get_ce_upgrade_info, UPGRADE_CE_MUTEX};

extern "C" {
    fn StoreUpgradeInSetting(user_id: i32) -> bool;
}

/// Upgrade data to ce apps.
pub fn upgrade_ce_data(user_id: i32) -> Result<()> {
    let mut upgrade_data = database_file_upgrade::get_file_content(user_id)?;
    upgrade_ce(user_id, &mut upgrade_data)
}

fn remove_db(path: &str) -> Result<()> {
    let backup_db_path = format!("{}{}", path, BACKUP_SUFFIX);
    match fs::remove_file(path) {
        Ok(_) => (),
        Err(e) => {
            logw!("[WARNING]Remove db:[{}] failed, error code:[{}]", path, e);
        },
    };
    match fs::remove_file(&backup_db_path) {
        Ok(_) => (),
        Err(e) => {
            logw!("[WARNING]Remove db:[{}] failed, error code:[{}]", &backup_db_path, e);
        },
    };
    Ok(())
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
        if ce_db.insert_datas(data).is_err() {
            need_rollback = true;
            break;
        }
    }
    // remove de and de backup
    if need_rollback {
        ce_db.exec("rollback")?;
        return log_throw_error!(ErrCode::DatabaseError, "Upgrade clone app data failed.");
    }
    ce_db.exec("commit")?;
    remove_db(&path_str)
}

fn store_upgrade_info_in_settings(user_id: i32) -> Result<()> {
    match unsafe{ StoreUpgradeInSetting(user_id) } {
        true => Ok(()),
        false => log_throw_error!(ErrCode::DatabaseError, "store data in setting failed."),
    }
}

fn upgrade_ce(user_id: i32, upgrade_data: &mut UpgradeData) -> Result<()> {
    let _rwlock = UPGRADE_CE_MUTEX.write().unwrap();
    if upgrade_data.ce_upgrade.is_some() {
        return Ok(());
    }
    logi!("[INFO]start ce upgrade [{}].", user_id);
    let upgrade_info = get_ce_upgrade_info();
    if upgrade_info.is_empty() {
        return Ok(());
    }

    let ce_upgrade_db_name = database_file_upgrade::construct_hap_owner_info(upgrade_info)?;
    upgrade_ce_data_process(user_id, &ce_upgrade_db_name)?;
    upgrade_data.ce_upgrade = Some(ce_upgrade_db_name);
    store_upgrade_info_in_settings(user_id)?;
    database_file_upgrade::save_to_writer(user_id, upgrade_data)
}
