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

/// the module test stub for controller_device

use saf_definition::{Role, RemoteUserAuthItem, PermissionQuery};
use crate::remote_control::controller_device::*;

pub use crate::remote_control::{
    BatchGenerateResult,
    BatchVerifyResult,
    generate_controller_device_package,
    verify_controller_device_package,
};

/// validate_controller_permission_query stub
#[allow(dead_code)]
pub fn validate_controller_permission_query_stub(query: &PermissionQuery) -> saf_definition::Result<()> {
    validate_controller_permission_query(query)
}

/// build_auth_results_json stub
#[allow(dead_code)]
pub fn build_auth_results_json_stub(results: &[RemoteUserAuthItem]) -> saf_definition::Result<String> {
    build_auth_results_json(results)
}

/// build_controller_remote_auth_message stub
#[allow(dead_code)]
pub fn build_controller_remote_auth_message_stub(
    query: &PermissionQuery,
    auth_results_json: &str,
    challenge: u64,
    local_device_id: &str,
    permissions: &[String],
) -> saf_definition::Result<String> {
    build_controller_remote_auth_message(query, auth_results_json, challenge, local_device_id, permissions)
}

/// check_role_is_controller stub
#[allow(dead_code)]
pub fn check_role_is_controller_stub(role: saf_definition::Role) -> bool {
    check_role_is_controller(role)
}

use crate::remote_control::controlled_device::ut_controlled_device_stub::{create_test_permission_query, create_api_operation};

/// create_test_remote_user_auth_results
pub fn create_test_remote_user_auth_results(
    permission: &str,
    auth_result: &str,
) -> saf_definition::RemoteUserAuthResults {
    saf_definition::RemoteUserAuthResults {
        results: vec![RemoteUserAuthItem {
            permission: permission.to_string(),
            auth_result: auth_result.to_string(),
        }],
        permission_query: create_test_permission_query(
            Role::Controller,
            "test_domain",
            "test_ticket",
            vec![create_api_operation(permission)],
        ),
    }
}

/// create_test_remote_info
pub fn create_test_remote_info(role: saf_definition::Role, remote_id: &str) -> saf_definition::RemoteInfo {
    saf_definition::RemoteInfo {
        role,
        remote_id: remote_id.to_string(),
        domain_id: "test_domain".to_string(),
        remote_control_params: saf_definition::RemoteControlParams {
            challenge: "1234567890".to_string(),
            remote_control_ticket: "test_ticket".to_string(),
            controlled_device_name: "controlled".to_string(),
            controller_device_name: "controller".to_string(),
            sign_verify_msg: String::new(),
        },
    }
}