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

//! Test cases for generate_controller_device_package

use saf_definition::{ErrCode, Role, RemoteUserAuthItem, RemoteControlParams, RemoteInfo, PermissionQuery};
use secure_access_fence_service::ut_core_service_lib_stub::*;

#[test]
fn test_generate_controller_device_package_empty() {
    let result = generate_controller_device_package(vec![]);
    assert!(result.packages.is_empty());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}

#[test]
fn test_generate_controller_device_package_single_valid() {
    let auth_result = create_test_remote_user_auth_results("ohos.permission.TEST", "granted");
    let result = generate_controller_device_package(vec![auth_result]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}

#[test]
fn test_generate_controller_device_package_multiple_valid() {
    let auth_results = vec![
        create_test_remote_user_auth_results("perm1", "granted"),
        create_test_remote_user_auth_results("perm2", "denied"),
        create_test_remote_user_auth_results("perm3", "granted"),
    ];
    let result = generate_controller_device_package(auth_results);
    assert_eq!(3, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}

#[test]
fn test_generate_controller_device_package_invalid_role() {
    let auth_result = saf_definition::RemoteUserAuthResults {
        results: vec![RemoteUserAuthItem {
            permission: "perm".to_string(),
            auth_result: "granted".to_string(),
        }],
        permission_query: PermissionQuery {
            operation_info: vec![],
            need_ticket: true,
            ticket_expire_time_ms: 3600000,
            caller_token_id: 0,
            domain_id: "test_domain".to_string(),
            remote_info: RemoteInfo {
                role: Role::Controlled,
                remote_id: "test_remote_id".to_string(),
                domain_id: "test_domain".to_string(),
                remote_control_params: RemoteControlParams {
                    challenge: "1234567890".to_string(),
                    remote_control_ticket: "test_ticket".to_string(),
                    controlled_device_name: "controlled".to_string(),
                    controller_device_name: "controller".to_string(),
                    sign_verify_msg: String::new(),
                },
            },
        },
    };
    let result = generate_controller_device_package(vec![auth_result]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
}

#[test]
fn test_generate_controller_device_package_empty_challenge() {
    let auth_result = saf_definition::RemoteUserAuthResults {
        results: vec![RemoteUserAuthItem {
            permission: "perm".to_string(),
            auth_result: "granted".to_string(),
        }],
        permission_query: PermissionQuery {
            operation_info: vec![],
            need_ticket: true,
            ticket_expire_time_ms: 3600000,
            caller_token_id: 0,
            domain_id: "test_domain".to_string(),
            remote_info: RemoteInfo {
                role: Role::Controller,
                remote_id: "test_remote_id".to_string(),
                domain_id: "test_domain".to_string(),
                remote_control_params: RemoteControlParams {
                    challenge: "".to_string(),
                    remote_control_ticket: "test_ticket".to_string(),
                    controlled_device_name: "controlled".to_string(),
                    controller_device_name: "controller".to_string(),
                    sign_verify_msg: String::new(),
                },
            },
        },
    };
    let result = generate_controller_device_package(vec![auth_result]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
}

#[test]
fn test_generate_controller_device_package_invalid_challenge_format() {
    let auth_result = saf_definition::RemoteUserAuthResults {
        results: vec![RemoteUserAuthItem {
            permission: "perm".to_string(),
            auth_result: "granted".to_string(),
        }],
        permission_query: PermissionQuery {
            operation_info: vec![],
            need_ticket: true,
            ticket_expire_time_ms: 3600000,
            caller_token_id: 0,
            domain_id: "test_domain".to_string(),
            remote_info: RemoteInfo {
                role: Role::Controller,
                remote_id: "test_remote_id".to_string(),
                domain_id: "test_domain".to_string(),
                remote_control_params: RemoteControlParams {
                    challenge: "invalid_challenge".to_string(),
                    remote_control_ticket: "test_ticket".to_string(),
                    controlled_device_name: "controlled".to_string(),
                    controller_device_name: "controller".to_string(),
                    sign_verify_msg: String::new(),
                },
            },
        },
    };
    let result = generate_controller_device_package(vec![auth_result]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
}

#[test]
fn test_generate_controller_device_package_partial_failure() {
    let valid_auth = create_test_remote_user_auth_results("perm1", "granted");
    let invalid_auth = saf_definition::RemoteUserAuthResults {
        results: vec![RemoteUserAuthItem {
            permission: "perm2".to_string(),
            auth_result: "granted".to_string(),
        }],
        permission_query: PermissionQuery {
            operation_info: vec![],
            need_ticket: true,
            ticket_expire_time_ms: 3600000,
            caller_token_id: 0,
            domain_id: "test_domain".to_string(),
            remote_info: RemoteInfo {
                role: Role::Controlled,
                remote_id: "test_remote_id".to_string(),
                domain_id: "test_domain".to_string(),
                remote_control_params: RemoteControlParams {
                    challenge: "1234567890".to_string(),
                    remote_control_ticket: "test_ticket".to_string(),
                    controlled_device_name: "controlled".to_string(),
                    controller_device_name: "controller".to_string(),
                    sign_verify_msg: String::new(),
                },
            },
        },
    };
    let result = generate_controller_device_package(vec![valid_auth, invalid_auth]);
    assert_eq!(2, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
}

#[test]
fn test_validate_controller_permission_query_valid() {
    let query = PermissionQuery {
        operation_info: vec![],
        need_ticket: true,
        ticket_expire_time_ms: 3600000,
        caller_token_id: 0,
        domain_id: "test_domain".to_string(),
        remote_info: RemoteInfo {
            role: Role::Controller,
            remote_id: "test_remote_id".to_string(),
            domain_id: "test_domain".to_string(),
            remote_control_params: RemoteControlParams {
                challenge: "1234567890".to_string(),
                remote_control_ticket: "test_ticket".to_string(),
                controlled_device_name: "controlled".to_string(),
                controller_device_name: "controller".to_string(),
                sign_verify_msg: String::new(),
            },
        },
    };
    let result = validate_controller_permission_query_stub(&query);
    assert!(result.is_ok());
}

#[test]
fn test_validate_controller_permission_query_invalid_role() {
    let query = PermissionQuery {
        operation_info: vec![],
        need_ticket: true,
        ticket_expire_time_ms: 3600000,
        caller_token_id: 0,
        domain_id: "test_domain".to_string(),
        remote_info: RemoteInfo {
            role: Role::Controlled,
            remote_id: "test_remote_id".to_string(),
            domain_id: "test_domain".to_string(),
            remote_control_params: RemoteControlParams {
                challenge: "1234567890".to_string(),
                remote_control_ticket: "test_ticket".to_string(),
                controlled_device_name: "controlled".to_string(),
                controller_device_name: "controller".to_string(),
                sign_verify_msg: String::new(),
            },
        },
    };
    let result = validate_controller_permission_query_stub(&query);
    assert!(result.is_err());
    assert_eq!(ErrCode::DataTypeMismatch, result.unwrap_err().code);
}

#[test]
fn test_build_auth_results_json_single() {
    let results = vec![RemoteUserAuthItem {
        permission: "perm1".to_string(),
        auth_result: "granted".to_string(),
    }];
    let result = build_auth_results_json_stub(&results);
    assert!(result.is_ok());
    let json = result.unwrap();
    assert!(json.contains("perm1"));
    assert!(json.contains("granted"));
}

#[test]
fn test_build_auth_results_json_multiple() {
    let results = vec![
        RemoteUserAuthItem {
            permission: "perm1".to_string(),
            auth_result: "granted".to_string(),
        },
        RemoteUserAuthItem {
            permission: "perm2".to_string(),
            auth_result: "denied".to_string(),
        },
    ];
    let result = build_auth_results_json_stub(&results);
    assert!(result.is_ok());
    let json = result.unwrap();
    assert!(json.contains("perm1"));
    assert!(json.contains("perm2"));
}

#[test]
fn test_build_auth_results_json_empty() {
    let results: Vec<RemoteUserAuthItem> = vec![];
    let result = build_auth_results_json_stub(&results);
    assert!(result.is_ok());
}

// ==================== Mock tests ====================

#[test]
fn test_generate_controller_device_package_sign_success() {
    reset_mock_state();
    set_mock_sign_success(true);
    set_mock_local_udid("test_udid");
    
    let auth_result = create_test_remote_user_auth_results("ohos.permission.TEST", "granted");
    let result = generate_controller_device_package(vec![auth_result]);
    
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
    assert!(!result.packages[0].ticket.is_empty());
}

#[test]
fn test_generate_controller_device_package_sign_failure() {
    reset_mock_state();
    set_mock_sign_success(false);
    set_mock_local_udid("mock_local_udid");
    
    let auth_result = create_test_remote_user_auth_results("ohos.permission.TEST", "granted");
    let result = generate_controller_device_package(vec![auth_result]);
    
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
    assert!(result.packages[0].ticket.is_empty());
}

#[test]
fn test_generate_controller_device_package_multiple_with_mock() {
    reset_mock_state();
    set_mock_sign_success(true);
    set_mock_local_udid("mock_local_udid");
    
    let auth_results = vec![
        create_test_remote_user_auth_results("perm1", "granted"),
        create_test_remote_user_auth_results("perm2", "denied"),
    ];
    let result = generate_controller_device_package(auth_results);
    
    assert_eq!(2, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}