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

//! This module implements the Wrapper of the SAF service.

use crate::ticket_operation;
use ipc::parcel::MsgParcel;
use saf_definition::{macros_lib, ErrCode};
use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;
use saf_plugin_interface::plugin_interface::{
    EventType, ExtMap, ERROR_METRICS_KEYS, GET_DEVICE_UDID_KEYS, PERFORMANCE_METRICS_KEYS, POLICY_AUTH_STATUS_KEYS,
    VERIFY_REMOTE_TICKET_KEYS, ATL_PERMISSION_POLICY_KEYS,
};
use saf_sdk::Value;

#[cxx::bridge(namespace = "OHOS::Security::SAF")]
pub mod ffi {
    /// C++ compatible VerifyTicketInfo for bridge.
    pub struct CxxVerifyTicketInfo {
        /// Message string.
        pub message: String,
        /// Challenge string.
        pub challenge: String,
        /// Ticket string.
        pub ticket: String,
    }

    /// CLI command info for cxx bridge (input).
    pub struct CxxCommandInfo {
        pub cmd_name: String,
        pub sub_cmd: String,
    }

    /// Permission info for cxx bridge (output).
    pub struct CxxPermissionInfo {
        pub permission: String,
        pub permission_status: i32,
        pub auth_status: i32,
        pub flag: i32,
    }

    /// Ticket query result for cxx bridge (output).
    pub struct CxxTicketQueryResult {
        pub permission_results: Vec<CxxPermissionInfo>,
        pub has_ticket: bool,
        pub ticket_message: String,
        pub ticket_challenge: String,
        pub ticket: String,
    }

    /// CommandPermissionInfo for cxx bridge (output).
    pub struct CxxCommandPermissionInfo {
        pub cmd_name: String,
        pub sub_cmd: String,
        pub permissions: Vec<String>,
        pub query_ret: i32,
    }

    /// IAM auth token verification result.
    pub struct IamVerifyResult {
        pub result_code: i32,
        pub challenge: Vec<u8>,
        pub auth_trust_level: u32,
    }

    // C++ callable Rust functions
    extern "Rust" {
        /// Notify performance metrics.
        fn notify_performance_metrics(item_count: i32, elapsed_time: i32, os_account_id: i32, function_name: String);
        /// Notify error.
        fn notify_error(error_message: String, error_code: i32, os_account_id: i32, function_name: String);
        /// CXX bridge for batch_generate_ticket.
        fn cxx_batch_generate_ticket(
            os_account_id: i32,
            caller_id: &str,
            domain_id: &str,
            messages: &[String],
            result_code: &mut i32,
        ) -> Vec<CxxVerifyTicketInfo>;
        /// Get policy auth status.
        fn get_policy_auth_status(
            permissions: &Vec<String>,
            auth_statuses: &mut Vec<i32>,
            is_remote: bool,
            is_public_cli: bool,
        ) -> i32;
        /// Query one ATL's lock-screen policy, distinguishing absent and empty lists.
        fn get_atl_permission_policy(
            atl: i32, policy_found: &mut bool, permission_policy: &mut Vec<String>,
        ) -> i32;
        fn verify_remote_ticket(domain_id: String, remote_control_ticket: String, os_account_id: i32) -> i32;
        fn cxx_store_challenge(caller_token_id: &str, challenge: &str, expire_time_ms: u64) -> i32;
        fn cxx_store_controlled_grant_record(
            os_account_id: i32,
            controlled_device_name: String,
            controller_device_name: String,
            permission_names: Vec<String>,
            caller_token_id: i32,
        ) -> i32;
        /// CXX bridge for task identity verification (memory only, used by RequestToolPermissions).
        fn cxx_verify_task_identity(
            os_account_id: i32,
            caller_token_id: &str,
            task_attributes: &str,
            identity: &str,
            challenge: &str,
        ) -> i32;
    }

    // Rust callable C++ functions
    unsafe extern "C++" {
        include!("secure_access_fence_ipc.h");
        include!("message_option.h");
        include!("secure_access_fence_service.h");
        include!("cxx_bridge_functions.h");
        include!("iam_verify_wrapper.h");
        #[namespace = "OHOS"]
        type MessageParcel = ipc::parcel::MessageParcel;
        /// On remote request.
        fn OnRemoteRequest(code: u32, data: Pin<&mut MessageParcel>, reply: Pin<&mut MessageParcel>) -> i32;
        fn GetBootTimeMs() -> i64;
        /// Call C++ PermissionManager::RequestToolPermissions via bridge.
        fn CxxRequestToolTicket(
            cli_cmd_infos: &Vec<CxxCommandInfo>,
            ticket_expire_time_ms: u64,
            result: &mut CxxTicketQueryResult,
        ) -> i32;
        /// Call C++ PermissionManager::QueryStaticCliCmdPermissions via bridge.
        fn CxxQueryStaticCliCmdPermissions(
            cmds: &Vec<CxxCommandInfo>,
            caller_token_id: u32,
            result: &mut Vec<CxxCommandPermissionInfo>,
        ) -> i32;
        /// Get the binder calling token id.
        fn GetCallingTokenId() -> u32;
        /// Resolve os_account_id from a token id (SA→0, HAP→osAccountID).
        fn GetOsAccountIdFromTokenId(tokenId: u32) -> i32;
        /// Verify auth token via IAM, returns challenge + authTrustLevel.
        fn VerifyAuthToken(authToken: &Vec<u8>, allowableDurationMs: u64) -> IamVerifyResult;
    }
}

/// IPC request handler for C++ redirect.
pub fn on_remote_request(code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
    let data_pin = data.as_msg_parcel_mut();
    let reply_pin = reply.as_msg_parcel_mut();
    ffi::OnRemoteRequest(code, data_pin, reply_pin)
}

/// Performance metrics notification function.
pub fn notify_performance_metrics(item_count: i32, elapsed_time: i32, os_account_id: i32, function_name: String) {
    logi!(
        "[INFO] Performance metrics from C++/Rust: function={}, item_count={}, elapsed_time={}ms",
        function_name,
        item_count,
        elapsed_time
    );

    if let Err(e) = call_plugin_performance_event(item_count, elapsed_time, os_account_id, function_name) {
        loge!("[ERROR] Failed to call plugin process_event for performance metrics: {}", e);
    }
}

/// Error notification function.
pub fn notify_error(error_message: String, error_code: i32, os_account_id: i32, function_name: String) {
    logi!(
        "[INFO] Error metrics from C++: function={}, error_code={}, os_account_id={}, error_message={}",
        function_name,
        error_code,
        os_account_id,
        error_message
    );

    if let Err(e) = call_plugin_error_event(error_message, error_code, os_account_id, function_name) {
        loge!("[ERROR] Failed to call plugin process_event for error metrics: {}", e);
    }
}

pub fn get_policy_auth_status(
    permissions: &Vec<String>,
    auth_statuses: &mut Vec<i32>,
    is_remote: bool,
    is_public_cli: bool,
) -> i32 {
    logi!(
        "[get_policy_auth_status] permissions_len={}, is_remote={}, is_public_cli={}",
        permissions.len(),
        is_remote,
        is_public_cli
    );

    let plugin = SAFPlugin::get_instance();
    let loader = match plugin.load_plugin() {
        Ok(loader) => loader,
        Err(_e) => {
            return saf_definition::ErrCode::PluginNotSupport as i32;
        },
    };

    let mut params = ExtMap::new();
    params.insert(POLICY_AUTH_STATUS_KEYS.permissions, Value::StringList(permissions.clone()));
    params.insert(POLICY_AUTH_STATUS_KEYS.is_remote, Value::Bool(is_remote));
    params.insert(POLICY_AUTH_STATUS_KEYS.is_public_cli, Value::Bool(is_public_cli));

    logi!("[get_policy_auth_status] calling plugin process_event");

    let result = match loader.process_event(EventType::GetPolicyAuthStatus, &mut params) {
        Ok(r) => r,
        Err(e) => {
            loge!("[get_policy_auth_status] process_event failed, e={}", e);
            return e as i32;
        },
    };

    logi!("[get_policy_auth_status] plugin returned, parsing results");

    if let Some(Value::NumberList(statuses)) = result.get(POLICY_AUTH_STATUS_KEYS.auth_statuses) {
        logi!("[get_policy_auth_status] auth_statuses_len={}", statuses.len());
        auth_statuses.extend(statuses.iter().cloned());
    } else {
        loge!("[get_policy_auth_status] auth_statuses not found in result params");
        return saf_definition::ErrCode::HashMapKeyNotFound as i32;
    }

    logi!("[get_policy_auth_status] success, auth_statuses_len={}", auth_statuses.len());

    0
}

pub fn get_atl_permission_policy(
    atl: i32,
    policy_found: &mut bool,
    permission_policy: &mut Vec<String>,
) -> i32 {
    // Clear out-parameters before any failure; never reuse stale policy data.
    *policy_found = false;
    permission_policy.clear();
    if atl <= 0 {
        return saf_definition::ErrCode::InvalidArgument as i32;
    }
    let plugin = SAFPlugin::get_instance();
    let loader = match plugin.load_plugin() {
        Ok(loader) => loader,
        Err(_) => return saf_definition::ErrCode::PluginNotSupport as i32,
    };
    let mut params = ExtMap::new();
    params.insert(ATL_PERMISSION_POLICY_KEYS.atl, Value::Number(atl as u32));
    let result = match loader.process_event(EventType::GetAtlPermissionPolicy, &mut params) {
        Ok(result) => result,
        Err(e) => {
            loge!("[get_atl_permission_policy] process_event failed, err={}", e);
            return e as i32;
        },
    };
    let found = match result.get(ATL_PERMISSION_POLICY_KEYS.policy_found) {
        Some(Value::Bool(flag)) => *flag,
        _ => {
            loge!("[get_atl_permission_policy] invalid or missing PolicyFound");
            return saf_definition::ErrCode::InvalidArgument as i32;
        },
    };
    match result.get(ATL_PERMISSION_POLICY_KEYS.permission_policy) {
        Some(Value::StringList(permissions)) if found => {
            if permissions.iter().any(|permission| permission.is_empty()) {
                return saf_definition::ErrCode::InvalidArgument as i32;
            }
            permission_policy.extend(permissions.iter().cloned());
        },
        Some(Value::StringList(permissions)) if permissions.is_empty() => {},
        None if !found => {},
        _ => {
            loge!("[get_atl_permission_policy] invalid or missing PermissionPolicy");
            return saf_definition::ErrCode::InvalidArgument as i32;
        },
    }
    // Set only after validating the complete response.
    *policy_found = found;
    0
}

pub fn verify_remote_ticket(domain_id: String, remote_control_ticket: String, os_account_id: i32) -> i32 {
    let plugin = SAFPlugin::get_instance();
    let loader = match plugin.load_plugin() {
        Ok(loader) => loader,
        Err(_e) => {
            return saf_definition::ErrCode::PluginNotSupport as i32;
        },
    };
    let mut params = ExtMap::new();
    params.insert(VERIFY_REMOTE_TICKET_KEYS.domain_id, Value::String(domain_id));
    params.insert(VERIFY_REMOTE_TICKET_KEYS.remote_control_ticket, Value::String(remote_control_ticket));
    params.insert(VERIFY_REMOTE_TICKET_KEYS.os_account_id, Value::Number(os_account_id as u32));

    logi!("[verify_remote_ticket] calling plugin process_event");

    match loader.process_event(EventType::VerifyRemoteTicket, &mut params) {
        Ok(_r) => 0,
        Err(e) => {
            loge!("[verify_remote_ticket] process_event failed, e={}", e);
            e as i32
        },
    }
}

/// C++ -> Rust bridge for batch_generate_ticket.
pub fn cxx_batch_generate_ticket(
    os_account_id: i32,
    caller_id: &str,
    domain_id: &str,
    messages: &[String],
    result_code: &mut i32,
) -> Vec<ffi::CxxVerifyTicketInfo> {
    logi!(
        "[Wrapper cxx_batch_generate_ticket] os_account_id = {}, caller_id = {}, messages_count = {}",
        os_account_id,
        caller_id,
        messages.len()
    );
    match ticket_operation::batch_generate_ticket(os_account_id, caller_id, domain_id, messages) {
        Ok(v) => {
            *result_code = 0;
            v.into_iter()
                .map(|r| ffi::CxxVerifyTicketInfo { message: r.message, challenge: r.challenge, ticket: r.ticket })
                .collect()
        },
        Err(e) => {
            notify_error(
                format!("batch_generate_ticket failed: {}", e.code),
                e.code as i32,
                os_account_id,
                "cxx_batch_generate_ticket".to_string(),
            );
            *result_code = e.code as i32;
            Vec::new()
        },
    }
}

// C++ -> Rust bridge for store challenge.
pub fn cxx_store_challenge(caller_token_id: &str, challenge: &str, expire_time_ms: u64) -> i32 {
    logi!(
        "[Wrapper cxx_store_challenge] caller = {}, challenge_len = {}, expire_time_ms = {}",
        caller_token_id,
        challenge.len(),
        expire_time_ms
    );
    match ticket_operation::challenge_store::global_challenge_store().insert(caller_token_id, challenge, expire_time_ms)
    {
        Ok(()) => 0,
        Err(e) => {
            loge!("[Wrapper cxx_store_challenge] insert failed, err = {}", e.code);
            e.code as i32
        },
    }
}

/// C++ -> Rust bridge for store_controlled_grant_record.
pub fn cxx_store_controlled_grant_record(
    os_account_id: i32,
    controlled_device_name: String,
    controller_device_name: String,
    permission_names: Vec<String>,
    caller_token_id: i32,
) -> i32 {
    logi!(
        "[Wrapper cxx_store_controlled_grant_record] os_account_id={}, caller_token_id={}, permission_count={}",
        os_account_id,
        caller_token_id,
        permission_names.len()
    );

    let calling_bundle_name = match crate::remote_control::get_bundle_name_from_token(caller_token_id) {
        Ok(name) => name,
        Err(e) => {
            loge!("[cxx_store_controlled_grant_record] Failed to get bundle name: {:?}", e);
            return e.code as i32;
        },
    };

    let params = crate::remote_control::StoreGrantRecordParams {
        os_account_id,
        controlled_device_name,
        controller_device_name,
        is_self_grant: false,
        permission_names,
        device_role: saf_definition::Role::Controlled,
        calling_bundle_name,
        grant_type: saf_definition::GrantType::RemoteGrant,
        timestamp: match saf_utils::system_time_in_millis() {
            Ok(t) => t,
            Err(e) => {
                loge!("[cxx_store_controlled_grant_record] Failed to get timestamp: {:?}", e);
                return e.code as i32;
            },
        },
    };

    match crate::remote_control::store_grant_record(params) {
        Ok(()) => {
            logi!("[cxx_store_controlled_grant_record] success");
            0
        },
        Err(e) => {
            loge!("[cxx_store_controlled_grant_record] Failed to store grant record: {:?}", e);
            e.code as i32
        },
    }
}

/// Get the binder calling token id as a string.
pub fn get_calling_token_id() -> String {
    ffi::GetCallingTokenId().to_string()
}

/// C++ -> Rust bridge for task identity verification (memory first,
/// disk fallback for long-duration tasks after the SA restarts).
pub fn cxx_verify_task_identity(
    os_account_id: i32,
    caller_token_id: &str,
    task_attributes: &str,
    identity: &str,
    challenge: &str,
) -> i32 {
    logi!(
        "[Wrapper cxx_verify_task_identity] os_account_id = {}, caller = {}, task_attributes_len = {}",
        os_account_id, caller_token_id.len(), task_attributes.len()
    );
    match crate::task_identity::verify_task_identity(
        os_account_id,
        caller_token_id,
        task_attributes,
        identity,
        challenge,
    ) {
        Ok(_) => 0,
        Err(e) => {
            loge!("[Wrapper cxx_verify_task_identity] verify failed, err = {}", e.code);
            e.code as i32
        }
    }
}

/// Get Device UDID via plugin process_event
pub fn get_device_udid(os_account_id: i32) -> saf_definition::Result<String> {
    if os_account_id < 0 {
        loge!("[Wrapper get_device_udid] os_account_id is negative: {}", os_account_id);
        return macros_lib::log_throw_error!(
            ErrCode::InvalidOsAccountId,
            "os_account_id is negative: {}",
            os_account_id
        );
    }

    let plugin = SAFPlugin::get_instance();
    let loader = match plugin.load_plugin() {
        Ok(loader) => loader,
        Err(_e) => {
            loge!("[Wrapper get_device_udid] load_plugin failed");
            return macros_lib::log_throw_error!(ErrCode::PluginNotSupport, "load plugin failed");
        },
    };

    let mut params = ExtMap::new();
    params.insert(GET_DEVICE_UDID_KEYS.os_account_id, Value::Number(os_account_id as u32));

    let result = match loader.process_event(EventType::GetDeviceUdid, &mut params) {
        Ok(r) => r,
        Err(e) => {
            loge!("[Wrapper get_device_udid] process_event failed, e={}", e);
            return macros_lib::log_throw_error!(ErrCode::GetUdidFailed, "process_event failed, e={}", e);
        },
    };

    let Some(Value::String(udid)) = result.get(GET_DEVICE_UDID_KEYS.udid) else {
        loge!("[Wrapper get_device_udid] udid not found in result params");
        return macros_lib::log_throw_error!(ErrCode::HashMapKeyNotFound, "udid not found in result params");
    };

    logi!("[Wrapper get_device_udid] success, udid_len={}", udid.len());
    Ok(udid.clone())
}

// Plugin performance event helper
fn call_plugin_performance_event(
    item_count: i32,
    elapsed_time: i32,
    os_account_id: i32,
    function_name: String,
) -> Result<(), String> {
    let plugin = SAFPlugin::get_instance();

    let loader = plugin.load_plugin().map_err(|e| format!("load_plugin failed: {}", e))?;

    let mut params = ExtMap::new();
    params.insert(PERFORMANCE_METRICS_KEYS.item_count, Value::Number(item_count as u32));
    params.insert(PERFORMANCE_METRICS_KEYS.elapsed_time, Value::Number(elapsed_time as u32));
    params.insert(PERFORMANCE_METRICS_KEYS.os_account_id, Value::Number(os_account_id as u32));
    params.insert(PERFORMANCE_METRICS_KEYS.function_name, Value::String(function_name));

    let _ = loader.process_event(EventType::StatisticsMetrics, &mut params);
    Ok(())
}

fn call_plugin_error_event(
    error_message: String,
    error_code: i32,
    os_account_id: i32,
    function_name: String,
) -> Result<(), String> {
    let plugin = SAFPlugin::get_instance();

    let loader = plugin.load_plugin().map_err(|e| format!("load_plugin failed: {}", e))?;

    let mut params = ExtMap::new();
    params.insert(ERROR_METRICS_KEYS.error_message, Value::String(error_message));
    params.insert(ERROR_METRICS_KEYS.error_code, Value::Number(error_code as u32));
    params.insert(ERROR_METRICS_KEYS.os_account_id, Value::Number(os_account_id as u32));
    params.insert(ERROR_METRICS_KEYS.error_function, Value::String(function_name));

    let _ = loader.process_event(EventType::StatisticsError, &mut params);
    Ok(())
}

extern "C" {
    fn IsScreenLocked(os_account_id: i32, isLocked: *mut bool) -> i32;
}

pub fn cxx_is_screen_locked(os_account_id: i32, is_locked: &mut bool) -> i32 {
    unsafe { IsScreenLocked(os_account_id, is_locked) }
}
