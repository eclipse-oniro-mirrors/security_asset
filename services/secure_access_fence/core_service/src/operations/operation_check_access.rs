/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.apache/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! This module is used to check access permission.

use saf_definition::{Conversion, Extension};
use saf_common::CallingInfo;
use saf_definition::{SAFMap, Tag, Value, ErrCode, Result, macros_lib};
use std::ffi::CString;
use std::os::raw::c_char;
use std::fmt::format;

impl_enum_trait! {
    #[derive(Clone, Copy)]
    #[derive(Eq, PartialEq)]
    #[derive(Debug)]
    pub enum AuthTrustLevel {
        ATL1 = 10000,
        ATL2 = 20000,
        ATL3 = 30000,
        ATL4 = 40000,
    }
}

const DEVICE_ID_MIN_LENGTH: usize = 1;
const DEVICE_ID_MAX_LENGTH: usize = 2048;
const BUNDLE_NAME_MIN_LENGTH: usize = 7;
const BUNDLE_NAME_MAX_LENGTH: usize = 128;

extern "C" {
    fn IsDeviceValid(userId: i32, deviceId: *const c_char, authTrustLevel: i32, isValid: &mut bool) -> i32;
}

pub(crate) fn check_access(calling_info: &CallingInfo, attributes: &SAFMap) -> Result<bool> {
    validate_permissions(calling_info)?;
    validate_required_parameters(attributes)?;
    validate_optional_parameters(attributes)?;
    validate_parameters(attributes)?;

    let device_id = get_device_id(attributes)?;
    let auth_trust_level = get_auth_trust_level(attributes)?;
    let access_bundle_name = get_access_bundle_name(attributes)?;

    let is_device_valid = check_device_valid(calling_info, &device_id, auth_trust_level)?;
    if !is_device_valid {
        return Ok(false);
    }

    let is_in_blacklist = check_access_bundle_in_blacklist(&access_bundle_name)?;
    if is_in_blacklist {
        return Ok(false);
    }

    Ok(true)
}

fn validate_permissions(_calling_info: &CallingInfo) -> Result<()> {
    // todo 看是否校验权限
    Ok(())
}

fn validate_required_parameters(attributes: &SAFMap) -> Result<()> {
    let required_tags = vec![
        Tag::AuthTrustLevel,
        Tag::DeviceId,
        Tag::AccessBundleName,
    ];
    check_required_tags(attributes, &required_tags)
}

fn validate_optional_parameters(attributes: &SAFMap) -> Result<()> {
    let valid_tags = vec![
        Tag::AuthTrustLevel,
        Tag::DeviceId,
        Tag::AccessBundleName,
        Tag::CallerBundleName,
    ];
    check_tag_validity(attributes, &valid_tags)
}

fn validate_parameters(attributes: &SAFMap) -> Result<()> {
    check_value_validity(attributes)
}

fn check_required_tags(attrs: &SAFMap, required_tags: &[Tag]) -> Result<()> {
    for tag in required_tags {
        if !attrs.contains_key(tag) {
            return macros_lib::log_throw_error!(
                ErrCode::ParamVerificationFailed,
                "The required tag [{}] is missing.",
                tag
            );
        }
    }
    Ok(())
}

fn check_tag_validity(attrs: &SAFMap, valid_tags: &[Tag]) -> Result<()> {
    for tag in attrs.keys() {
        if !valid_tags.contains(tag) {
            return macros_lib::log_throw_error!(
                ErrCode::ParamVerificationFailed,
                "The tag [{}] is illegal.",
                tag
            );
        }
    }
    Ok(())
}

fn check_value_validity(attrs: &SAFMap) -> Result<()> {
    for (tag, value) in attrs {
        check_data_type(tag, value)?;
        check_data_value(tag, value)?;
    }
    Ok(())
}

fn check_data_type(tag: &Tag, value: &Value) -> Result<()> {
    if tag.data_type() != value.data_type() {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "The data type[{}] of the tag[{}] does not match that of the value.",
            value.data_type(),
            tag
        );
    }
    Ok(())
}

fn check_data_value(tag: &Tag, value: &Value) -> Result<()> {
    match tag {
        Tag::AuthTrustLevel => check_auth_trust_level(tag, value),
        Tag::DeviceId => check_array_size(tag, value, DEVICE_ID_MIN_LENGTH, DEVICE_ID_MAX_LENGTH),
        Tag::AccessBundleName | Tag::CallerBundleName => {
            check_array_size(tag, value, BUNDLE_NAME_MIN_LENGTH, BUNDLE_NAME_MAX_LENGTH)
        },
        _ => Ok(()),
    }
}

fn check_number_range(tag: &Tag, value: &Value, min: u32, max: u32) -> Result<()> {
    let Value::Number(n) = value else {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[{}] is not a number.",
            tag
        );
    };
    if *n < min || *n > max {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "The value[{}] of Tag[{}] is not in the valid number range.",
            *n,
            tag
        );
    }
    Ok(())
}

fn check_auth_trust_level(tag: &Tag, value: &Value) -> Result<()> {
    let Value::Number(n) = value else {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[{}] is not a number.",
            tag
        );
    };
    match AuthTrustLevel::try_from(*n) {
        Ok(_) => Ok(()),
        Err(_) => {
            macros_lib::log_throw_error!(
                ErrCode::ParamVerificationFailed,
                "The value[{}] of Tag[{}] is not a valid AuthTrustLevel.",
                *n,
                tag
            )
        },
    }
}

fn check_array_size(tag: &Tag, value: &Value, min: usize, max: usize) -> Result<()> {
    let Value::Bytes(v) = value else {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "[{}] is not a bytes.",
            tag
        );
    };
    if v.len() < min || v.len() > max {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "The array length[{}] of Tag[{}], exceeds the valid range.",
            v.len(),
            tag
        );
    }
    Ok(())
}

fn get_device_id(attributes: &SAFMap) -> Result<Vec<u8>> {
    attributes.get_bytes_attr(&Tag::DeviceId).map(|v| v.clone())
}

fn get_auth_trust_level(attributes: &SAFMap) -> Result<AuthTrustLevel> {
    let value = attributes.get(&Tag::AuthTrustLevel)
        .ok_or_else(|| SAFError {
            code: ErrCode::ParamVerificationFailed,
            msg: "AuthTrustLevel not found".to_string(),
        })?;
    
    let Value::Number(n) = value else {
        return macros_lib::log_throw_error!(
            ErrCode::ParamVerificationFailed,
            "AuthTrustLevel must be Number type"
        );
    };
    
    AuthTrustLevel::try_from(*n).map_err(|_| SAFError {
        code: ErrCode::ParamVerificationFailed,
        msg: format!("Invalid AuthTrustLevel value: {}", *n),
    })
}

fn get_access_bundle_name(attributes: &SAFMap) -> Result<Vec<u8>> {
    attributes.get_bytes_attr(&Tag::AccessBundleName).map(|v| v.clone())
}

fn check_device_valid(calling_info: &CallingInfo, device_id: &[u8], auth_trust_level: AuthTrustLevel) -> Result<bool> {
    let device_id_str = String::from_utf8_lossy(device_id);
    let device_id_cstr = CString::new(device_id_str.as_ref()).unwrap();
    let mut is_valid: bool = false;
    let ret = unsafe {
        IsDeviceValid(
            calling_info.foreground_user_id(),
            device_id_cstr.as_ptr(),
            auth_trust_level as i32,
            &mut is_valid
        )
    };
    if ret != 0 {
        return macros_lib::log_throw_error!(
            ErrCode::UserIAMError,
            "Check device id valid failed"
        );
    }
    Ok(is_valid)
}

fn check_access_bundle_in_blacklist(_access_bundle_name: &[u8]) -> Result<bool> {
    // todo 调用saf提供接口检查是否在黑名单中
    Ok(false)
}

#[cfg(feature = "SAFTest")]
/// stub for test
pub mod ut_operation_check_access_stub {
    include!{"../../../../test/unittest/ut_test/services/core_service/test_stub/operations/ut_operation_add_stub.rs"}
}
