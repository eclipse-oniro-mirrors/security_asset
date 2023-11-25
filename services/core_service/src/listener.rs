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
use asset_definition::{AssetError, Value};
use asset_file_operator::delete_user_db_dir;
use asset_log::{loge, logi};

use crate::sys_event::upload_fault_system_event;

fn upload_delete_data_error(e: &AssetError) {
    let calling_info = CallingInfo::new_self();
    let func_name = hisysevent::function!();
    upload_fault_system_event(&calling_info, func_name, e);
}

extern "C" fn delete_data_by_owner(user_id: i32, owner: *const u8, owner_size: u32) {
    let owner: Vec<u8> = unsafe { slice::from_raw_parts(owner, owner_size as usize).to_vec() };
    let mut cond = DbMap::new();
    cond.insert(column::OWNER_TYPE, Value::Number(OwnerType::Hap as u32));
    cond.insert(column::OWNER, Value::Bytes(owner.clone()));
    cond.insert(column::IS_PERSISTENT, Value::Bool(false));
    let mut db = match Database::build(user_id) {
        Ok(db) => db,
        Err(e) => {
            upload_delete_data_error(&e);
            return
        }
    };
    let tmp = db.delete_datas(&cond);
    if let Err(e) = tmp {
        upload_delete_data_error(&e);
    }
    let calling_info = CallingInfo::new(user_id, OwnerType::Hap, owner);

    cond.insert(column::IS_PERSISTENT, Value::Bool(true));
    match db.query_datas(&vec![], &cond, None) {
        Ok(data) if data.is_empty() => {
            let res = SecretKey::delete_by_owner(&calling_info);
            if let Err(e) = res {
                upload_delete_data_error(&e);
            }
        },
        Ok(_) => {
            logi!("The delete owner have msg left, won't delete huks key!")
        },
        Err(e) => {
            loge!("Query delete owner left data fail, ErrorCode:[{}]", e.code)
        },
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
