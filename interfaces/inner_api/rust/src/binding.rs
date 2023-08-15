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

use asset_rust_sdk::asset_insert;
use std::ffi::{c_char, CString};
use hilog_rust::{hilog, HiLogLabel, LogType};
use asset_common_lib::asset_log_info;

/// blablabla as documentation
#[no_mangle]
pub extern "C" fn AssetInsert(code: i32) -> i32
{
    asset_log_info!("AssetBinding", "receive code {} in AssetInsert", code);
    match asset_insert(code) {
        Ok(res) => {
            res as i32
        },
        Err(res) => {
            println!("err");
            res as i32
        }
    }
}
