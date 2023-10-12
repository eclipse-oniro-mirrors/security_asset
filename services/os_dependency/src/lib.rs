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

//! This module implements the function of Asset SDK from C to RUST.
#![allow(dead_code)]

use std::{
    ffi::{c_char, CString},
    fs, path::Path,
};

use asset_common::{
    loge, logi,
};

use db_operator::{
    database_table_helper::{
        DefaultDatabaseHelper,
        G_COLUMN_ACCESS_TYPE, G_COLUMN_AUTH_TYPE,
    },
    types::{
        DataValue, Pair,
    },
};

/// Function called from C programming language to Rust programming language for delete hap Asset.
/// # Safety
#[no_mangle]
pub unsafe extern "C" fn delete_hap_asset(user_id: i32, owner: *const c_char, auth_type: u32, access_type: u32) -> i32 {
    let owner_str = CString::from_raw(owner as *mut c_char).into_string().unwrap();
    let cond = vec![
        Pair { column_name: G_COLUMN_ACCESS_TYPE, value: DataValue::Integer(access_type) },
        Pair { column_name: G_COLUMN_AUTH_TYPE, value: DataValue::Integer(auth_type) }
    ];

    match DefaultDatabaseHelper::delete_datas_default_once(user_id, &owner_str, "", &cond) {
        Ok(remove_num) => {
            logi!("remove {} data", remove_num);
            remove_num
        },
        Err(_) => 0
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