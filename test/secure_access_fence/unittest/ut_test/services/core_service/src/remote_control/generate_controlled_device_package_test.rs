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

//! Test cases for generate_controlled_device_package

use saf_definition::{ErrCode, Role};
use secure_access_fence_service::ut_core_service_lib_stub::*;

#[test]
fn test_generate_controlled_device_package_empty() {
    let result = generate_controlled_device_package(vec![]);
    assert!(result.packages.is_empty());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}

#[test]
fn test_generate_controlled_device_package_single_valid_query() {
    reset_mock_state();
    set_mock_sign_success(true);
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "test_ticket",
        vec![create_api_operation("ohos.permission.TEST")],
    );
    let result = generate_controlled_device_package(vec![query]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}

#[test]
fn test_generate_controlled_device_package_multiple_valid_queries() {
    reset_mock_state();
    set_mock_sign_success(true);
    let queries = vec![
        create_test_permission_query(
            Role::Controlled,
            "domain1",
            "ticket1",
            vec![create_api_operation("perm1")],
        ),
        create_test_permission_query(
            Role::Controlled,
            "domain2",
            "ticket2",
            vec![create_cli_operation("cmd", "sub")],
        ),
        create_test_permission_query(
            Role::Controlled,
            "domain3",
            "ticket3",
            vec![create_api_operation("perm2"), create_cli_operation("cmd2", "sub2")],
        ),
    ];
    let result = generate_controlled_device_package(queries);
    assert_eq!(3, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
}

#[test]
fn test_generate_controlled_device_package_invalid_role() {
    let query = create_test_permission_query(
        Role::Controller,
        "test_domain",
        "test_ticket",
        vec![create_api_operation("ohos.permission.TEST")],
    );
    let result = generate_controlled_device_package(vec![query]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
    assert!(result.packages[0].remote_message.device_info.controller_device_id.is_empty());
}

#[test]
fn test_generate_controlled_device_package_empty_domain_id() {
    let query = create_test_permission_query(
        Role::Controlled,
        "",
        "test_ticket",
        vec![create_api_operation("ohos.permission.TEST")],
    );
    let result = generate_controlled_device_package(vec![query]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
    assert!(result.packages[0].remote_message.device_info.controller_device_id.is_empty());
}

#[test]
fn test_generate_controlled_device_package_empty_ticket() {
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "",
        vec![create_api_operation("ohos.permission.TEST")],
    );
    let result = generate_controlled_device_package(vec![query]);
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
    assert!(result.packages[0].remote_message.device_info.controller_device_id.is_empty());
}

#[test]
fn test_generate_controlled_device_package_partial_failure() {
    let queries = vec![
        create_test_permission_query(
            Role::Controlled,
            "valid_domain",
            "valid_ticket",
            vec![create_api_operation("perm")],
        ),
        create_test_permission_query(
            Role::Controller,
            "invalid_domain",
            "invalid_ticket",
            vec![],
        ),
    ];
    let result = generate_controlled_device_package(queries);
    assert_eq!(2, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
}

#[test]
fn test_validate_controlled_permission_query_invalid_role_controller() {
    let query = create_test_permission_query(
        Role::Controller,
        "test_domain",
        "test_ticket",
        vec![],
    );
    let result = validate_controlled_permission_query_stub(&query);
    assert!(result.is_err());
    assert_eq!(ErrCode::DataTypeMismatch, result.unwrap_err().code);
}

#[test]
fn test_validate_controlled_permission_query_empty_domain_id() {
    let query = create_test_permission_query(
        Role::Controlled,
        "",
        "test_ticket",
        vec![],
    );
    let result = validate_controlled_permission_query_stub(&query);
    assert!(result.is_err());
    assert_eq!(ErrCode::ArgEmpty, result.unwrap_err().code);
}

#[test]
fn test_validate_controlled_permission_query_empty_ticket() {
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "",
        vec![],
    );
    let result = validate_controlled_permission_query_stub(&query);
    assert!(result.is_err());
    assert_eq!(ErrCode::ArgEmpty, result.unwrap_err().code);
}

#[test]
fn test_validate_controlled_permission_query_valid() {
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "test_ticket",
        vec![create_api_operation("test_permission")],
    );
    let result = validate_controlled_permission_query_stub(&query);
    assert!(result.is_ok());
}

#[test]
fn test_parse_cli_and_permission_cli_operation() {
    let operations = vec![create_cli_operation("cmd", "sub")];
    let result = parse_cli_and_permission_stub(&operations);
    assert!(result.is_ok());
    let (cli_infos, api_permissions) = result.unwrap();
    assert_eq!(1, cli_infos.len());
    assert!(api_permissions.is_empty());
    assert_eq!("cmd", cli_infos[0].cmd_name);
    assert_eq!("sub", cli_infos[0].sub_cmd);
}

#[test]
fn test_parse_cli_and_permission_api_operation() {
    let operations = vec![create_api_operation("ohos.permission.TEST")];
    let result = parse_cli_and_permission_stub(&operations);
    assert!(result.is_ok());
    let (cli_infos, api_permissions) = result.unwrap();
    assert!(cli_infos.is_empty());
    assert_eq!(1, api_permissions.len());
    assert_eq!("ohos.permission.TEST", api_permissions[0]);
}

#[test]
fn test_parse_cli_and_permission_mixed_operations() {
    let operations = vec![
        create_cli_operation("cmd1", "sub1"),
        create_api_operation("perm1"),
        create_cli_operation("cmd2", "sub2"),
        create_api_operation("perm2"),
    ];
    let result = parse_cli_and_permission_stub(&operations);
    assert!(result.is_ok());
    let (cli_infos, api_permissions) = result.unwrap();
    assert_eq!(2, cli_infos.len());
    assert_eq!(2, api_permissions.len());
}

#[test]
fn test_parse_cli_and_permission_empty_cli_cmd_name() {
    let operations = vec![create_cli_operation("", "sub")];
    let result = parse_cli_and_permission_stub(&operations);
    assert!(result.is_err());
    assert_eq!(ErrCode::ArgEmpty, result.unwrap_err().code);
}

#[test]
fn test_parse_cli_and_permission_empty_api_permission() {
    let operations = vec![create_api_operation("")];
    let result = parse_cli_and_permission_stub(&operations);
    assert!(result.is_err());
    assert_eq!(ErrCode::ArgEmpty, result.unwrap_err().code);
}

#[test]
fn test_create_empty_package() {
    let package = create_empty_package_stub();
    assert!(package.remote_message.device_info.controller_device_id.is_empty());
    assert!(package.remote_message.device_info.controlled_device_id.is_empty());
    assert!(package.remote_message.remote_auth_message.is_empty());
    assert!(package.remote_message.caller_bundle_name.is_empty());
    assert!(package.challenge.is_empty());
    assert!(package.ticket.is_empty());
}

#[test]
fn test_build_remote_auth_message_basic() {
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "test_ticket",
        vec![create_api_operation("test_permission")],
    );
    let cli_infos = vec![];
    let api_permissions = vec!["test_permission".to_string()];
    
    let result = build_remote_auth_message_stub(&query, cli_infos, api_permissions, 1234567890);
    assert!(result.is_ok());
    
    let json = result.unwrap();
    assert!(json.contains("1234567890"));
    assert!(json.contains("test_permission"));
    assert!(json.contains("test_domain"));
}

#[test]
fn test_build_remote_auth_message_with_cli() {
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "test_ticket",
        vec![
            create_cli_operation("test_cmd", "test_sub"),
            create_api_operation("test_permission"),
        ],
    );
    let cli_infos = vec![saf_definition::CommandInfo {
        cmd_name: "test_cmd".to_string(),
        sub_cmd: "test_sub".to_string(),
    }];
    let api_permissions = vec!["test_permission".to_string()];
    
    let result = build_remote_auth_message_stub(&query, cli_infos, api_permissions, 1234567890);
    assert!(result.is_ok());
    
    let json = result.unwrap();
    assert!(json.contains("test_cmd"));
    assert!(json.contains("test_sub"));
}

// ==================== Mock tests ====================

#[test]
fn test_generate_controlled_device_package_sign_success() {
    reset_mock_state();
    set_mock_sign_success(true);
    set_mock_local_udid("test_udid");
    
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "test_ticket",
        vec![create_api_operation("ohos.permission.TEST")],
    );
    let result = generate_controlled_device_package(vec![query]);
    
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::Success as i32, result.error_code);
    assert!(!result.packages[0].ticket.is_empty());
    assert_eq!("mock_controller_udid", result.packages[0].remote_message.device_info.controller_device_id);
}

#[test]
fn test_generate_controlled_device_package_sign_failure() {
    reset_mock_state();
    set_mock_sign_success(false);
    set_mock_local_udid("mock_local_udid");
    
    let query = create_test_permission_query(
        Role::Controlled,
        "test_domain",
        "test_ticket",
        vec![create_api_operation("ohos.permission.TEST")],
    );
    let result = generate_controlled_device_package(vec![query]);
    
    assert_eq!(1, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
    assert!(result.packages[0].ticket.is_empty());
}

#[test]
fn test_generate_controlled_device_package_partial_sign_failure() {
    reset_mock_state();
    set_mock_sign_success(true);
    set_mock_local_udid("mock_local_udid");
    
    let valid_query = create_test_permission_query(
        Role::Controlled,
        "valid_domain",
        "valid_ticket",
        vec![create_api_operation("perm")],
    );
    let invalid_query = create_test_permission_query(
        Role::Controlled,
        "",
        "test_ticket",
        vec![create_api_operation("perm")],
    );
    
    let result = generate_controlled_device_package(vec![valid_query, invalid_query]);
    
    assert_eq!(2, result.packages.len());
    assert_eq!(ErrCode::GeneralError as i32, result.error_code);
}