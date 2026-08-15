/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/// the module test stub for controlled_device

use saf_definition::{
    CommandInfo, OperationInfo, OperationType, PermissionQuery, RemoteInfo,
    RemoteControlParams, Role,
};
use crate::remote_control::controlled_device::*;

pub use crate::remote_control::{
    BatchGenerateResult,
    BatchVerifyResult,
    generate_controlled_device_package,
    verify_controlled_device_package,
};

/// validate_controlled_permission_query stub
#[allow(dead_code)]
pub fn validate_controlled_permission_query_stub(query: &PermissionQuery) -> saf_definition::Result<()> {
    validate_controlled_permission_query(query)
}

/// parse_cli_and_permission stub
#[allow(dead_code)]
pub fn parse_cli_and_permission_stub(
    operation_info: &[OperationInfo]
) -> saf_definition::Result<(Vec<CommandInfo>, Vec<String>)> {
    parse_cli_and_permission(operation_info)
}

/// build_remote_auth_message stub
#[allow(dead_code)]
pub fn build_remote_auth_message_stub(
    query: &PermissionQuery,
    cli_infos: Vec<CommandInfo>,
    api_permissions: Vec<String>,
    challenge: u64,
) -> saf_definition::Result<String> {
    build_remote_auth_message(query, cli_infos, api_permissions, challenge)
}

/// create_empty_package stub
#[allow(dead_code)]
pub fn create_empty_package_stub() -> saf_definition::RemoteAuthPackage {
    crate::remote_control::create_empty_package()
}

/// check_controller_device_id_match stub
#[allow(dead_code)]
pub fn check_controller_device_id_match_stub(local_udid: &str, package: &saf_definition::RemoteAuthPackage) -> bool {
    check_controller_device_id_match(local_udid, package)
}

/// parse_challenge stub
#[allow(dead_code)]
pub fn parse_challenge_stub(challenge_str: &str) -> Option<u64> {
    crate::remote_control::parse_challenge(challenge_str)
}

/// validate_ticket_expiration stub
#[allow(dead_code)]
pub fn validate_ticket_expiration_stub(challenge: u64, expire_time_ms: u64, current_time: u64) -> bool {
    match challenge.checked_add(expire_time_ms) {
        Some(expire_time) => expire_time >= current_time,
        None => false,
    }
}

/// create_test_remote_auth_package
pub fn create_test_remote_auth_package(controller_id: &str, controlled_id: &str, challenge: &str) -> saf_definition::RemoteAuthPackage {
    let expire_time_ms: u64 = 86400000; // 24h
    let now_ms: u64 = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_millis() as u64;
    let challenge_value = match challenge.parse::<u64>() {
        Ok(_) => now_ms.to_string(),
        Err(_) => challenge.to_string(),
    };
    saf_definition::RemoteAuthPackage {
        remote_message: saf_definition::RemoteMessage {
            device_info: saf_definition::DeviceIdHeader {
                controller_device_id: controller_id.to_string(),
                controlled_device_id: controlled_id.to_string(),
            },
            remote_auth_message: format!("{{\"ticketExpireTimeMs\":\"{}\"}}", expire_time_ms),
            caller_bundle_name: String::new(),
        },
        challenge: challenge_value,
        ticket: String::new(),
    }
}

/// create_test_permission_query
pub fn create_test_permission_query(
    role: Role,
    domain_id: &str,
    remote_control_ticket: &str,
    operation_info: Vec<OperationInfo>,
) -> PermissionQuery {
    PermissionQuery {
        operation_info,
        need_ticket: true,
        ticket_expire_time_ms: 3600000,
        caller_token_id: 0,
        domain_id: domain_id.to_string(),
        remote_info: RemoteInfo {
            role,
            remote_id: "test_remote_id".to_string(),
            domain_id: domain_id.to_string(),
            remote_control_params: RemoteControlParams {
                challenge: "1234567890".to_string(),
                remote_control_ticket: remote_control_ticket.to_string(),
                controlled_device_name: "controlled_device".to_string(),
                controller_device_name: "controller_device".to_string(),
                sign_verify_msg: "sign_verify_msg".to_string(),
            },
        },
    }
}

/// create_cli_operation
pub fn create_cli_operation(cmd_name: &str, sub_cmd: &str) -> OperationInfo {
    OperationInfo {
        operation_type: OperationType::Cli,
        cli_cmd_info: CommandInfo {
            cmd_name: cmd_name.to_string(),
            sub_cmd: sub_cmd.to_string(),
        },
        permission: String::new(),
    }
}

/// create_api_operation
pub fn create_api_operation(permission: &str) -> OperationInfo {
    OperationInfo {
        operation_type: OperationType::Api,
        cli_cmd_info: CommandInfo {
            cmd_name: String::new(),
            sub_cmd: String::new(),
        },
        permission: permission.to_string(),
    }
}

/// set_mock_local_udid
pub fn set_mock_local_udid(udid: &str) {
    saf_common::set_mock_local_udid(udid);
}

/// set_mock_sign_success
pub fn set_mock_sign_success(success: bool) {
    crate::remote_control::account_based_auth_manager::set_mock_sign_success(success);
}

/// set_mock_verify_success
pub fn set_mock_verify_success(success: bool) {
    crate::remote_control::account_based_auth_manager::set_mock_verify_success(success);
}

/// reset all mock state to defaults
pub fn reset_mock_state() {
    crate::remote_control::account_based_auth_manager::reset_mock_state();
    saf_common::reset_mock_local_udid();
}