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

//! This create implement the asset

use std::{
    ffi::{c_char, CString},
    fs, path::Path,
};

use asset_common::{hasher, logi};
use asset_sdk::definition::{
    Accessibility, AuthType, Value
};
use crypto_manager::crypto::SecretKey;
use db_operator::{
    database_table_helper::{
        DefaultDatabaseHelper, COLUMN_OWNER,
    },
    types::DbMap,
};

fn delete_key(user_id: i32, owner: &Vec<u8>, auth_type: AuthType, access_type: Accessibility) {
    let secret_key = SecretKey::new(user_id, owner, auth_type, access_type);
    match secret_key.delete() {
        Ok(true) => logi!("delete huks key pass"),
        Ok(false) => logi!("delete huks key never reached"),
        Err(res) => logi!("delete huks key fail error = {}", res),
    };
}

/// Function called from C programming language to Rust programming language for delete hap Asset.
/// # Safety
#[no_mangle]
pub unsafe extern "C" fn delete_hap_asset(user_id: i32, owner: *const c_char) -> i32 {
    // 1 delete data in db
    let owner = CString::from_raw(owner as *mut c_char).into_string().unwrap();
    let cond = DbMap::from([
        (COLUMN_OWNER, Value::Bytes(owner.as_bytes().to_vec())),
    ]);
    match DefaultDatabaseHelper::delete_datas_default_once(user_id, &cond) {
        Ok(remove_num) if remove_num > 0 => {
            // 2 delete data in huks
            let owner = hasher::sha256(&owner.as_bytes().to_vec());
            delete_key(user_id, &owner, AuthType::None, Accessibility::DeviceFirstUnlock);
            delete_key(user_id, &owner, AuthType::None, Accessibility::DeviceUnlock);
            delete_key(user_id, &owner, AuthType::Any, Accessibility::DeviceFirstUnlock);
            delete_key(user_id, &owner, AuthType::Any, Accessibility::DeviceUnlock);
            remove_num
        },
        _ => 0
    }
}

const ROOT_PATH: &str = "data/service/el1/public/asset_service";

/// Function called from C programming language to Rust programming language for delete user Asset.
/// # Safety
/// dereference pointer
#[no_mangle]
pub extern "C" fn delete_user_asset(user_id: i32) {
    let path_str = format!("{}/{}", ROOT_PATH, user_id);
    let path = Path::new(&path_str);
    if !path.exists() {
        match fs::remove_dir_all(path) {
            Ok(_) => { logi!("remove dir success!"); },
            Err(e) if e.kind() != std::io::ErrorKind::NotFound => {
                logi!("remove dir failed! not found dir");
            },
            Err(e) if e.kind() == std::io::ErrorKind::PermissionDenied => {
                logi!("remove dir failed! permission denied");
            },
            Err(e) if e.kind() == std::io::ErrorKind::Interrupted => {
                logi!("remove dir failed! interrupted");
            },
            _ => (),
        }
    }
}
