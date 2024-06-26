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

use std::{
    ffi::CStr, fs::{self, DirEntry}, slice, time::Instant
};

use asset_common::{AutoCounter, CallingInfo, OwnerType};
use asset_crypto_manager::{crypto_manager::CryptoManager, secret_key::SecretKey};
use asset_db_operator::{
    database::Database,
    types::{column, DbMap},
};
use asset_definition::{Result, Value, SyncType};
use asset_file_operator::delete_user_db_dir;
use asset_log::{loge, logi};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_sdk::plugin_interface::{EventType, ExtDbMap, PARAM_NAME_APP_INDEX, PARAM_NAME_BUNDLE_NAME, PARAM_NAME_IS_HAP, PARAM_NAME_USER_ID};

use crate::sys_event::upload_fault_system_event;

const ASSET_DB: &str = "asset.db";
const BACKUP_SUFFIX: &str = ".backup";
const ROOT_PATH: &str = "data/service/el1/public/asset_service";

fn delete_on_package_removed(user_id: i32, owner: Vec<u8>) -> Result<bool> {
    let mut cond = DbMap::new();
    cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Hap as u32));
    cond.insert(column::OWNER, Value::Bytes(owner));
    cond.insert(column::IS_PERSISTENT, Value::Bool(false));
    let mut reverse_condition = DbMap::new();
    reverse_condition.insert(column::SYNC_TYPE, Value::Number(SyncType::TrustedAccount as u32));
    let mut db = Database::build(user_id)?;
    let _ = db.delete_datas(&cond, Some(&reverse_condition), false)?;

    cond.remove(column::IS_PERSISTENT);
    db.is_data_exists(&cond, false)
}

fn clear_cryptos(calling_info: &CallingInfo) {
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_by_calling_info(calling_info);
}

fn delete_data_by_owner(user_id: i32, owner: *const u8, owner_size: u32) {
    let _counter_user = AutoCounter::new();
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

pub(crate) extern "C" fn on_package_removed(user_id: i32, owner: *const u8, owner_size: u32, bundle_name: *const u8,
    app_index: i32) {
    delete_data_by_owner(user_id, owner, owner_size);

    let c_str = unsafe { CStr::from_ptr(bundle_name as _) };
    let bundle_name = match c_str.to_str() {
        Ok(s) => s.to_string(),
        Err(e) => {
            loge!("[FATAL]Parse sting from bundle name failed, error is {}.", e);
            return;
        }
    };

    logi!("[INFO]On app -{}-{}-{}- removed.", user_id, bundle_name, app_index);

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(bundle_name.as_bytes().to_vec()));

        // only hap package can be removed
        params.insert(PARAM_NAME_IS_HAP, true);
        params.insert(PARAM_NAME_APP_INDEX, Value::Number(app_index as u32));
        match load.process_event(EventType::OnPackageClear, &params) {
            Ok(()) => logi!("process package remove event success."),
            Err(code) => loge!("process package remove event failed, code: {}", code),
        }
    }
}

extern "C" fn delete_dir_by_user(user_id: i32) {
    let _counter_user = AutoCounter::new();
    let _ = delete_user_db_dir(user_id);
}

extern "C" fn delete_crypto_need_unlock() {
    let _counter_user = AutoCounter::new();
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_need_device_unlocked();
}

pub(crate) extern "C" fn backup_db() {
    let _counter_user = AutoCounter::new();
    let start_time = Instant::now();
    match backup_all_db(&start_time) {
        Ok(_) => (),
        Err(e) => {
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, start_time, "backup_db", &e);
        },
    }
}

pub(crate) extern "C" fn on_app_restore(user_id: i32, bundle_name: *const u8, app_index: i32) {
    let c_str = unsafe { CStr::from_ptr(bundle_name as _) };
    let bundle_name = match c_str.to_str() {
        Ok(s) => s.to_string(),
        Err(e) => {
            loge!("[FATAL]Parse sting from bundle name failed, error is {}.", e);
            return;
        }
    };
    logi!("[INFO]On app -{}-{}- restore.", user_id, bundle_name);

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(bundle_name.as_bytes().to_vec()));
        params.insert(PARAM_NAME_APP_INDEX, Value::Number(app_index as u32));
        match load.process_event(EventType::OnAppRestore, &params) {
            Ok(()) => logi!("process app restore event success."),
            Err(code) => loge!("process app restore event failed, code: {}", code),
        }
    }
}

pub(crate) extern "C" fn on_user_unlocked(user_id: i32) {
    logi!("[INFO]On user -{}- unlocked.", user_id);

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        match load.process_event(EventType::OnUserUnlocked, &params) {
            Ok(()) => logi!("process user unlocked event success."),
            Err(code) => loge!("process user unlocked event failed, code: {}", code),
        }
    }

}

fn backup_db_if_accessible(entry: &DirEntry, user_id: i32) -> Result<()> {
    let from_path = entry.path().with_file_name(format!("{}/{}", user_id, ASSET_DB)).to_string_lossy().to_string();
    Database::check_db_accessible(from_path.clone(), user_id)?;
    let backup_path = format!("{}{}", from_path, BACKUP_SUFFIX);
    fs::copy(from_path, backup_path)?;
    Ok(())
}

fn backup_all_db(start_time: &Instant) -> Result<()> {
    for entry in fs::read_dir(ROOT_PATH)? {
        let entry = entry?;
        if let Ok(user_id) = entry.file_name().to_string_lossy().to_string().parse::<i32>() {
            if let Err(e) = backup_db_if_accessible(&entry, user_id) {
                let calling_info = CallingInfo::new_self();
                upload_fault_system_event(&calling_info, *start_time, &format!("backup_db_{}", user_id), &e);
            }
        }
    }
    Ok(())
}

#[derive(Clone)]
#[repr(C)]
struct EventCallBack {
    on_package_remove: extern "C" fn(i32, *const u8, u32, *const u8, i32),
    on_user_removed: extern "C" fn(i32),
    on_screen_off: extern "C" fn(),
    on_charging: extern "C" fn(),
    on_app_restore: extern "C" fn(i32, *const u8, i32),
    on_user_unlocked: extern "C" fn(i32),
}

extern "C" {
    fn SubscribeSystemAbility(
        eventCallBack: EventCallBack
    ) -> bool;
    fn UnSubscribeSystemAbility() -> bool;
    fn SubscribeSystemEvent(
        eventCallBack: EventCallBack
    ) -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

/// Subscribe to the add and remove events of system abilities.
pub(crate) fn subscribe() {
    unsafe {
        let call_back = EventCallBack {
            on_package_remove: on_package_removed,
            on_user_removed: delete_dir_by_user,
            on_screen_off: delete_crypto_need_unlock,
            on_charging: backup_db,
            on_app_restore,
            on_user_unlocked
        };
        if SubscribeSystemEvent(call_back.clone()) {
            logi!("Subscribe system event success.");
        } else {
            loge!("Subscribe system event failed.")
        }

        if SubscribeSystemAbility(call_back) {
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
