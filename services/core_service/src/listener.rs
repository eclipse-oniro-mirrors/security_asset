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

//! This module is used to subscribe common event and system ability.

use std::{fs::{self, DirEntry}, slice, time::Instant, path::Path};

use asset_constants::{CallingInfo, OwnerType};
use asset_crypto_manager::{crypto_manager::CryptoManager, secret_key::SecretKey};
use asset_db_operator::{
    database::Database,
    types::{column, DbMap},
};
use asset_definition::{Result, Value};
use asset_file_operator::delete_user_db_dir;
use asset_log::{loge, logi};

use crate::sys_event::upload_fault_system_event;

const ASSET_DB: &str = "asset.db";
const BACKUP_SUFFIX: &str = ".backup";
const ROOT_PATH: &str = "data/service/el1/public/asset_service";

fn delete_on_package_removed(user_id: i32, owner: Vec<u8>) -> Result<bool> {
    let mut cond = DbMap::new();
    cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Hap as u32));
    cond.insert(column::OWNER, Value::Bytes(owner));
    cond.insert(column::IS_PERSISTENT, Value::Bool(false));
    let mut db = Database::build(user_id)?;
    let _ = db.delete_datas(&cond)?;

    cond.insert(column::IS_PERSISTENT, Value::Bool(true));
    db.is_data_exists(&cond)
}

fn clear_cryptos(calling_info: &CallingInfo) {
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_by_calling_info(calling_info);
}

extern "C" fn delete_data_by_owner(user_id: i32, owner: *const u8, owner_size: u32) {
    let start_time = Instant::now();
    let owner: Vec<u8> = unsafe { slice::from_raw_parts(owner, owner_size as usize).to_vec() };
    let calling_info = CallingInfo::new(user_id, OwnerType::Hap, owner.clone());
    clear_cryptos(&calling_info);
    let res = match delete_on_package_removed(user_id, owner) {
        Ok(true) => {
            logi!("The owner wants to retain data after uninstallation. Do not delete key in HUKS!");
            Ok(())
        },
        Ok(false) => SecretKey::delete_by_owner(&calling_info),
        Err(e) => {
            // Report the database operation fault event.
            upload_fault_system_event(&calling_info, start_time, "on_package_removed", &e);
            SecretKey::delete_by_owner(&calling_info)
        },
    };

    if let Err(e) = res {
        // Report the key operation fault event.
        let calling_info = CallingInfo::new_self();
        upload_fault_system_event(&calling_info, start_time, "on_package_removed", &e);
    }
}

extern "C" fn delete_dir_by_user(user_id: i32) {
    let _ = delete_user_db_dir(user_id);
}

extern "C" fn delete_crypto_need_unlock() {
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_need_device_unlocked();
}

fn process_backup(entry: &DirEntry, user_id: i32) -> Result<()> {
    if let Ok(file_name) = entry.file_name().into_string() {
        if file_name == ASSET_DB {
            Database::check_db(entry.path().with_file_name(ASSET_DB).to_string_lossy().to_string(), user_id)?;
            let backup_path = entry.path().with_file_name(format!("{}{}", ASSET_DB, BACKUP_SUFFIX));
            fs::copy(entry.path(), backup_path)?;
        }
    }
    Ok(())
}

extern "C" fn backup_db() {
    let start_time = Instant::now();
    match visit_root_dir(&process_backup) {
        Ok(_) => (),
        Err(e) => {
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, start_time, "backup_db", &e);
        },
    }
}

fn visit_root_dir(cb: &dyn Fn(&DirEntry, i32) -> Result<()>) -> Result<()> {
    let root_path = Path::new(ROOT_PATH);
    visit_dirs(root_path, cb)
}

fn visit_dirs(dir: &Path, cb: &dyn Fn(&DirEntry, i32) -> Result<()>) -> Result<()> {
    let start_time = Instant::now();
    if !dir.is_dir() {
        return Ok(());
    }
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if let Ok(user_id) = entry.file_name().to_string_lossy().to_string().parse::<i32>() {
            for entry in fs::read_dir(path)? {
                let entry = entry?;
                let path = entry.path();
                if path.is_dir() {
                    continue;
                }
                if let Err(e) = cb(&entry, user_id) {
                    let calling_info = CallingInfo::new_self();
                    upload_fault_system_event(&calling_info, start_time, &format!("backup_db_{}", user_id), &e);
                }
            }
        }
    }
    Ok(())
}

extern "C" {
    fn SubscribeSystemAbility(
        onPackageRemoved: extern "C" fn(i32, *const u8, u32),
        onUserRemoved: extern "C" fn(i32),
        onScreenOff: extern "C" fn(),
        onCharging: extern "C" fn(),
    ) -> bool;
    fn UnSubscribeSystemAbility() -> bool;
    fn SubscribeSystemEvent(
        onPackageRemoved: extern "C" fn(i32, *const u8, u32),
        onUserRemoved: extern "C" fn(i32),
        onScreenOff: extern "C" fn(),
        onCharging: extern "C" fn(),
    ) -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

/// Subscribe to the add and remove events of system abilities.
pub(crate) fn subscribe() {
    unsafe {
        if SubscribeSystemEvent(delete_data_by_owner, delete_dir_by_user, delete_crypto_need_unlock, backup_db) {
            logi!("Subscribe system event success.");
        } else {
            loge!("Subscribe system event failed.")
        }

        if SubscribeSystemAbility(delete_data_by_owner, delete_dir_by_user, delete_crypto_need_unlock, backup_db) {
            logi!("Subscribe system ability success.");
        } else {
            loge!("Subscribe system ability failed.")
        }
    }
}

/// Unsubscribe to the add and remove events of system abilities.
pub(crate) fn unsubscribe() {
    unsafe {
        if !UnSubscribeSystemAbility() {
            loge!("Unsubscribe system ability failed.")
        }

        if !UnSubscribeSystemEvent() {
            loge!("Unsubscribe system event failed.")
        }
    }
}
