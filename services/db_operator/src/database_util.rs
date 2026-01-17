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

//! This module provides interfaces for database util.
//! Databases are isolated based on users and protected by locks.

use std::ffi::CStr;

use asset_common::OWNER_INFO_SEPARATOR;
use asset_definition::{
    macros_lib, ErrCode, Result,
};

extern "C" {
    fn GetCeUpgradeInfo() -> *const u8;
}

const MINIM_OWNER_INFO_LEN: usize = 3;
const REMOVE_INDEX: usize = 2;

pub(crate) fn get_ce_upgrade_info() -> &'static [u8] {
    let info = unsafe { GetCeUpgradeInfo() };
    if !info.is_null() {
        let c_str = unsafe { CStr::from_ptr(info as _) };
        if let Ok(result) = c_str.to_str() {
            return result.as_bytes()
        }
    }
    &[]
}

/// use owner info to construct db name
pub fn construct_hap_owner_info(owner_info: &[u8]) -> Result<String> {
    let owner_info_string = String::from_utf8_lossy(owner_info).to_string();
    let split_owner_info: Vec<&str> = owner_info_string.split(OWNER_INFO_SEPARATOR).collect();
    if split_owner_info.len() < MINIM_OWNER_INFO_LEN || split_owner_info.last().is_none() {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Wrong queried owner info!");
    }
    let app_index = split_owner_info.last().unwrap();
    let mut split_owner_info_mut = split_owner_info.clone();
    for _ in 0..REMOVE_INDEX {
        split_owner_info_mut.pop();
    }
    let owner_info = split_owner_info_mut.join("_").clone();
    Ok(format!("Hap_{}_{}", owner_info, app_index))
}

pub(crate) fn is_db_need_ce_unlock(db_name: &str) -> bool {
    let upgrade_info = get_ce_upgrade_info();
    if upgrade_info.is_empty() {
        return false;
    }
    match construct_hap_owner_info(upgrade_info) {
        Ok(de_db_name) => db_name == format!("enc_{}", de_db_name),
        Err(_e) => false,
    }
}
