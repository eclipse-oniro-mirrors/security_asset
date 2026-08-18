/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! This module implements remote grant status management.

use saf_definition::{macros_lib, ErrCode, Result, RemoteGrantStatus};

/// Get remote grant status.
pub fn get_remote_grant_status() -> Result<i32> {
    macros_lib::log_throw_error!(
        ErrCode::PluginNotSupport,
        "Plugin not support for get remote grant status"
    )
}

/// Update remote grant status.
pub fn update_remote_grant_status(status: i32) -> Result<()> {
    if status != RemoteGrantStatus::Enable as i32 && status != RemoteGrantStatus::Disable as i32 {
        return macros_lib::log_throw_error!(
            ErrCode::InvalidArgument,
            "Invalid remote grant status: {}",
            status
        );
    }
    
    macros_lib::log_throw_error!(
        ErrCode::PluginNotSupport,
        "Plugin not support for update remote grant status"
    )
}