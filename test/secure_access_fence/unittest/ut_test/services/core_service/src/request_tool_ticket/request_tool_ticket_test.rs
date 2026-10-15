// SPDX-License-Identifier: Apache-2.0

use saf_definition::ErrCode;
use secure_access_fence_service::ut_request_tool_ticket_stub::{
    do_request_tool_ticket, filter_permission_results, reset_mocks, set_mock_cxx_result, set_mock_permission,
    validate_cli_cmd_infos, validate_ticket_expire_time_ms, CxxCommandInfo, CxxPermissionInfo, CxxTicketQueryResult,
};

fn make_perm(permission: &str, auth_status: i32) -> CxxPermissionInfo {
    CxxPermissionInfo { permission: permission.to_string(), permission_status: 0, auth_status, flag: 0 }
}

fn make_cmd(cmd_name: &str, sub_cmd: &str) -> CxxCommandInfo {
    CxxCommandInfo { cmd_name: cmd_name.to_string(), sub_cmd: sub_cmd.to_string() }
}

fn make_result(perms: Vec<CxxPermissionInfo>, has_ticket: bool, ticket: &str) -> CxxTicketQueryResult {
    CxxTicketQueryResult {
        permission_results: perms,
        has_ticket,
        ticket_message: if has_ticket { "msg".to_string() } else { String::new() },
        ticket_challenge: if has_ticket { "challenge".to_string() } else { String::new() },
        ticket: ticket.to_string(),
    }
}

// ==================== validate_cli_cmd_infos ====================

#[test]
fn test_validate_empty_list() {
    let result = validate_cli_cmd_infos(&[]);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::ArgEmpty);
}

#[test]
fn test_validate_valid_list() {
    let cmds = vec![make_cmd("ohos-cliTimer", "run")];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_ok());
}

#[test]
fn test_validate_empty_cmd_name() {
    let cmds = vec![make_cmd("", "run")];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArgument);
}

#[test]
fn test_validate_empty_sub_cmd_ok() {
    let cmds = vec![make_cmd("cmd1", "")];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_ok(), "empty subCmd should be allowed");
}

#[test]
fn test_validate_multiple_cmds_one_empty() {
    let cmds = vec![make_cmd("cmd1", "sub1"), make_cmd("", "sub2")];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArgument);
}

#[test]
fn test_validate_exceeds_max_count() {
    let cmds: Vec<CxxCommandInfo> = (0..301).map(|_| make_cmd("c", "s")).collect();
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArrayLen);
}

#[test]
fn test_validate_max_count_boundary() {
    let cmds: Vec<CxxCommandInfo> = (0..300).map(|_| make_cmd("c", "s")).collect();
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_ok(), "exactly 300 should pass");
}

#[test]
fn test_validate_cmd_name_exceeds_max_length() {
    let long_name = "a".repeat(513);
    let cmds = vec![make_cmd(&long_name, "sub")];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArgument);
}

#[test]
fn test_validate_sub_cmd_exceeds_max_length() {
    let long_sub = "a".repeat(513);
    let cmds = vec![make_cmd("cmd", &long_sub)];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArgument);
}

#[test]
fn test_validate_cmd_name_max_length_boundary() {
    let name = "a".repeat(512);
    let sub = "b".repeat(512);
    let cmds = vec![make_cmd(&name, &sub)];
    let result = validate_cli_cmd_infos(&cmds);
    assert!(result.is_ok(), "exactly 512 chars should pass");
}

// ==================== validate_ticket_expire_time_ms ====================

#[test]
fn test_validate_expire_time_zero_ok() {
    let result = validate_ticket_expire_time_ms(0);
    assert!(result.is_ok(), "0 is valid (PermissionManager treats as default)");
}

#[test]
fn test_validate_expire_time_max_ok() {
    let result = validate_ticket_expire_time_ms(60000);
    assert!(result.is_ok(), "60000 is max, should pass");
}

#[test]
fn test_validate_expire_time_exceeds_max() {
    let result = validate_ticket_expire_time_ms(60001);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArgument);
}

// ==================== filter_permission_results ====================

#[test]
fn test_filter_has_ticket_clears_all() {
    let mut results = vec![make_perm("perm1", 2), make_perm("perm2", 2)];
    filter_permission_results(&mut results, true);
    assert!(results.is_empty(), "has_ticket=true should clear all");
}

#[test]
fn test_filter_no_ticket_keeps_unauthorized_only() {
    let mut results = vec![make_perm("perm1", 2), make_perm("perm2", 0), make_perm("perm3", 1)];
    filter_permission_results(&mut results, false);
    assert_eq!(results.len(), 2);
    assert_eq!(results[0].permission, "perm2");
    assert_eq!(results[1].permission, "perm3");
}

#[test]
fn test_filter_no_ticket_all_authorized() {
    let mut results = vec![make_perm("perm1", 2), make_perm("perm2", 2)];
    filter_permission_results(&mut results, false);
    assert!(results.is_empty(), "all authorized + no ticket -> empty list");
}

#[test]
fn test_filter_no_ticket_all_unauthorized() {
    let mut results = vec![make_perm("perm1", 0), make_perm("perm2", 1)];
    filter_permission_results(&mut results, false);
    assert_eq!(results.len(), 2, "all unauthorized -> keep all");
}

#[test]
fn test_filter_restricted_status() {
    let mut results = vec![make_perm("perm1", 2), make_perm("perm2", 3), make_perm("perm3", 4)];
    filter_permission_results(&mut results, false);
    assert_eq!(results.len(), 2);
    assert_eq!(results[0].permission, "perm2");
    assert_eq!(results[1].permission, "perm3");
}

// ==================== do_request_tool_ticket (Mock driven) ====================

#[test]
fn test_do_request_permission_denied() {
    reset_mocks();
    set_mock_permission(false);
    let cmds = vec![make_cmd("cmd1", "sub1")];
    let result = do_request_tool_ticket(cmds, 0);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::PermissionDenied);
}

#[test]
fn test_do_request_empty_cmds() {
    reset_mocks();
    set_mock_permission(true);
    let result = do_request_tool_ticket(vec![], 0);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::ArgEmpty);
}

#[test]
fn test_do_request_expire_time_exceeds() {
    reset_mocks();
    set_mock_permission(true);
    let cmds = vec![make_cmd("cmd1", "sub1")];
    let result = do_request_tool_ticket(cmds, 60001);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err().code, ErrCode::InvalidArgument);
}

#[test]
fn test_do_request_cxx_bridge_failure() {
    reset_mocks();
    set_mock_permission(true);
    set_mock_cxx_result(0x3001C, make_result(vec![], false, ""));
    let cmds = vec![make_cmd("cmd1", "sub1")];
    let result = do_request_tool_ticket(cmds, 0);
    assert!(result.is_err());
}

#[test]
fn test_do_request_success_with_ticket() {
    reset_mocks();
    set_mock_permission(true);
    set_mock_cxx_result(0, make_result(vec![make_perm("perm1", 2), make_perm("perm2", 2)], true, "ticket_str"));
    let cmds = vec![make_cmd("cmd1", "sub1")];
    let result = do_request_tool_ticket(cmds, 0);
    assert!(result.is_ok());
    let r = result.unwrap();
    assert!(r.has_ticket);
    assert_eq!(r.ticket, "ticket_str");
    assert!(r.permission_results.is_empty(), "has_ticket -> clear results");
}

#[test]
fn test_do_request_success_partial_authorized() {
    reset_mocks();
    set_mock_permission(true);
    set_mock_cxx_result(
        0,
        make_result(vec![make_perm("perm1", 2), make_perm("perm2", 0), make_perm("perm3", 1)], false, ""),
    );
    let cmds = vec![make_cmd("cmd1", "sub1")];
    let result = do_request_tool_ticket(cmds, 0);
    assert!(result.is_ok());
    let r = result.unwrap();
    assert!(!r.has_ticket);
    assert!(r.ticket.is_empty());
    assert_eq!(r.permission_results.len(), 2, "only unauthorized kept");
    assert_eq!(r.permission_results[0].permission, "perm2");
    assert_eq!(r.permission_results[1].permission, "perm3");
}

#[test]
fn test_do_request_success_all_unauthorized() {
    reset_mocks();
    set_mock_permission(true);
    set_mock_cxx_result(0, make_result(vec![make_perm("perm1", 0), make_perm("perm2", 1)], false, ""));
    let cmds = vec![make_cmd("cmd1", "sub1")];
    let result = do_request_tool_ticket(cmds, 0);
    assert!(result.is_ok());
    let r = result.unwrap();
    assert!(!r.has_ticket);
    assert_eq!(r.permission_results.len(), 2, "all unauthorized kept");
}
