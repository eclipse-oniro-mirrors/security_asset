// SPDX-License-Identifier: Apache-2.0
/// Test stub exports for request_tool_ticket
/// All types and functions are exported from here (wrapper is private module)

#[cfg(feature = "SAFTest")]
pub use crate::wrapper::ffi::{CxxCommandInfo, CxxPermissionInfo, CxxTicketQueryResult};

#[cfg(feature = "SAFTest")]
pub use crate::{
    do_request_tool_ticket, filter_permission_results, validate_cli_cmd_infos, validate_ticket_expire_time_ms,
};

#[cfg(feature = "SAFTest")]
pub use crate::test_mocks::{reset_mocks, set_mock_cxx_result, set_mock_permission};
