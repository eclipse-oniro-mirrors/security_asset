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

use asset_common::{loge, logi, hasher};
use asset_sdk::definition::{
    Accessibility, AuthType, Value
};
use crypto_manager::crypto::{
    KeyInfo, SecretKey,
};
use db_operator::{
    database_table_helper::{
        DefaultDatabaseHelper, G_COLUMN_OWNER,
    },
    types::Pair,
};

/// Function called from C programming language to Rust programming language for delete hap Asset.
/// # Safety
#[no_mangle]
pub unsafe extern "C" fn delete_hap_asset(user_id: i32, owner: *const c_char) -> i32 {
    // 1 delete data in db
    let owner_str = CString::from_raw(owner as *mut c_char).into_string().unwrap();
    let cond = vec![
        Pair { column_name: G_COLUMN_OWNER, value: Value::Bytes(owner_str.as_bytes().to_vec()) }
    ];
    let remove_num = match DefaultDatabaseHelper::delete_datas_default_once(user_id, &cond) {
        Ok(remove_num) => {
            logi!("remove {} data", remove_num);
            remove_num
        },
        Err(_) => 0
    };
    // 2 delete data in hucks
    let mut info = Vec::with_capacity(4);
    let owner_hasher = hasher::sha256(owner_str.as_bytes()).to_vec();
    logi!("delete_hap_asset! user_id:[{}], owner_hash:[{}]", user_id, String::from_utf8(owner_str.as_bytes().to_vec()).unwrap());  // todo delete
    info.push(KeyInfo { user_id, owner_hash: owner_hasher.clone(), auth_type: AuthType::Any as u32, access_type: Accessibility::DeviceUnlock as u32 });
    info.push(KeyInfo { user_id, owner_hash: owner_hasher.clone(), auth_type: AuthType::None as u32, access_type: Accessibility::DeviceFirstUnlock as u32 });
    info.push(KeyInfo { user_id, owner_hash: owner_hasher.clone(), auth_type: AuthType::None as u32, access_type: Accessibility::DeviceUnlock as u32 });
    info.push(KeyInfo { user_id, owner_hash: owner_hasher, auth_type: AuthType::Any as u32, access_type: Accessibility::DeviceFirstUnlock as u32 });
    while let Some(sub_info) = info.pop() {
        logi!("delet key use: auth_type:[{}], access_type:[{}]", sub_info.auth_type, sub_info.access_type);  // todo delete
        let secret_key = SecretKey::new(sub_info);
        match secret_key.delete() {
            Ok(true) => logi!("delete huks key pass"),
            Ok(false) => logi!("delete huks key never reached"),
            Err(res) => logi!("delete huks key fail error = {}", res),
        };
    }
    remove_num
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
            Err(e) if e.kind() != std::io::ErrorKind::NotFound => {
                loge!("remove dir failed! not found dir");
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
