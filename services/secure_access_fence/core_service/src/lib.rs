/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

//! This module implements secure access fence service.

use std::ffi::{c_char, CString};
use std::time::Instant;
use std::os::raw::c_char as raw_c_char;
use ipc::parcel::MsgParcel;
use samgr::manage::SystemAbilityManager;
use std::time::Duration;
use system_ability_fwk::{
    ability::{Ability, Handler},
    cxx_share::SystemAbilityOnDemandReason,
};
use ylong_runtime::builder::RuntimeBuilder;

use saf_common::{Counter, TaskManager};
use saf_definition::{macros_lib, ErrCode, Result};
use saf_ipc::{SA_ID, CliInfo, VerifyTicketInfo};
use saf_log::{logd, loge, logi};
use saf_plugin::saf_plugin::{SAFContext, SAFPlugin};

use crate::wrapper::{notify_performance_metrics, notify_error};

mod common_event;
mod stub;
mod wrapper;
mod ticket_operation;

#[macro_use]
mod metrics_macro;

const GET_TICKET_INFO_PERMISSION: &str = "ohos.permission.GET_TICKET_INFO";

const JSON_KEY_MESSAGE: &str = "message";
const JSON_KEY_CHALLENGE: &str = "challenge";
const JSON_KEY_TICKET: &str = "ticket";
const JSON_KEY_CALLER_TOKEN_ID: &str = "callerTokenId";
const JSON_KEY_CLI_INFOS: &str = "cliInfos";
const JSON_KEY_CLI_CMD_NAME: &str = "cliCmdName";
const JSON_KEY_SUB_CLI_CMD_NAME: &str = "subCliCmdName";
const JSON_KEY_PERMISSION_LIST: &str = "permissionList";
const JSON_KEY_START_TIME: &str = "startTime";
const JSON_KEY_TICKET_EXPIRE_TIME_MS: &str = "ticketExpireTimeMs";
const METRIC_ITEM_COUNT_TICKET_VERIFY: i32 = 1;
const DEFAULT_DOMAIN_ID: &str = "";
const STRING_QUOTE: char = '"';
extern "C" {
    fn CheckPermission(permission: *const raw_c_char) -> bool;
}

struct SAFAbility;

#[repr(C)]
struct StringArray {
    size: u32,
    data: *const *const c_char,
}


#[repr(C)]
struct CommonEventInfoFfi {
    event_type: *const c_char,
    want: StringArray,
}

const DELAYED_UNLOAD_TIME_IN_SEC: i32 = 60;  // 60s
const SEC_TO_MILLISEC: i32 = 1000;

pub(crate) fn unload_sa() {
    ylong_runtime::spawn(async move {
        loop {
            ylong_runtime::time::sleep(Duration::from_secs(DELAYED_UNLOAD_TIME_IN_SEC as u64)).await;
            let counter = Counter::get_instance();
            if counter.lock().unwrap().count() > 0 {
                continue;
            }

            if let Ok(load) = SAFPlugin::get_instance().load_plugin() {
                if load.get_working_request_num() > 0 {
                    continue;
                }
            }

            let task_manager = TaskManager::get_instance();
            if !task_manager.lock().unwrap().is_empty() {
                continue;
            }

            SystemAbilityManager::unload_system_ability(SA_ID);
            break;
        }
    });
}

impl Ability for SAFAbility {
    fn on_start_with_reason(&self, reason: SystemAbilityOnDemandReason, handler: Handler) {
        logi!("Start SAF SA service, reason_id: {:?}", reason.reason_id);
        if let Err(e) = RuntimeBuilder::new_multi_thread().worker_num(1).max_blocking_pool_size(1).build_global() {
            loge!("[WARNING]Ylong new global thread failed! {}", e);
        };
        let _ = start_service(handler);
        common_event::handle_common_event(reason);
    }

    fn on_active(&self, reason: SystemAbilityOnDemandReason) {
        common_event::handle_common_event(reason);
    }

    fn on_idle(&self, reason: SystemAbilityOnDemandReason) -> i32 {
        let counter = Counter::get_instance();
        if counter.lock().unwrap().count() > 0 {
            logi!(
                "SAF service on idle not success for use_account: {}, delay time: {}s",
                counter.lock().unwrap().count(),
                DELAYED_UNLOAD_TIME_IN_SEC
            );
            return DELAYED_UNLOAD_TIME_IN_SEC * SEC_TO_MILLISEC;
        }
        match SAFPlugin::get_instance().load_plugin() {
            Ok(loader) => {
                let delay_time = loader.on_idle();
                if delay_time == 0 {
                    logd!("Saf service idle, reason: {}", reason.name);
                }
                delay_time
            },
            Err(_) => {
                loge!("load plugin failed.");
                0
            },
        }
    }

    fn on_stop(&self) {
        logi!("SAF service on_stop");
        let counter = Counter::get_instance();
        match SAFPlugin::get_instance().load_plugin() {
            Ok(loader) => {
                loader.on_stop();
            },
            Err(_) => loge!("load plugin failed."),
        }
        counter.lock().unwrap().stop();
        common_event::unsubscribe();
    }

    fn on_extension(&self, extension: String, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
        logi!("SAF on_extension, extension is {}", extension);
        if let Ok(load) = SAFPlugin::get_instance().load_plugin() {
            match load.on_sa_extension(extension, data, reply) {
                Ok(()) => logi!("process sa extension event success."),
                Err(code) => loge!("process sa extension event failed, code: {}", code),
            };
        }
        logi!("SAF on_extension end");
        0
    }
}

fn start_service(handler: Handler) -> Result<()> {
    // 加载plugin插件
    match SAFPlugin::get_instance().load_plugin() {
        Ok(loader) => {
            let _tr = loader.init(Box::new(SAFContext {}));
            loader.on_start();
            logi!("load plugin success.");
        },
        Err(_) => loge!("load plugin failed."),
    }

    if !handler.publish(SAFService::new(handler.clone())) {
        return macros_lib::log_throw_error!(ErrCode::IpcError, "SAF publish stub object failed");
    };

    common_event::subscribe();
    Ok(())
}

#[used]
#[link_section = ".init_array"]
static A: extern "C" fn() = {
    extern "C" fn init() {
        let Some(sa) = SAFAbility.build_system_ability(SA_ID, true) else {
            loge!("Create SAF service failed.");
            return;
        };

        sa.register();
    }
    init
};

struct SAFService {
    system_ability: system_ability_fwk::ability::Handler,
}

impl SAFService {
    pub(crate) fn new(handler: system_ability_fwk::ability::Handler) -> Self {
        Self { system_ability: handler }
    }

    fn batch_generate_ticket(&self, os_account_id: i32, caller_id: &str, domain_id: &str, messages: &[String]) ->
        Result<Vec<VerifyTicketInfo>> {
        let permission = CString::new(GET_TICKET_INFO_PERMISSION).unwrap();
        if unsafe { !CheckPermission(permission.as_ptr()) } {
            loge!("Permission denied! Need {}", GET_TICKET_INFO_PERMISSION);
            
            notify_error(
                "Permission denied".to_string(),
                ErrCode::PermissionDenied as i32,
                os_account_id,
                "batch_generate_ticket".to_string()
            );

            return macros_lib::log_throw_error!(ErrCode::PermissionDenied,
                "Permission denied! Need {}", GET_TICKET_INFO_PERMISSION);
        }

        execute_with_metrics!(
            "batch_generate_ticket",
            ticket_operation::batch_generate_ticket,
            messages.len() as i32,
            os_account_id,
            caller_id,
            domain_id,
            messages
        )
    }

    fn batch_verify_ticket(
        &self, os_account_id: i32, caller_id: &str, domain_id: &str, verify_infos: &[VerifyTicketInfo]) ->
        Result<Vec<i32>> {
        execute_with_metrics!(
            "batch_verify_ticket",
            ticket_operation::batch_verify_ticket,
            verify_infos.len() as i32,
            os_account_id,
            caller_id,
            verify_infos
        )
    }

    fn verify_ticket(
        &self, os_account_id: i32, caller_id: &str, verify_info_str: &str
    ) -> Result<Vec<CliInfo>> {
        execute_with_metrics!(
            "verify_ticket",
            verify_ticket_impl,
            METRIC_ITEM_COUNT_TICKET_VERIFY,
            os_account_id,
            caller_id,
            verify_info_str
        )
    }
}

fn verify_ticket_impl(os_account_id: i32, caller_id: &str, verify_info_str: &str) -> Result<Vec<CliInfo>> {
    let (ticket_info, message_json) = parse_verify_info_json_full(verify_info_str)?;
    verify_single_ticket(os_account_id, caller_id, ticket_info)?;
    check_ticket_time_validity_with_json(&message_json)?;
    extract_cli_infos_with_json(&message_json)
}

fn verify_single_ticket(os_account_id: i32, caller_id: &str, ticket_info: VerifyTicketInfo) -> Result<()> {
    let verify_res = ticket_operation::batch_verify_ticket(
        os_account_id, caller_id, DEFAULT_DOMAIN_ID, &[ticket_info]
    )?;

    if verify_res.is_empty() {
        macros_lib::log_throw_error!(ErrCode::GeneralError, "VerifyTicket: verify result is empty")
    } else if verify_res[0] != 0 {
        let err_code = ErrCode::try_from(verify_res[0] as u32)
            .unwrap_or(ErrCode::GeneralError);
        macros_lib::log_throw_error!(err_code, "ticket verify failed, code={}", verify_res[0])
    } else {
        Ok(())
    }
}

fn check_ticket_time_validity_with_json(message_json: &ylong_json::JsonValue) -> Result<()> {
    if message_json == &ylong_json::JsonValue::Null {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "VerifyTicket: message field missing for time check");
    }
    let start_time = extract_json_u64(message_json, JSON_KEY_START_TIME)?;
    // ticketExpireTimeMs is a duration offset from startTime, not an absolute timestamp
    let expire_duration_time = extract_json_u64(message_json, JSON_KEY_TICKET_EXPIRE_TIME_MS)?;
    let expire_time = start_time.checked_add(expire_duration_time).ok_or_else(|| {
        loge!("VerifyTicket: expire_time overflow");
        macros_lib::SAFError::new(ErrCode::DataTypeMismatch, "expire_time overflow".to_string())
    })?;
    let now_ms = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis() as u64;
    if now_ms < start_time || now_ms > expire_time {
        macros_lib::log_throw_error!(ErrCode::TicketTimeInvalid,
            "VerifyTicket: ticket time invalid, now={}, start={}, expire={}", now_ms, start_time, expire_time)
    } else {
        Ok(())
    }
}

fn extract_json_u64(json: &ylong_json::JsonValue, key: &str) -> Result<u64> {
    let value = &json[key];
    if value == &ylong_json::JsonValue::Null {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "VerifyTicket: {} field missing", key);
    }
    let s = value.to_compact_string().map_err(|e| {
        loge!("VerifyTicket: {} extract failed: {}", key, e);
        macros_lib::SAFError::new(ErrCode::GeneralError, format!("{} extract failed", key))
    })?;
    s.parse::<u64>().map_err(|e| {
        loge!("VerifyTicket: {} parse u64 failed: {}", key, e);
        macros_lib::SAFError::new(ErrCode::DataTypeMismatch, format!("{} parse u64 failed", key))
    })
}

fn parse_verify_info_json_full(info_str: &str) -> Result<(VerifyTicketInfo, ylong_json::JsonValue)> {
    let json = ylong_json::JsonValue::from_text(info_str).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::ArgEmpty, "VerifyTicket: json parse failed: {}", e)
    })?;
    if json.try_as_object().map_or(true, |o| o.is_empty()) {
        return Err(macros_lib::log_and_into_saf_error!(ErrCode::ArgEmpty, "VerifyTicket: json object is empty"))
    }

    let raw_message = extract_json_string(&json, JSON_KEY_MESSAGE)?;
    let raw_challenge = extract_json_string(&json, JSON_KEY_CHALLENGE)?;
    let raw_ticket = extract_json_string(&json, JSON_KEY_TICKET)?;

    let message_json = ylong_json::JsonValue::from_text(&raw_message).map_err(|e| {
        loge!("VerifyTicket: message json parse failed: {}", e);
        macros_lib::SAFError::new(ErrCode::ArgEmpty, format!("message json parse failed: {}", e))
    })?;

    Ok((VerifyTicketInfo {
        message: raw_message,
        challenge: raw_challenge,
        ticket: raw_ticket,
    }, message_json))
}

fn extract_json_string(json: &ylong_json::JsonValue, key: &str) -> Result<String> {
    let value = &json[key];
    if value == &ylong_json::JsonValue::Null {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "{} field missing", key);
    }
    match value {
        ylong_json::JsonValue::String(s) => Ok(s.clone()),
        _ => value.to_compact_string().map_err(|e| {
            macros_lib::SAFError::new(ErrCode::ArgEmpty, format!("VerifyTicket: {} extract failed: {}", key, e))
        })
    }
}

fn extract_cli_infos_with_json(message_json: &ylong_json::JsonValue) -> Result<Vec<CliInfo>> {
    if message_json == &ylong_json::JsonValue::Null {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty,
            "VerifyTicket: message field missing for cliInfos extraction");
    }

    let caller_token_id = extract_json_string(message_json, JSON_KEY_CALLER_TOKEN_ID)?;

    let cli_infos_array = &message_json[JSON_KEY_CLI_INFOS];
    if cli_infos_array == &ylong_json::JsonValue::Null {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "VerifyTicket: cliInfos not found in message");
    }
    let arr = cli_infos_array.try_as_array().map_err(|e| {
        loge!("VerifyTicket: cliInfos not array: {}", e);
        macros_lib::SAFError::new(ErrCode::ArgEmpty, "cliInfos not array".to_string())
    })?;

    let mut result = Vec::with_capacity(arr.len());
    for item in arr.iter() {
        // ATM不解析这个json， 且ATM需要不带双引号的字符串，在此处帮助ATM删除双引号
        let cli_cmd_name = extract_json_string(item, JSON_KEY_CLI_CMD_NAME)?.trim_matches(STRING_QUOTE).to_string();
        let sub_cli_cmd_name =
            extract_json_string(item, JSON_KEY_SUB_CLI_CMD_NAME)?.trim_matches(STRING_QUOTE).to_string();
        let permission_list = extract_permission_list(item)?;
        result.push(CliInfo {
            caller_token_id: caller_token_id.clone(),
            cli_cmd_name,
            sub_cli_cmd_name,
            permission_list,
        });
    }
    Ok(result)
}

fn extract_permission_list(json: &ylong_json::JsonValue) -> Result<Vec<String>> {
    let perm_array = &json[JSON_KEY_PERMISSION_LIST];
    if perm_array == &ylong_json::JsonValue::Null {
        return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "VerifyTicket: permissionList not found");
    }
    let arr = perm_array.try_as_array().map_err(|e| {
        loge!("VerifyTicket: permissionList not array: {}", e);
        macros_lib::SAFError::new(ErrCode::ArgEmpty, "permissionList not array".to_string())
    })?;
    let mut result = Vec::with_capacity(arr.len());
    for item in arr.iter() {
        let s = item.to_compact_string().map_err(|e| {
            loge!("VerifyTicket: permissionList item not string: {}", e);
            macros_lib::SAFError::new(ErrCode::DataTypeMismatch, "permissionList item not string".to_string())
        })?;
        result.push(s.replace(STRING_QUOTE, ""));
    }
    Ok(result)
}

#[cfg(feature = "SAFTest")]
/// stub for test
pub mod ut_core_service_lib_stub {
    include!{"../../../test/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs"}
}
