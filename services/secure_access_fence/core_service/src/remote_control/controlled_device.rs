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

//! This module implements controlled device package generation and verification.

use ipc::Skeleton;
use saf_common::{get_user_id, JsonBuilder};
use saf_utils::system_time_in_millis;
use saf_definition::{macros_lib, ErrCode, Result,
    PermissionQuery, RemoteAuthPackage,
    Role, RemoteMessage,
};
use saf_log::{loge, logi};
use std::ffi::CString;
use std::os::raw::c_char as raw_c_char;

extern "C" {
    fn CheckPermission(permission: *const raw_c_char) -> bool;
}

const QUERY_TOOL_PERMISSIONS: &str = "ohos.permission.QUERY_TOOL_PERMISSIONS";

use crate::remote_control::{create_empty_package, log_remote_auth_package, parse_ticket_expire_time, parse_timestamp,
    validate_ticket_expiration, generate_crypto_random_challenge, serialize_permission_query_to_message,
    validate_remote_auth_message_fields, BatchGenerateResult, BatchVerifyResult};
use crate::remote_control::remote_challenge_manager::cache_challenge;
use crate::remote_control::account_based_auth_manager::{sign_remote_auth_package, verify_remote_auth_package, SignParams};

/// Generates controlled device packages for permission queries.
pub fn generate_controlled_device_package(
    queries: Vec<PermissionQuery>
) -> BatchGenerateResult {
    let permission = CString::new(QUERY_TOOL_PERMISSIONS).unwrap();
    if unsafe { !CheckPermission(permission.as_ptr()) } {
        loge!("Permission denied! Need {}", QUERY_TOOL_PERMISSIONS);
        return BatchGenerateResult {
            packages: vec![create_empty_package(); queries.len()
            ],
            error_code: ErrCode::PermissionDenied as i32,
        };
    }
    
    if let Err(e) = validate_controlled_batch_params(&queries) {
        loge!("Invalid batch params: {:?}", e);
        return BatchGenerateResult {
            packages: vec![create_empty_package(); queries.len()],
            error_code: e.code as i32,
        };
    }
    
    let mut packages = Vec::with_capacity(queries.len());
    let mut has_error = false;
    
    for (idx, query) in queries.into_iter().enumerate() {
        match generate_single_controlled_package(query) {
            Ok(pkg) => packages.push(pkg),
            Err(e) => {
                loge!("Generate package failed at idx[{}], err={:?}",idx, e);
                packages.push(create_empty_package());
                has_error = true;
            }
        }
    }
    
    BatchGenerateResult {
        packages,
        error_code: if has_error { ErrCode::GeneralError as i32 } else { ErrCode::Success as i32 },
    }
}

fn check_controller_device_id_match(local_udid: &str, package: &RemoteAuthPackage) -> bool {
    local_udid == package.remote_message.device_info.controller_device_id
}

/// Verifies controlled device packages.
pub fn verify_controlled_device_package(
    os_account_id: i32,
    packages: Vec<RemoteAuthPackage>
) -> BatchVerifyResult {
    let permission = CString::new(QUERY_TOOL_PERMISSIONS).unwrap();
    if unsafe { !CheckPermission(permission.as_ptr()) } {
        loge!("Permission denied! Need {}", QUERY_TOOL_PERMISSIONS);
        return BatchVerifyResult {
            results: Vec::new(),
            error_code: ErrCode::PermissionDenied as i32,
        };
    }

    logi!("[verify_controlled_device_package] os_account_id={}, package_count={}", os_account_id, packages.len());
    
    if !(1..=super::MAX_REMOTE_BATCH_COUNT).contains(&packages.len()) {
        loge!("Invalid packages count: {}, max allowed: {}", packages.len(), super::MAX_REMOTE_BATCH_COUNT);
        return BatchVerifyResult {
            results: Vec::new(),
            error_code: ErrCode::InvalidArrayLen as i32,
        };
    }
    
    for (idx, package) in packages.iter().enumerate() {
        log_remote_auth_package(&format!("verify_controlled_device_package[{}]", idx), package);
    }
    
    let local_udid = match saf_common::get_local_udid() {
        Ok(udid) => udid,
        Err(e) => {
            loge!("Failed to get local udid: {:?}", e);
            return BatchVerifyResult {
                results: Vec::new(),
                error_code: e.code as i32,
            };
        }
    };

    let current_time = match system_time_in_millis() {
        Ok(t) => t,
        Err(e) => {
            loge!("Failed to get system time: {:?}", e);
            return BatchVerifyResult {
                results: Vec::new(),
                error_code: e.code as i32,
            };
        }
    };

    let mut results = Vec::with_capacity(packages.len());
    let mut system_error_code = ErrCode::Success as i32;

    for package in packages.iter() {
        match validate_single_controlled_package(package, &local_udid, current_time, os_account_id) {
            Ok(result) => results.push(result),
            Err(e) => {
                loge!("System error during validation: {:?}", e);
                results.push(false);
                if system_error_code == ErrCode::Success as i32 {
                    system_error_code = e.code as i32;
                }
            }
        }
    }
    
    BatchVerifyResult {
        results,
        error_code: system_error_code,
    }
}

fn validate_single_controlled_package(
    package: &RemoteAuthPackage,
    local_udid: &str,
    current_time: u64,
    os_account_id: i32,
) -> Result<bool> {
    if !check_controller_device_id_match(local_udid, package) {
        loge!("Local udid mismatch");
        return Ok(false);
    }

    if let Err(e) = validate_remote_auth_message_fields(&package.remote_message.remote_auth_message) {
        loge!("Invalid remote_auth_message fields: {:?}", e);
        return Ok(false);
    }

    // Verify cross-layer consistency: out challenge must match inner (signed) challenge
    if let Err(e) = super::validate_challenge_consistency(package) {
        loge!("Challenge consistency check failed: {:?}", e);
        return Ok(false);
    }
    
    let timestamp = match parse_timestamp(&package.remote_message.remote_auth_message) {
        Ok(t) => t,
        Err(_) => {
            loge!("Failed to parse timestamp");
            return Ok(false);
        }
    };

    let expire_time_ms = match parse_ticket_expire_time(&package.remote_message.remote_auth_message) {
        Ok(t) => t,
        Err(_) => {
            loge!("Failed to parse ticket expire time");
            return Ok(false);
        }
    };

    if !validate_ticket_expiration(timestamp, expire_time_ms, current_time) {
        loge!("Ticket expired");
        return Ok(false);
    }

    match verify_remote_auth_package(os_account_id, package) {
        Ok(result) => Ok(result),
        Err(e) => {
            if e.code == ErrCode::ArgEmpty {
                loge!("Business failure - missing field: {:?}", e);
                Ok(false)
            } else {
                loge!("System error during verification: {:?}", e);
                Err(e)
            }
        }
    }
}

fn generate_single_controlled_package(query: PermissionQuery) -> Result<RemoteAuthPackage> {
    validate_controlled_permission_query(&query)?;
    
    let (cli_infos, mut api_permissions) = super::parse_cli_and_permission(&query.operation_info)?;
    
    if !cli_infos.is_empty() {
        super::batch_query_cli_permission(&cli_infos, &mut api_permissions)?;
    }
    
    let challenge = generate_crypto_random_challenge()?;
    let timestamp = system_time_in_millis()?;
    
    let remote_auth_message = build_remote_auth_message(&query, api_permissions, &challenge, timestamp)?;
    
    let uid = Skeleton::calling_uid();
    let user_id = get_user_id(uid)?;
    
    logi!("Generate package: user_id={}", user_id);
    
    let sign_params = SignParams {
        os_account_id: user_id,
        uid: query.remote_info.domain_id.clone(),
        remote_auth_package: remote_auth_message,
        remote_control_token: query.remote_info.remote_control_params.remote_control_ticket.clone(),
    };
    
    let sign_result = sign_remote_auth_package(sign_params)?;
    
    cache_challenge(user_id, &challenge, timestamp, &sign_result.device_id_header)?;

    let package = RemoteAuthPackage {
        remote_message: RemoteMessage {
            device_info: sign_result.device_id_header,
            remote_auth_message: sign_result.remote_auth_package,
            caller_bundle_name: String::new(),
        },
        challenge,
        ticket: sign_result.sign_info,
    };
    
    log_remote_auth_package("generate_single_controlled_package", &package);
    
    Ok(package)
}

fn validate_controlled_permission_query(query: &PermissionQuery) -> Result<()> {
    if query.remote_info.role != Role::Controlled {
        return macros_lib::log_throw_error!(ErrCode::DataTypeMismatch, "Invalid role: expected CONTROLLED");
    }
    if query.domain_id.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "domain_id is empty");
    }
    if query.remote_info.remote_control_params.remote_control_ticket.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "remote_control_ticket is empty");
    }
    if query.ticket_expire_time_ms <= 0 || query.ticket_expire_time_ms > super::MAX_REMOTE_TICKET_EXPIRE_TIME_MS {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "ticket_expire_time_ms out of range {}",
            query.ticket_expire_time_ms);
    }
    Ok(())
}

fn build_remote_auth_message(
    query: &PermissionQuery,
    api_permissions: Vec<String>,
    challenge: &str,
    timestamp: u64,
) -> Result<String> {
    let mut builder = JsonBuilder::new();
    serialize_permission_query_to_message(&mut builder, query);
    builder.add_string("challenge", challenge);
    builder.add_u64("timestamp", timestamp);
    builder.add_string_array("permissions", api_permissions);

    builder.build()
}

fn validate_controlled_batch_params(queries: &[PermissionQuery]) -> Result<()> {
    if !(1..=super::MAX_REMOTE_BATCH_COUNT).contains(&queries.len()) {
        return macros_lib::log_throw_error!(ErrCode::InvalidArrayLen,
            "Invalid queries count: {}, max allowed: {}", 
            queries.len(), super::MAX_REMOTE_BATCH_COUNT);
    }
    
    for (idx, query) in queries.iter().enumerate() {
        if query.operation_info.len() > super::MAX_REMOTE_PERMISSION_COUNT {
            return macros_lib::log_throw_error!(ErrCode::InvalidArrayLen,
                "Invalid operation_info count at idx[{}]: {}, max allowed: {}", 
                idx, query.operation_info.len(), super::MAX_REMOTE_PERMISSION_COUNT);
        }
    }
    
    Ok(())
}

#[cfg(feature = "SAFTest")]
pub mod ut_controlled_device_stub {
    include! {"../../../../../test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/remote_control/ut_controlled_device_stub.rs"}
}