/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

//! This module is used to provide common functions for operation add and batch add.

use std::ffi::{c_char, CString};

use asset_common::{CallingInfo, OwnerType};
use asset_definition::{
    log_throw_error, Accessibility, AssetMap, ErrCode, SyncType, Tag, Value, WrapType, Extension, Result
};

use crate::{
    common::{add_owner_info, add_group},
    types::{column, DbMap}
};

extern "C" {
    fn CheckPermission(permission: *const c_char) -> bool;
}

/// Constant for max system user ID.
pub const SYSTEM_USER_ID_MAX: i32 = 99;
/// Constant for required attributes.
pub const REQUIRED_ATTRS: [Tag; 2] = [Tag::Secret, Tag::Alias];
/// Constant for optional attributes.
pub const OPTIONAL_ATTRS: [Tag; 3] = [Tag::Secret, Tag::ConflictResolution, Tag::WrapType];

/// Check the validity of accessibility.
pub fn check_accessibity_validity(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if calling_info.user_id() > SYSTEM_USER_ID_MAX {
        return Ok(());
    }
    let accessibility =
        attributes.get_enum_attr::<Accessibility>(&Tag::Accessibility).unwrap_or(Accessibility::DeviceFirstUnlocked);
    if accessibility == Accessibility::DevicePowerOn {
        return Ok(());
    }
    log_throw_error!(
        ErrCode::InvalidArgument,
        "[FATAL][SA]System user data cannot be protected by the lock screen password."
    )
}

/// Check the permission of the persistent.
pub fn check_persistent_permission(attributes: &AssetMap) -> Result<()> {
    if attributes.get(&Tag::IsPersistent).is_some() {
        let permission = CString::new("ohos.permission.STORE_PERSISTENT_DATA").unwrap();
        if unsafe { !CheckPermission(permission.as_ptr()) } {
            return log_throw_error!(ErrCode::PermissionDenied, "[FATAL][SA]Permission check failed.");
        }
    }
    Ok(())
}

/// Check the permission for sync.
pub fn check_sync_permission(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if attributes.get(&Tag::SyncType).is_none()
        || (attributes.get_num_attr(&Tag::SyncType)? & SyncType::TrustedAccount as u32) == 0
    {
        return Ok(());
    }
    match calling_info.owner_type_enum() {
        OwnerType::Hap => {
            if calling_info.app_index() > 0 {
                return log_throw_error!(ErrCode::Unsupported, "[FATAL]The caller does not support storing sync data.");
            }
        },
        OwnerType::HapGroup => {
            return log_throw_error!(ErrCode::Unsupported, "[FATAL]The caller does not support storing sync data.");
        },
        OwnerType::Native => (),
    }
    Ok(())
}

/// Check the permission for wrap.
pub fn check_wrap_permission(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if attributes.get(&Tag::WrapType).is_none()
        || attributes.get_enum_attr::<WrapType>(&Tag::WrapType)? == WrapType::Never
    {
        return Ok(());
    }
    match calling_info.owner_type_enum() {
        OwnerType::Hap | OwnerType::HapGroup => {
            if calling_info.app_index() > 0 {
                return log_throw_error!(ErrCode::Unsupported, "[FATAL]The caller does not support storing wrap data.");
            }
        },
        OwnerType::Native => (),
    }

    if attributes.get(&Tag::SyncType).is_none()
        || (attributes.get_num_attr(&Tag::SyncType)? & SyncType::TrustedAccount as u32) == 0
    {
        Ok(())
    } else {
        log_throw_error!(ErrCode::Unsupported, "[FATAL]trusted account data can not be set need wrap data.")
    }
}

/// Get query condition.
pub fn get_query_condition(attrs: &AssetMap, calling_info: &CallingInfo) -> Result<DbMap> {
    let alias = attrs.get_bytes_attr(&Tag::Alias)?;
    let mut query = DbMap::new();
    if calling_info.group().is_some() {
        add_group(calling_info, &mut query);
    } else {
        add_owner_info(calling_info, &mut query);
    }
    query.insert(column::ALIAS, Value::Bytes(alias.clone()));
    Ok(query)
}