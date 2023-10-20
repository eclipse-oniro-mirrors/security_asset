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

use std::ffi::{c_char, CString};

use asset_common::{
    definition::{Accessibility, AuthType, Value},
    hasher, loge,
};
use asset_crypto_manager::crypto::SecretKey;
use asset_db_operator::{
    database_table_helper::{DefaultDatabaseHelper, COLUMN_OWNER},
    types::DbMap,
};
use asset_file_operator::delete_user_db_dir;

fn delete_key(user_id: i32, owner: &Vec<u8>, auth_type: AuthType, access_type: Accessibility) {
    let secret_key = SecretKey::new(user_id, owner, auth_type, access_type);
    if let Err(e) = secret_key.delete() {
        loge!("Delete huks key failed, error = {}", e);
    }
}

/// Function called from C programming language to Rust programming language for delete hap Asset.
#[no_mangle]
pub extern "C" fn delete_data_by_owner(user_id: i32, owner: *const c_char) -> i32 {
    let owner = unsafe { CString::from_raw(owner as *mut c_char).into_string().unwrap() }; // todo: unwrap改掉
    let mut cond = DbMap::new();
    // cond.insert(COLUMN_OWNER_TYPE, Value::Number(OwnerType::Hap as u32)); // todo: 加个constants 文件 yzt
    cond.insert(COLUMN_OWNER, Value::Bytes(owner.as_bytes().to_vec())); // todo: owner + ownerLen 一起通过函数参数传过来
    match DefaultDatabaseHelper::delete_datas_default_once(user_id, &cond) {
        Ok(remove_num) => {
            let owner = hasher::sha256(&owner.as_bytes().to_vec());
            delete_key(user_id, &owner, AuthType::None, Accessibility::DeviceFirstUnlock);
            delete_key(user_id, &owner, AuthType::None, Accessibility::DeviceUnlock);
            delete_key(user_id, &owner, AuthType::Any, Accessibility::DeviceFirstUnlock);
            delete_key(user_id, &owner, AuthType::Any, Accessibility::DeviceUnlock);
            remove_num
        },
        _ => 0,
    }
}

/// Function called from C programming language to Rust programming language for delete user Asset.
#[no_mangle]
pub extern "C" fn delete_dir_by_user(user_id: i32) -> bool {
    delete_user_db_dir(user_id).is_ok()
}
