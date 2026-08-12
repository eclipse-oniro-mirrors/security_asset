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

//! Test cases for verify_controlled_device_package and verify_controller_device_package

use saf_definition::{Role, ErrCode};
use secure_access_fence_service::ut_core_service_lib_stub::*;

// ==================== verify_controlled_device_package tests ====================

#[test]
fn test_verify_controlled_device_package_empty() {
    let result = verify_controlled_device_package(100, vec![]);
    assert!(result.results.is_empty());
    assert_eq!(result.error_code, ErrCode::InvalidArrayLen as i32);
}

#[test]
fn test_check_controller_device_id_match_true() {
    let package = create_test_remote_auth_package("test_udid", "controlled_udid", "1234567890");
    assert!(check_controller_device_id_match_stub("test_udid", &package));
}

#[test]
fn test_check_controller_device_id_match_false() {
    let package = create_test_remote_auth_package("other_udid", "controlled_udid", "1234567890");
    assert!(!check_controller_device_id_match_stub("test_udid", &package));
}

#[test]
fn test_check_controller_device_id_match_empty() {
    let package = create_test_remote_auth_package("", "controlled_udid", "1234567890");
    assert!(!check_controller_device_id_match_stub("test_udid", &package));
}

#[test]
fn test_parse_challenge_valid() {
    assert_eq!(Some(1234567890u64), parse_challenge_stub("1234567890"));
}

#[test]
fn test_parse_challenge_valid_large() {
    assert_eq!(Some(18446744073709551615u64), parse_challenge_stub("18446744073709551615"));
}

#[test]
fn test_parse_challenge_invalid_format() {
    assert_eq!(None, parse_challenge_stub("invalid"));
}

#[test]
fn test_parse_challenge_empty() {
    assert_eq!(None, parse_challenge_stub(""));
}

#[test]
fn test_parse_challenge_negative() {
    assert_eq!(None, parse_challenge_stub("-123"));
}

#[test]
fn test_validate_ticket_expiration_not_expired() {
    assert!(validate_ticket_expiration_stub(1000, 3600000, 2000));
}

#[test]
fn test_validate_ticket_expiration_exactly_not_expired() {
    assert!(validate_ticket_expiration_stub(1000, 1000, 2000));
}

#[test]
fn test_validate_ticket_expiration_expired() {
    assert!(!validate_ticket_expiration_stub(1000, 100, 2000));
}

#[test]
fn test_validate_ticket_expiration_expired_exactly() {
    assert!(!validate_ticket_expiration_stub(1000, 999, 2000));
}

#[test]
fn test_validate_ticket_expiration_zero_expire_time() {
    assert!(validate_ticket_expiration_stub(1000, 0, 500));
}

// ==================== verify_controller_device_package tests ====================

#[test]
fn test_verify_controller_device_package_empty() {
    let remote_info = create_test_remote_info(Role::Controller, "test_remote_id");
    let result = verify_controller_device_package(100, vec![], &remote_info);
    assert!(result.results.is_empty());
    assert_eq!(result.error_code, ErrCode::InvalidArrayLen as i32);
}

#[test]
fn test_check_role_is_controller_true() {
    assert!(check_role_is_controller_stub(Role::Controller));
}

#[test]
fn test_check_role_is_controller_false() {
    assert!(!check_role_is_controller_stub(Role::Controlled));
}

// ==================== Mock tests ====================

#[test]
fn test_verify_controlled_device_package_with_mock() {
    reset_mock_state();
    set_mock_local_udid("mock_controller_udid");
    set_mock_verify_success(true);
    
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let result = verify_controlled_device_package(100, vec![package]);
    
    assert_eq!(result.results.len(), 1);
    assert!(result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controlled_device_package_udid_mismatch_with_mock() {
    reset_mock_state();
    set_mock_local_udid("different_udid");
    set_mock_verify_success(true);
    
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let result = verify_controlled_device_package(100, vec![package]);
    
    assert_eq!(result.results.len(), 1);
    assert!(!result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controller_device_package_with_mock() {
    reset_mock_state();
    set_mock_local_udid("mock_local_udid");
    set_mock_verify_success(true);
    
    let remote_info = create_test_remote_info(Role::Controller, "mock_controller_udid");
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let result = verify_controller_device_package(100, vec![package], &remote_info);
    
    assert_eq!(result.results.len(), 1);
    assert!(result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controller_device_package_role_mismatch() {
    let remote_info = create_test_remote_info(Role::Controlled, "mock_controller_udid");
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let result = verify_controller_device_package(100, vec![package], &remote_info);
    
    assert!(result.results.is_empty());
    assert_eq!(result.error_code, ErrCode::DataTypeMismatch as i32);
}

#[test]
fn test_verify_controller_device_package_verify_failure() {
    reset_mock_state();
    set_mock_local_udid("mock_local_udid");
    set_mock_verify_success(false);
    
    let remote_info = create_test_remote_info(Role::Controller, "mock_controller_udid");
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let result = verify_controller_device_package(100, vec![package], &remote_info);
    
    assert_eq!(result.results.len(), 1);
    assert!(!result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

// ==================== Exception tests ====================

#[test]
fn test_verify_controlled_device_package_invalid_challenge() {
    reset_mock_state();
    set_mock_local_udid("mock_controller_udid");
    set_mock_verify_success(true);
    
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "invalid_challenge");
    let result = verify_controlled_device_package(100, vec![package]);
    
    assert_eq!(result.results.len(), 1);
    assert!(!result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controlled_device_package_empty_challenge() {
    reset_mock_state();
    set_mock_local_udid("mock_controller_udid");
    set_mock_verify_success(true);
    
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "");
    let result = verify_controlled_device_package(100, vec![package]);
    
    assert_eq!(result.results.len(), 1);
    assert!(!result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controlled_device_package_multiple_packages_partial_failure() {
    reset_mock_state();
    set_mock_local_udid("mock_controller_udid");
    set_mock_verify_success(true);
    
    let valid_package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let invalid_package = create_test_remote_auth_package("different_udid", "controlled_udid", "1234567890");
    
    let result = verify_controlled_device_package(100, vec![valid_package, invalid_package]);
    
    assert_eq!(result.results.len(), 2);
    assert!(result.results[0]);
    assert!(!result.results[1]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controller_device_package_invalid_challenge() {
    reset_mock_state();
    set_mock_local_udid("mock_local_udid");
    set_mock_verify_success(true);
    
    let remote_info = create_test_remote_info(Role::Controller, "mock_controller_udid");
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "invalid_challenge");
    let result = verify_controller_device_package(100, vec![package], &remote_info);
    
    assert_eq!(result.results.len(), 1);
    assert!(!result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controller_device_package_empty_challenge() {
    reset_mock_state();
    set_mock_local_udid("mock_local_udid");
    set_mock_verify_success(true);
    
    let remote_info = create_test_remote_info(Role::Controller, "mock_controller_udid");
    let package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "");
    let result = verify_controller_device_package(100, vec![package], &remote_info);
    
    assert_eq!(result.results.len(), 1);
    assert!(!result.results[0]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}

#[test]
fn test_verify_controller_device_package_multiple_packages_partial_failure() {
    reset_mock_state();
    set_mock_local_udid("mock_local_udid");
    set_mock_verify_success(true);
    
    let remote_info = create_test_remote_info(Role::Controller, "mock_controller_udid");
    let valid_package = create_test_remote_auth_package("mock_controller_udid", "controlled_udid", "1234567890");
    let invalid_package = create_test_remote_auth_package("different_id", "controlled_udid", "1234567890");
    
    let result = verify_controller_device_package(100, vec![valid_package, invalid_package], &remote_info);
    
    assert_eq!(result.results.len(), 2);
    assert!(result.results[0]);
    assert!(!result.results[1]);
    assert_eq!(result.error_code, ErrCode::Success as i32);
}