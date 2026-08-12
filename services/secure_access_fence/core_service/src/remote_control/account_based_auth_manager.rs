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

//! Trusted ring adapter for remote auth package operations

use saf_definition::{macros_lib, ErrCode, Result};
#[cfg(not(feature = "SAFTest"))]
use saf_log::logi;
use saf_definition::{DeviceIdHeader, RemoteAuthPackage};

#[cfg(not(feature = "SAFTest"))]
use std::collections::HashMap;
#[cfg(not(feature = "SAFTest"))]
use saf_definition::Value;
#[cfg(not(feature = "SAFTest"))]
use saf_plugin::saf_plugin::SAFPlugin;
#[cfg(not(feature = "SAFTest"))]
use saf_plugin_interface::plugin_interface::{
    EventType, ExtMap,
    SIGN_REMOTE_AUTH_PACKAGE_KEYS, VERIFY_REMOTE_AUTH_PACKAGE_KEYS,
};
#[cfg(not(feature = "SAFTest"))]
use saf_ipc::remote_message_wrapper;
#[cfg(not(feature = "SAFTest"))]
use saf_utils::{JsonValue, json_into_object, get_compact_json_value};

#[cfg(not(feature = "SAFTest"))]
fn log_sign_params(prefix: &str, params: &SignParams) {
    let log_prefix = format!("----testing {}", prefix);
    logi!(
        "[{}] SignParams: os_account_id={}, uid_len={}, remote_auth_package_len={}, remote_control_token_len={}",
        log_prefix,
        params.os_account_id,
        params.uid.len(),
        params.remote_auth_package.len(),
        params.remote_control_token.len()
    );
}

#[cfg(not(feature = "SAFTest"))]
fn log_sign_result(prefix: &str, result: &SignResult) {
    let log_prefix = format!("----testing {}", prefix);
    logi!(
        "[{}] SignResult: controller_device_id_len={}, controlled_device_id_len={}, \
         remote_auth_package_len={}, sign_info_len={}",
        log_prefix,
        result.device_id_header.controller_device_id.len(),
        result.device_id_header.controlled_device_id.len(),
        result.remote_auth_package.len(),
        result.sign_info.len()
    );
}

#[cfg(not(feature = "SAFTest"))]
fn log_verify_package(prefix: &str, package: &RemoteAuthPackage) {
    let log_prefix = format!("----testing {}", prefix);
    let remote_message = &package.remote_message;
    let device_info = &remote_message.device_info;
    logi!(
        "[{}] RemoteAuthPackage: ticket_len={}, \
         controller_device_id_len={}, controlled_device_id_len={}, \
         remote_auth_message_len={}",
        log_prefix,
        package.ticket.len(),
        device_info.controller_device_id.len(),
        device_info.controlled_device_id.len(),
        remote_message.remote_auth_message.len()
    );
}

/// Sign request parameters.
pub struct SignParams {
    /// OS account ID.
    pub os_account_id: i32,
    /// UID string.
    pub uid: String,
    /// Remote auth package.
    pub remote_auth_package: String,
    /// Remote control token.
    pub remote_control_token: String,
}

/// Sign result.
pub struct SignResult {
    /// Device ID header.
    pub device_id_header: DeviceIdHeader,
    /// Remote auth package.
    pub remote_auth_package: String,
    /// Sign info.
    pub sign_info: String,
}

#[cfg(not(feature = "SAFTest"))]
fn parse_sign_result(mut ret: ExtMap) -> Result<SignResult> {
    let device_id_header_str = match ret.remove(SIGN_REMOTE_AUTH_PACKAGE_KEYS.device_id_header) {
        Some(Value::String(ref s)) => s.clone(),
        _ => return macros_lib::log_throw_error!(ErrCode::HashMapKeyNotFound, "device_id_header not found"),
    };

    let mut device_id_header_obj = json_into_object(JsonValue::from_text(device_id_header_str)
        .map_err(|e| macros_lib::log_and_into_saf_error!(
            ErrCode::JsonParseError, "parse device_id_header failed: {}", e))?)?;

    let device_id_header = remote_message_wrapper::parse_device_id_header_from_json(&mut device_id_header_obj)?;

    let remote_auth_package = match ret.remove(SIGN_REMOTE_AUTH_PACKAGE_KEYS.result_remote_auth_package) {
        Some(Value::String(ref s)) => s.clone(),
        None => return macros_lib::log_throw_error!(ErrCode::HashMapKeyNotFound, 
            "result_remote_auth_package not found"),
        _ => return macros_lib::log_throw_error!(ErrCode::DataTypeMismatch, 
            "result_remote_auth_package type mismatch, expected String"),
    };
    
    let sign_info = match ret.remove(SIGN_REMOTE_AUTH_PACKAGE_KEYS.sign_info) {
        Some(Value::String(ref s)) => s.clone(),
        None => return macros_lib::log_throw_error!(ErrCode::HashMapKeyNotFound, 
            "sign_info not found"),
        _ => return macros_lib::log_throw_error!(ErrCode::DataTypeMismatch, 
            "sign_info type mismatch, expected String"),
    };

    Ok(SignResult {
        device_id_header,
        remote_auth_package,
        sign_info,
    })
}

/// Signs remote auth package through trusted ring plugin.
#[cfg(not(feature = "SAFTest"))]
pub fn sign_remote_auth_package(params: SignParams) -> Result<SignResult> {
    log_sign_params("sign_remote_auth_package", &params);

    let loader = SAFPlugin::get_instance().load_plugin().map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::PluginNotSupport, "load plugin failed: {}", e)
    })?;

    let mut ext_map: ExtMap = HashMap::new();
    let os_account_id_u32 = u32::try_from(params.os_account_id).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidOsAccountId,
            "os_account_id is negative: {}", params.os_account_id)
    })?;
    ext_map.insert(SIGN_REMOTE_AUTH_PACKAGE_KEYS.os_account_id, 
        Value::Number(os_account_id_u32));
    ext_map.insert(SIGN_REMOTE_AUTH_PACKAGE_KEYS.uid, 
        Value::String(params.uid));
    ext_map.insert(SIGN_REMOTE_AUTH_PACKAGE_KEYS.remote_auth_package, 
        Value::String(params.remote_auth_package));
    ext_map.insert(SIGN_REMOTE_AUTH_PACKAGE_KEYS.remote_control_token, 
        Value::String(params.remote_control_token));

    let ret = loader.process_event(EventType::SignRemoteAuthPackage, &mut ext_map)
        .map_err(|e| {
            macros_lib::log_and_into_saf_error!(
                ErrCode::try_from(e).unwrap_or(ErrCode::GeneralError),
                "sign_remote_auth_package process_event failed")
        })?;

    let result = parse_sign_result(ret)?;
    log_sign_result("sign_remote_auth_package_result", &result);
    Ok(result)
}

/// Verifies remote auth package through trusted ring plugin.
#[cfg(not(feature = "SAFTest"))]
pub fn verify_remote_auth_package(os_account_id: i32, package: &RemoteAuthPackage) -> Result<bool> {
    logi!("[----testing verify_remote_auth_package] os_account_id={}", os_account_id);
    log_verify_package("verify_remote_auth_package", package);

    let loader = SAFPlugin::get_instance().load_plugin().map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::PluginNotSupport, "load plugin failed: {}", e)
    })?;

    let device_id_header_str = remote_message_wrapper::serialize_device_id_header_to_json(
        &package.remote_message.device_info
    )?;
    
    let uid = parse_uid_from_remote_auth_message(&package.remote_message.remote_auth_message)?;
    
    logi!("[----testing verify_remote_auth_package] device_id_header_json_len={}", device_id_header_str.len());
    logi!("[----testing verify_remote_auth_package] device_id_header_json={}", device_id_header_str);
    logi!("[----testing verify_remote_auth_package] uid={}", uid);
    
    let mut ext_map: ExtMap = HashMap::new();
    let os_account_id_u32 = u32::try_from(os_account_id).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidOsAccountId,
            "os_account_id is negative: {}", os_account_id)
    })?;
    ext_map.insert(VERIFY_REMOTE_AUTH_PACKAGE_KEYS.os_account_id,
        Value::Number(os_account_id_u32));
    ext_map.insert(VERIFY_REMOTE_AUTH_PACKAGE_KEYS.uid,
        Value::String(uid));
    ext_map.insert(VERIFY_REMOTE_AUTH_PACKAGE_KEYS.device_id_header,
        Value::String(device_id_header_str));
    ext_map.insert(VERIFY_REMOTE_AUTH_PACKAGE_KEYS.remote_auth_message,
        Value::String(package.remote_message.remote_auth_message.clone()));
    ext_map.insert(VERIFY_REMOTE_AUTH_PACKAGE_KEYS.sign_info, 
        Value::String(package.ticket.clone()));

    let ret = loader.process_event(EventType::VerifyRemoteAuthPackage, &mut ext_map)
        .map_err(|e| {
            macros_lib::log_and_into_saf_error!(
                ErrCode::try_from(e).unwrap_or(ErrCode::GeneralError),
                "verify_remote_auth_package process_event failed")
        })?;

    match ret.get(VERIFY_REMOTE_AUTH_PACKAGE_KEYS.verify_result) {
        Some(Value::Bool(b)) => {
            logi!("[----testing verify_remote_auth_package] verify_result={}", b);
            Ok(*b)
        },
        _ => macros_lib::log_throw_error!(ErrCode::HashMapKeyNotFound, "verify_result not found"),
    }
}

#[cfg(not(feature = "SAFTest"))]
fn parse_uid_from_remote_auth_message(remote_auth_message: &str) -> Result<String> {
    let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError, 
            "parse remote_auth_message failed: {}", e)
    })?;
    
    let uid = get_compact_json_value(&json, "domainId")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();
    
    if uid.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "uid is empty in remote_auth_message");
    }
    
    Ok(uid)
}

// ======================== SAFTest mock implementations ========================

#[cfg(feature = "SAFTest")]
use std::sync::atomic::{AtomicBool, Ordering};
#[cfg(feature = "SAFTest")]
use lazy_static::lazy_static;

#[cfg(feature = "SAFTest")]
lazy_static! {
    static ref MOCK_SIGN_SUCCESS: AtomicBool = AtomicBool::new(true);
    static ref MOCK_VERIFY_SUCCESS: AtomicBool = AtomicBool::new(true);
}

#[cfg(feature = "SAFTest")]
pub fn sign_remote_auth_package(params: SignParams) -> Result<SignResult> {
    if MOCK_SIGN_SUCCESS.load(Ordering::SeqCst) {
        Ok(SignResult {
            device_id_header: DeviceIdHeader {
                controller_device_id: "mock_controller_udid".to_string(),
                controlled_device_id: "mock_controlled_udid".to_string(),
            },
            remote_auth_package: params.remote_auth_package,
            sign_info: "mock_sign_info".to_string(),
        })
    } else {
        macros_lib::log_throw_error!(ErrCode::GeneralError, "Mock sign failed")
    }
}

#[cfg(feature = "SAFTest")]
pub fn set_mock_sign_success(success: bool) {
    MOCK_SIGN_SUCCESS.store(success, Ordering::SeqCst);
}

#[cfg(feature = "SAFTest")]
pub fn verify_remote_auth_package(_os_account_id: i32, _package: &RemoteAuthPackage) -> Result<bool> {
    Ok(MOCK_VERIFY_SUCCESS.load(Ordering::SeqCst))
}

#[cfg(feature = "SAFTest")]
pub fn set_mock_verify_success(success: bool) {
    MOCK_VERIFY_SUCCESS.store(success, Ordering::SeqCst);
}

#[cfg(feature = "SAFTest")]
pub fn reset_mock_state() {
    MOCK_VERIFY_SUCCESS.store(true, Ordering::SeqCst);
    MOCK_SIGN_SUCCESS.store(true, Ordering::SeqCst);
    crate::remote_control::remote_challenge_manager::reset_mock_challenge_cache();
}
