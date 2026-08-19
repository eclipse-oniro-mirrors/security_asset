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

//! This module implements remote control functionality for GenerateControlledDevicePackage.

mod cli_manager;
mod grant_record;
mod remote_challenge_manager;
mod account_based_auth_manager;

#[cfg(not(feature = "SAFTest"))]
mod controlled_device;

#[cfg(not(feature = "SAFTest"))]
mod controller_device;

use saf_common::{JsonBuilder, new_object, object_add_number, object_add_string};
use saf_definition::{macros_lib, ErrCode, Result, RemoteAuthPackage, RemoteMessage, DeviceIdHeader, OperationType, PermissionQuery};
use saf_log::logi;
use saf_utils::{JsonValue, get_compact_json_value};

/// Batch generate result for device packages.
pub struct BatchGenerateResult {
    /// Generated packages (same count as input queries, failures use empty objects).
    pub packages: Vec<RemoteAuthPackage>,
    /// Error code (0 for full success, non-zero for partial or full failure).
    pub error_code: i32,
}

/// Batch verify result for device packages.
pub struct BatchVerifyResult {
    /// Verification results (same count as input packages).
    pub results: Vec<bool>,
    /// System error code (0 for no system error, non-zero for system error encountered).
    pub error_code: i32,
}

const MAX_REMOTE_BATCH_COUNT: usize = 10;
const MAX_REMOTE_PERMISSION_COUNT: usize = 40;

fn log_remote_auth_package(package: &RemoteAuthPackage) {
    let remote_message = &package.remote_message;
    let device_info = &remote_message.device_info;
    logi!(
        "RemoteAuthPackage: ticket_len={}, \
         controller_device_id_len={}, controlled_device_id_len={}, \
         remote_auth_message_len={}",
        package.ticket.len(),
        device_info.controller_device_id.len(),
        device_info.controlled_device_id.len(),
        remote_message.remote_auth_message.len()
    );
}

fn create_empty_package() -> RemoteAuthPackage {
    RemoteAuthPackage {
        remote_message: RemoteMessage {
            device_info: DeviceIdHeader {
                controller_device_id: String::new(),
                controlled_device_id: String::new(),
            },
            remote_auth_message: String::new(),
            caller_bundle_name: String::new(),
        },
        challenge: String::new(),
        ticket: String::new(),
    }
}

fn parse_ticket_expire_time(remote_auth_message: &str) -> Result<i32> {
    let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse remote_auth_message failed: {}", e)
    })?;

    let expire_time_str = get_compact_json_value(&json, "ticketExpireTimeMs")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();

    expire_time_str.parse::<i32>().map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse ticketExpireTimeMs failed: {}", e)
    })
}

fn parse_challenge_from_remote_auth_message(remote_auth_message: &str) -> Result<String> {
    let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse remote_auth_message failed: {}", e)
    })?;

    let challenge = get_compact_json_value(&json, "challenge")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();

    if challenge.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "challenge is empty");
    }

    Ok(challenge)
}

fn validate_challenge_consistency(package: &RemoteAuthPackage) -> Result<String> {
    let inner_challenge = parse_challenge_from_remote_auth_message(&package.remote_message.remote_auth_message)?;

    if inner_challenge != package.challenge {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "challenge mismatch!");
    }
    Ok(inner_challenge)
}

fn parse_caller_bundle_name_from_remote_auth_message(remote_auth_message: &str) -> Result<String> {
    let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse remote_auth_message failed: {}", e)
    })?;

    let bundle_name = get_compact_json_value(&json, "callerBundleName")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();

    Ok(bundle_name)
}

fn parse_local_device_id_from_remote_auth_message(remote_auth_message: &str) -> Result<String> {
        let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse remote_auth_message failed: {}", e)
    })?;

    let local_device_id = get_compact_json_value(&json, "localDeviceId")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();

    Ok(local_device_id)
}

const ALLOWED_REMOTE_AUTH_MESSAGE_FIELDS: &[&str] = &[
    "operationInfo",
    "role",
    "ticketExpireTimeMs",
    "domainId",
    "controlledDeviceName",
    "controllerDeviceName",
    "signVerifyMsg",
    "callerTokenId",
    "challenge",
    "timestamp",
    "permissions",
    "authResults",
    "localDeviceId",
    "callerBundleName",
    "version"
];

fn validate_remote_auth_message_fields(remote_auth_message: &str) -> Result<()> {
    let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse remote_auth_message failed: {}", e)
    })?;

    let obj = match json {
        JsonValue::Object(ref obj) => obj,
        _ => return macros_lib::log_throw_error!(ErrCode::DataTypeMismatch,
            "remote_auth_message is not a JSON object"),
    };

    let allowed_set: std::collections::HashSet<&str> = ALLOWED_REMOTE_AUTH_MESSAGE_FIELDS.iter().copied().collect();
    
    for (key, _) in obj.iter() {
        if !allowed_set.contains(key.as_str()) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
                "Unexpected field '{}' in remote_auth_message - potential MITM attack detected", key);
        }
    }

    Ok(())
}

fn parse_timestamp(remote_auth_message: &str) -> Result<u64> {
    let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse remote_auth_message failed: {}", e)
    })?;

    let ts_str = get_compact_json_value(&json, "timestamp")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();

    ts_str.parse::<u64>().map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
            "parse timestamp failed: {}", e)
    })
}

/// Maximum ticket expire time for remote control scenario: 1 day
pub const MAX_REMOTE_TICKET_EXPIRE_TIME_MS: i32 = 24 * 60 * 60 * 1000;
fn validate_ticket_expiration(timestamp: u64, expire_time_ms: i32, current_time: u64) -> bool {
    if expire_time_ms <= 0 || expire_time_ms > MAX_REMOTE_TICKET_EXPIRE_TIME_MS {
        return false;
    }
    match timestamp.checked_add(expire_time_ms as u64) {
        Some(expire_time) => expire_time >= current_time,
        None => false,
    }
}

const REMOTE_CHALLENGE_SIZE: u32 = 16;

fn generate_crypto_random_challenge() -> Result<String> {
    let mut buf = vec![0u8; REMOTE_CHALLENGE_SIZE as usize];
    crate::ticket_operation::generate_random_bytes(&mut buf, REMOTE_CHALLENGE_SIZE)?;
    let encoded = crate::ticket_operation::base64_encode(&buf)?;
    Ok(String::from_utf8_lossy(&encoded).to_string())
}
fn parse_cli_and_permission(
    operation_info: &[saf_definition::OperationInfo]
) -> Result<(Vec<saf_definition::CommandInfo>, Vec<String>)> {
    let mut cli_infos = Vec::with_capacity(operation_info.len());
    let mut api_permissions = Vec::with_capacity(operation_info.len());
    
    for op in operation_info {
        match op.operation_type {
            saf_definition::OperationType::Cli => {
                if op.cli_cmd_info.cmd_name.is_empty() {
                    return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "CLI command name is empty");
                }
                cli_infos.push(op.cli_cmd_info.clone());
            },
            saf_definition::OperationType::Api => {
                if op.permission.is_empty() {
                    return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "API permission is empty");
                }
                api_permissions.push(op.permission.clone());
            },
        }
    }
    Ok((cli_infos, api_permissions))
}

fn serialize_permission_query_to_message(builder: &mut JsonBuilder, query: &PermissionQuery) {
    let mut op_array = Vec::with_capacity(query.operation_info.len());
    for op in &query.operation_info {
        let mut op_obj = new_object();
        object_add_number(&mut op_obj, "operationType", op.operation_type as i64);
        if op.operation_type == OperationType::Cli {
            let mut cli_obj = new_object();
            object_add_string(&mut cli_obj, "cliCmdName", &op.cli_cmd_info.cmd_name);
            object_add_string(&mut cli_obj, "subCliCmdName", &op.cli_cmd_info.sub_cmd);
            op_obj.insert("info".to_string(), JsonValue::Object(cli_obj));
        } else {
            op_obj.insert("info".to_string(), JsonValue::String(op.permission.clone()));
        }
        op_array.push(op_obj);
    }
    builder.add_object_array("operationInfo", op_array);
    
    builder.add_number("role", query.remote_info.role as i64);
    builder.add_number("ticketExpireTimeMs", query.ticket_expire_time_ms as i64);
    builder.add_string("domainId", &query.remote_info.domain_id);
    builder.add_string("controlledDeviceName", &query.remote_info.remote_control_params.controlled_device_name);
    builder.add_string("controllerDeviceName", &query.remote_info.remote_control_params.controller_device_name);
    builder.add_string("signVerifyMsg", &query.remote_info.remote_control_params.sign_verify_msg);
    builder.add_number("callerTokenId", query.caller_token_id as i64);
}

pub use controlled_device::{
    generate_controlled_device_package,
    verify_controlled_device_package,
};

pub use controller_device::{
    generate_controller_device_package,
    verify_controller_device_package,
};

pub use grant_record::{get_bundle_name_from_token, 
    store_grant_record, StoreGrantRecordParams};

pub use cli_manager::batch_query_cli_permission;

// ======================== SAFTest module declarations ========================

#[cfg(feature = "SAFTest")]
pub mod controlled_device;

#[cfg(feature = "SAFTest")]
pub mod controller_device;

#[cfg(feature = "SAFTest")]
pub use controlled_device::ut_controlled_device_stub;

#[cfg(feature = "SAFTest")]
pub use controller_device::ut_controller_device_stub;