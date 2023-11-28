/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

use std::slice;

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

extern "C" fn delete_data_by_owner(user_id: i32, owner: *const u8, owner_size: u32) {
    let owner: Vec<u8> = unsafe { slice::from_raw_parts(owner, owner_size as usize).to_vec() };
    let calling_info = CallingInfo::new(user_id, OwnerType::Hap, owner.clone());
    let res = match delete_on_package_removed(user_id, owner) {
        Ok(true) => {
            logi!("The owner wants to retain data after uninstallation. Do not delete key in HUKS!");
            Ok(())
        },
        Ok(false) => SecretKey::delete_by_owner(&calling_info),
        Err(e) => {
            // Report the database operation fault event.
            upload_fault_system_event(&calling_info, "on_package_removed", &e);
            SecretKey::delete_by_owner(&calling_info)
        },
    };

    if let Err(e) = res {
        // Report the key operation fault event.
        let calling_info = CallingInfo::new_self();
        upload_fault_system_event(&calling_info, "on_package_removed", &e);
    }
}

extern "C" fn delete_dir_by_user(user_id: i32) {
    let _ = delete_user_db_dir(user_id);
}

extern "C" fn delete_crypto_need_unlock() {
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_need_device_unlocked();
}

extern "C" {
    fn SubscribeSystemAbility(
        onPackageRemoved: extern "C" fn(i32, *const u8, u32),
        onUserRemoved: extern "C" fn(i32),
        onScreenOff: extern "C" fn(),
    ) -> bool;
    fn UnSubscribeSystemAbility() -> bool;
    fn SubscribeSystemEvent(
        onPackageRemoved: extern "C" fn(i32, *const u8, u32),
        onUserRemoved: extern "C" fn(i32),
        onScreenOff: extern "C" fn(),
    ) -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

/// Subscribe to the add and remove events of system abilities.
pub(crate) fn subscribe() {
    unsafe {
        if SubscribeSystemEvent(delete_data_by_owner, delete_dir_by_user, delete_crypto_need_unlock) {
            logi!("Subscribe system event success.");
        } else {
            loge!("Subscribe system event failed.")
        }

        if SubscribeSystemAbility(delete_data_by_owner, delete_dir_by_user, delete_crypto_need_unlock) {
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
