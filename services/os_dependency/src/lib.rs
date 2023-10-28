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

//! This module is used to adapt to the functions on which assets depend.

use std::slice;

use asset_constants::OwnerType;
use asset_crypto_manager::crypto::{CryptoManager, SecretKey};
use asset_db_operator::{
    database_table_helper::{DatabaseHelper, COLUMN_OWNER, COLUMN_OWNER_TYPE},
    types::DbMap,
};
use asset_definition::{Accessibility, AuthType, Value};
use asset_file_operator::delete_user_db_dir;
use asset_hasher::sha256;
use asset_log::loge;

fn delete_key(user_id: i32, owner: &Vec<u8>, auth_type: AuthType, access_type: Accessibility) {
    let secret_key = SecretKey::new(user_id, owner, auth_type, access_type);
    if let Err(e) = secret_key.delete() {
        loge!("Delete huks key failed, error = {}", e);
    }
}

/// Function called from C programming language to Rust programming language for delete data by owner.
///
/// # Safety
///
/// The caller must ensure that the owner pointer is valid.
#[no_mangle]
pub unsafe extern "C" fn delete_data_by_owner(user_id: i32, owner: *const u8, owner_size: u32) -> i32 {
    let owner: Vec<u8> = unsafe { slice::from_raw_parts(owner, owner_size as usize).to_vec() };
    let owner_hash: Vec<u8> = sha256(&owner);
    let mut cond = DbMap::new();
    cond.insert(COLUMN_OWNER_TYPE, Value::Number(OwnerType::Hap as u32));
    cond.insert(COLUMN_OWNER, Value::Bytes(owner));
    match DatabaseHelper::delete_datas(user_id, &cond) {
        Ok(remove_num) => {
            delete_key(user_id, &owner_hash, AuthType::None, Accessibility::DeviceFirstUnlock);
            delete_key(user_id, &owner_hash, AuthType::None, Accessibility::DeviceUnlock);
            delete_key(user_id, &owner_hash, AuthType::Any, Accessibility::DeviceFirstUnlock);
            delete_key(user_id, &owner_hash, AuthType::Any, Accessibility::DeviceUnlock);
            remove_num
        },
        _ => 0,
    }
}

/// Function called from C programming language to Rust programming language for delete dir by user.
#[no_mangle]
pub extern "C" fn delete_dir_by_user(user_id: i32) -> bool {
    delete_user_db_dir(user_id).is_ok()
}

/// Function called from C programming language to Rust programming language for delete crypto.
#[no_mangle]
pub extern "C" fn delete_crypto_needing_device_unlock() {
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_device_unlock();
}
