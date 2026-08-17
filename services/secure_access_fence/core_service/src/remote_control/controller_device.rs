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

//! This module implements controller device package generation and verification.

use ipc::Skeleton;
use saf_common::{get_user_id, JsonBuilder, new_object, object_add_string};
use saf_utils::system_time_in_millis;
use saf_definition::{macros_lib, ErrCode, Result,
    PermissionQuery, RemoteAuthPackage, Role, DeviceIdHeader, RemoteUserAuthItem,
    RemoteUserAuthResults, RemoteInfo, RemoteMessage, GrantType,
};
use crate::remote_control::{create_empty_package, log_remote_auth_package, parse_ticket_expire_time,
    parse_timestamp, serialize_permission_query_to_message, validate_remote_auth_message_fields, BatchGenerateResult, BatchVerifyResult};
use crate::remote_control::grant_record::{store_grant_record, StoreGrantRecordParams};
use crate::remote_control::remote_challenge_manager::verify_and_remove_challenge;
use crate::remote_control::account_based_auth_manager::{sign_remote_auth_package, verify_remote_auth_package, SignParams};
use crate::remote_control::grant_record::get_bundle_name_from_token;
use saf_log::{loge, logi};
use std::ffi::CString;
use std::os::raw::c_char as raw_c_char;

extern "C" {
    fn CheckPermission(permission: *const raw_c_char) -> bool;
}

const QUERY_TOOL_PERMISSIONS: &str = "ohos.permission.QUERY_TOOL_PERMISSIONS";

const MAX_PERMISSION_LEN: usize = 128;

/// Generates controller device packages for remote user auth results.
pub fn generate_controller_device_package(
    remote_user_auth_results: Vec<RemoteUserAuthResults>
) -> BatchGenerateResult {
    let permission = CString::new(QUERY_TOOL_PERMISSIONS).unwrap();
    if unsafe { !CheckPermission(permission.as_ptr()) } {
        loge!("Permission denied! Need {}", QUERY_TOOL_PERMISSIONS);
        return BatchGenerateResult {
            packages: vec![create_empty_package(); remote_user_auth_results.len().max(1)],
            error_code: ErrCode::PermissionDenied as i32,
        };
    }
    
    if let Err(e) = validate_controller_batch_params(&remote_user_auth_results) {
        loge!("Invalid batch params: {:?}", e);
        return BatchGenerateResult {
            packages: vec![create_empty_package(); remote_user_auth_results.len()],
            error_code: e.code as i32,
        };
    }
    
    let uid = Skeleton::calling_uid();
    let user_id = match get_user_id(uid) {
        Ok(id) => id,
        Err(e) => {
            loge!("Failed to get user_id: {:?}", e);
            return BatchGenerateResult {
                packages: vec![create_empty_package(); remote_user_auth_results.len()],
                error_code: ErrCode::InvalidOsAccountId as i32,
            };
        }
    };
    
    let mut packages = Vec::with_capacity(remote_user_auth_results.len());
    let mut has_error = false;
    
    for (idx, auth_result) in remote_user_auth_results.iter().enumerate() {
        match generate_single_controller_package(user_id, auth_result) {
            Ok(pkg) => {
                store_grant_record_if_success(user_id, &pkg, Role::Controller);
                packages.push(pkg);
            },
            Err(e) => {
                loge!("Generate controller package failed at idx[{}], err={:?}", idx, e);
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

fn store_grant_record_if_success(user_id: i32, pkg: &RemoteAuthPackage, role: Role) {
    let caller_bundle_name = super::parse_caller_bundle_name_from_remote_auth_message(
        &pkg.remote_message.remote_auth_message
    ).unwrap_or_default();

    if let Ok(params) = StoreGrantRecordParams::from_remote_auth_package(
        user_id,
        &pkg.remote_message.remote_auth_message,
        role,
        GrantType::RemoteGrant,
        caller_bundle_name
    ) {
        if params.permission_names.is_empty() {
            logi!("No GRANTED permissions found, skip storing grant record");
            return;
        }
        if let Err(e) = store_grant_record(params) {
            loge!("Failed to store grant record: {:?}", e);
        }
    }
}

fn check_role_is_controller(role: Role) -> bool {
    role == Role::Controller
}

/// Verifies controller device packages with remote info.
pub fn verify_controller_device_package(
    os_account_id: i32,
    packages: Vec<RemoteAuthPackage>,
    remote_info: &RemoteInfo
) -> BatchVerifyResult {
    let permission = CString::new(QUERY_TOOL_PERMISSIONS).unwrap();
    if unsafe { !CheckPermission(permission.as_ptr()) } {
        loge!("Permission denied! Need {}", QUERY_TOOL_PERMISSIONS);
        return BatchVerifyResult {
            results: Vec::new(),
            error_code: ErrCode::PermissionDenied as i32,
        };
    }

    logi!("[verify_controller_device_package] os_account_id={}, package_count={}, domain_id={}", 
        os_account_id, packages.len(), remote_info.domain_id);
    
    if packages.is_empty() || packages.len() > super::MAX_REMOTE_BATCH_COUNT {
        loge!("Invalid packages count: {}, max allowed: {}", packages.len(), super::MAX_REMOTE_BATCH_COUNT);
        return BatchVerifyResult {
            results: Vec::new(),
            error_code: ErrCode::InvalidArrayLen as i32,
        };
    }
    
    for (_idx, package) in packages.iter().enumerate() {
        log_remote_auth_package(package);
    }
    
    if !check_role_is_controller(remote_info.role) {
        loge!("Invalid role for verification: expected CONTROLLER");
        return BatchVerifyResult {
            results: Vec::new(),
            error_code: ErrCode::DataTypeMismatch as i32,
        };
    }
    
    let local_udid = match crate::wrapper::get_device_udid(os_account_id) {
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
        match validate_and_verify_single_controller_package(
            package, &local_udid, current_time, os_account_id
        ) {
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

fn validate_and_verify_single_controller_package(
    package: &RemoteAuthPackage,
    local_udid: &str,
    current_time: u64,
    os_account_id: i32,
) -> Result<bool> {
    let verify_result = match verify_single_controller_package(
        os_account_id, package, local_udid, current_time
    ) {
        Ok(result) => result,
        Err(e) => {
            if e.code == ErrCode::ArgEmpty || e.code == ErrCode::ReplayAttackDetected {
                loge!("Business failure: {:?}", e);
                return Ok(false);
            } else {
                loge!("System error during verification: {:?}", e);
                return Err(e);
            }
        }
    };

    Ok(verify_result)
}

fn generate_single_controller_package(
    os_account_id: i32,
    auth_result: &RemoteUserAuthResults
) -> Result<RemoteAuthPackage> {
    let (local_udid, api_permissions, challenge, timestamp) =
        prepare_controller_package_data(os_account_id, auth_result)?;

    let uid = Skeleton::calling_uid();
    let user_id = get_user_id(uid)?;

    logi!("Generate controller package: user_id={}", user_id);

    let package = build_and_sign_controller_package(
        &auth_result.permission_query,
        &auth_result.results,
        challenge,
        &local_udid,
        &api_permissions,
        user_id,
        timestamp,
    )?;

    log_remote_auth_package(&package);

    Ok(package)
}

fn prepare_controller_package_data(
    os_account_id: i32,
    auth_result: &RemoteUserAuthResults
) -> Result<(String, Vec<String>, String, u64)> {
    validate_controller_permission_query(&auth_result.permission_query)?;
    validate_auth_results_permissions(&auth_result.results)?;

    let local_udid = crate::wrapper::get_device_udid(os_account_id)?;

    let (cli_infos, mut api_permissions) =
        super::parse_cli_and_permission(&auth_result.permission_query.operation_info)?;

    if !cli_infos.is_empty() {
        super::batch_query_cli_permission(&cli_infos, &mut api_permissions)?;
    }

    validate_permissions_match(&auth_result.results, &api_permissions)?;

    let challenge = auth_result.permission_query.remote_info.remote_control_params.challenge.clone();
    if challenge.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "challenge is empty");
    }
    
    let timestamp = system_time_in_millis()?;

    Ok((local_udid, api_permissions, challenge, timestamp))
}

fn build_and_sign_controller_package(
    query: &PermissionQuery,
    auth_results: &[RemoteUserAuthItem],
    challenge: String,
    local_udid: &str,
    api_permissions: &[String],
    user_id: i32,
    timestamp: u64,
) -> Result<RemoteAuthPackage> {
    let caller_bundle_name = get_bundle_name_from_token(query.caller_token_id)
        .unwrap_or_default();
    
    let remote_auth_message = build_controller_remote_auth_message(
        query, auth_results, &challenge, timestamp, local_udid, api_permissions, &caller_bundle_name
    )?;

    let sign_params = SignParams {
        os_account_id: user_id,
        uid: query.remote_info.domain_id.clone(),
        remote_auth_package: remote_auth_message,
        remote_control_token: String::new(),
    };

    let sign_result = sign_remote_auth_package(sign_params)?;

    Ok(RemoteAuthPackage {
        remote_message: RemoteMessage {
            device_info: sign_result.device_id_header,
            remote_auth_message: sign_result.remote_auth_package,
            caller_bundle_name,
        },
        challenge,
        ticket: sign_result.sign_info,
    })
}

fn verify_single_controller_package(
    os_account_id: i32,
    package: &RemoteAuthPackage,
    local_udid: &str,
    current_time: u64
) -> Result<bool> {
    // Validate cross-layer consistency first
    let _inner_challenge = match super::validate_challenge_consistency(package) {
        Ok(c) => c,
        Err(e) => {
            loge!("Challenge consistency check failed: {:?}", e);
            return Ok(false);
        }
    };

        if let Err(e) = validate_remote_auth_message_fields(&package.remote_message.remote_auth_message) {
        loge!("Invalid remote_auth_message fields: {:?}", e);
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

    if !crate::remote_control::validate_ticket_expiration(timestamp, expire_time_ms, current_time) {
        loge!("Ticket expired");
        return Ok(false);
    }
    
    let verify_result = verify_remote_auth_package(os_account_id, package)?;
    if !verify_result {
        return Ok(false);
    }

    let controller_device_id = match super::parse_local_device_id_from_remote_auth_message(
        &package.remote_message.remote_auth_message
    ) {
        Ok(id) => id,
        Err(e) => {
            loge!("Failed to parse localDeviceId from remote_auth_message: {:?}", e);
            return Ok(false);
        }
    };
    
    let device_id_header = DeviceIdHeader {
        controlled_device_id: local_udid.to_string(),
        controller_device_id,
    };
    
    verify_and_remove_challenge(os_account_id, &package.challenge, &device_id_header)?;
    
    Ok(true)
}

fn validate_controller_permission_query(query: &PermissionQuery) -> Result<()> {
    if query.remote_info.role != Role::Controller {
        return macros_lib::log_throw_error!(ErrCode::DataTypeMismatch,
            "Invalid role: expected CONTROLLER");
    }

    if query.domain_id.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "domain_id is empty");
    }

    if query.remote_info.remote_control_params.challenge.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "challenge is empty");
    }

    if query.operation_info.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "operation_info is empty");
    }
    if query.ticket_expire_time_ms <= 0 || query.ticket_expire_time_ms > super::MAX_REMOTE_TICKET_EXPIRE_TIME_MS {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "ticket_expire_time_ms out of range {}",
            query.ticket_expire_time_ms);
    }
    Ok(())
}

fn validate_auth_results_permissions(results: &[RemoteUserAuthItem]) -> Result<()> {
    if results.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "authResults is empty");
    }
    
    for (idx, item) in results.iter().enumerate() {
        if item.permission.is_empty() {
            return macros_lib::log_throw_error!(ErrCode::ArgEmpty,
                "Permission at idx[{}] is empty", idx);
        }
        if item.permission.len() >= MAX_PERMISSION_LEN {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgSize,
                "Permission at idx[{}] exceeds max length {}, actual len={}",
                idx, MAX_PERMISSION_LEN, item.permission.len());
        }
    }
    Ok(())
}

fn validate_permissions_match(results: &[RemoteUserAuthItem], api_permissions: &[String]) -> Result<()> {
    let results_permissions: std::collections::HashSet<&str> = results
        .iter()
        .map(|r| r.permission.as_str())
        .collect();
    
    let api_permissions_set: std::collections::HashSet<&str> = api_permissions
        .iter()
        .map(|p| p.as_str())
        .collect();
    
    if results_permissions != api_permissions_set {
        loge!("Permissions mismatch: results_count={}, api_permissions_count={}", 
            results_permissions.len(), api_permissions_set.len());
        return macros_lib::log_throw_error!(ErrCode::DataTypeMismatch,
            "Permissions in results do not match permissions from operationInfo");
    }
    
    Ok(())
}

fn build_controller_remote_auth_message(
    query: &PermissionQuery,
    auth_results: &[RemoteUserAuthItem],
    challenge: &str,
    timestamp: u64,
    local_device_id: &str,
    permissions: &[String],
    caller_bundle_name: &str
) -> Result<String> {
    let mut builder = JsonBuilder::new();
    serialize_permission_query_to_message(&mut builder, query);

    builder.add_string("challenge", challenge);
    builder.add_u64("timestamp", timestamp);
    
    let mut auth_results_array = Vec::with_capacity(auth_results.len());
    for item in auth_results {
        let mut item_obj = new_object();
        object_add_string(&mut item_obj, "permission", &item.permission);
        object_add_string(&mut item_obj, "authResult", &item.auth_result);
        auth_results_array.push(item_obj);
    }
    builder.add_object_array("authResults", auth_results_array);

    builder.add_string_array("permissions", permissions.to_vec());
    builder.add_string("localDeviceId", local_device_id);
    builder.add_string("callerBundleName", caller_bundle_name);
    builder.add_number("version", 1 as i64);

    builder.build()
}

fn validate_controller_batch_params(auth_results: &[RemoteUserAuthResults]) -> Result<()> {
    if auth_results.is_empty() || auth_results.len() > super::MAX_REMOTE_BATCH_COUNT {
        return macros_lib::log_throw_error!(ErrCode::InvalidArrayLen,
            "Invalid auth_results count: {}, max allowed: {}", 
            auth_results.len(), super::MAX_REMOTE_BATCH_COUNT);
    }
    
    for (idx, auth_result) in auth_results.iter().enumerate() {
        if auth_result.permission_query.operation_info.len() > super::MAX_REMOTE_PERMISSION_COUNT {
            return macros_lib::log_throw_error!(ErrCode::InvalidArrayLen,
                "Invalid operation_info count at idx[{}]: {}, max allowed: {}", 
                idx, auth_result.permission_query.operation_info.len(), super::MAX_REMOTE_PERMISSION_COUNT);
        }
        
        if auth_result.results.len() > super::MAX_REMOTE_PERMISSION_COUNT {
            return macros_lib::log_throw_error!(ErrCode::InvalidArrayLen,
                "Invalid results count at idx[{}]: {}, max allowed: {}", 
                idx, auth_result.results.len(), super::MAX_REMOTE_PERMISSION_COUNT);
        }
    }
    
    Ok(())
}

#[cfg(feature = "SAFTest")]
pub mod ut_controller_device_stub {
    include! {"../../../../../test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/remote_control/ut_controller_device_stub.rs"}
}