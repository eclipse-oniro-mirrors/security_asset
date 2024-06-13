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

//! This module is used to check permission.

use std::{ffi::CString, os::raw::c_char};

use ipc::Skeleton;

use asset_common::{get_user_id, ROOT_USER_UPPERBOUND};
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result, Tag};

extern "C" {
    fn CheckPermission(permission: *const c_char) -> bool;
    fn CheckSystemHapPermission() -> bool;
}

pub(crate) fn check_system_permission(attrs: &AssetMap) -> Result<()> {
    if attrs.get(&Tag::UserId).is_some() {
        if unsafe { !CheckSystemHapPermission() } {
            return log_throw_error!(ErrCode::NotSystemApplication, "[FATAL]The caller is not system application.");
        }

        let permission = CString::new("ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS").unwrap();
        if unsafe { !CheckPermission(permission.as_ptr()) } {
            return log_throw_error!(ErrCode::PermissionDenied, "[FATAL][SA]Permission check failed.");
        }

        let uid = Skeleton::calling_uid();
        let user_id: i32 = get_user_id(uid)?;
        if user_id < 0 || user_id > ROOT_USER_UPPERBOUND as i32 {
            return log_throw_error!(
                ErrCode::AccessDenied,
                "[FATAL]The caller user_id is: {}. Not in range[0, 99]",
                user_id
            );
        }
    }
    Ok(())
}
