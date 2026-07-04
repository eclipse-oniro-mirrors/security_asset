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
use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;
use saf_plugin_interface::plugin_interface::{
    EventType, ExtMap, ERROR_METRICS_KEYS, PERFORMANCE_METRICS_KEYS, POLICY_AUTH_STATUS_KEYS, VERIFY_REMOTE_TICKET_KEYS,
};
use saf_sdk::Value;

#[cxx::bridge(namespace = "OHOS::Security::SAF")]
pub mod ffi {
    // C++ compatible VerifyTicketInfo for bridge
    pub struct CxxVerifyTicketInfo {
        pub message: String,
        pub challenge: String,
        pub ticket: String,
    }

    // C++ callable Rust functions
    extern "Rust" {
        fn notify_performance_metrics(item_count: i32, elapsed_time: i32, os_account_id: i32, function_name: String);
        fn notify_error(error_message: String, error_code: i32, os_account_id: i32, function_name: String);
        fn cxx_batch_generate_ticket(os_account_id: i32, caller_id: &str, domain_id: &str, messages: &[String], result_code: &mut i32) -> Vec<CxxVerifyTicketInfo>;
        fn get_policy_auth_status(permissions: &Vec<String>, auth_statuses: &mut Vec<i32>) -> i32;
        fn verify_remote_ticket(domain_id: String, remote_control_ticket: String) -> i32;
    }

    // Rust callable C++ functions
    unsafe extern "C++" {
        include!("secure_access_fence_ipc.h");
        include!("message_option.h");
        #[namespace = "OHOS"]
        type MessageParcel = ipc::parcel::MessageParcel;
        fn OnRemoteRequest(code: u32, data: Pin<&mut MessageParcel>, reply: Pin<&mut MessageParcel>) -> i32;
    }
}

// Original function: IPC request handler
pub fn on_remote_request(code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
    let data_pin = data.as_msg_parcel_mut();
    let reply_pin = reply.as_msg_parcel_mut();
    ffi::OnRemoteRequest(code, data_pin, reply_pin)
}

// Performance metrics notification function
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

// Error notification function
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

#[allow(dead_code)]
pub fn get_policy_auth_status(permissions: &Vec<String>, auth_statuses: &mut Vec<i32>) -> i32 {
    logi!("[get_policy_auth_status] permissions_len={}", permissions.len());

    let plugin = SAFPlugin::get_instance();
    let loader = match plugin.load_plugin() {
        Ok(loader) => loader,
        Err(_e) => {
            return saf_definition::ErrCode::PluginNotSupport as i32;
        },
    };

    let mut params = ExtMap::new();
    params.insert(POLICY_AUTH_STATUS_KEYS.permissions, Value::StringList(permissions.clone()));

    logi!("[get_policy_auth_status] calling plugin process_event");

    let result = match loader.process_event(EventType::GetPolicyAuthStatus, &mut params) {
        Ok(r) => r,
        Err(e) => {
            loge!("[get_policy_auth_status] process_event failed, e={}", e);
            return e as i32;
        },
    };

    logi!("[get_policy_auth_status] plugin returned, parsing results");

    let Some(Value::NumberList(statuses)) = result.get(POLICY_AUTH_STATUS_KEYS.auth_statuses) else {
        loge!("[get_policy_auth_status] auth_statuses not found in result params");
        return saf_definition::ErrCode::HashMapKeyNotFound as i32;
    };

    *auth_statuses = statuses.clone();

    logi!("[get_policy_auth_status] success, auth statuses_len={}", auth_statuses.len());

    0
}

pub fn verify_remote_ticket(domain_id: String, remote_control_ticket: String) -> i32 {
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

    logi!("[verify_remote_ticket] calling plugin process_event");

    match loader.process_event(EventType::VerifyRemoteTicket, &mut params) {
        Ok(_r) => 0,
        Err(e) => {
            loge!("[verify_remote_ticket] process_event failed, e={}", e);
            e as i32
        },
    }
}

// C++ -> Rust bridge for batch_generate_ticket. Returns empty vector on error and reports via notify_error.
pub fn cxx_batch_generate_ticket(os_account_id: i32, caller_id: &str, domain_id: &str, messages: &[String], result_code: &mut i32) -> Vec<ffi::CxxVerifyTicketInfo> {
    logi!("[Wrapper cxx_batch_generate_ticket] os_account_id = {}, caller_id = {}, messages_count = {}",
        os_account_id, caller_id, messages.len());
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

// Plugin error event helper
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
    fn IsScreenLocked(isLocked: *mut bool) -> i32;
}

pub fn cxx_is_screen_locked(is_locked: &mut bool) -> i32 {
    unsafe { IsScreenLocked(is_locked) }
}
