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

//! This module implements grant record storage functionality.

use ylong_json::JsonValue;
use saf_definition::{macros_lib, ErrCode, GrantType, Result, Role};
use saf_log::{loge, logi};
use saf_utils::{get_compact_json_value, system_time_in_millis};

#[cfg(not(feature = "SAFTest"))]
use std::collections::HashMap;
#[cfg(not(feature = "SAFTest"))]
use saf_plugin::saf_plugin::SAFPlugin;
#[cfg(not(feature = "SAFTest"))]
use saf_plugin_interface::plugin_interface::{
    EventType, ExtMap, STORE_GRANT_RECORD_KEYS,
};
#[cfg(not(feature = "SAFTest"))]
use saf_definition::Value;

const MAX_BUNDLE_NAME_LEN: usize = 256;

#[cfg(not(feature = "SAFTest"))]
extern "C" {
    fn GetBundleNameFromTokenId(
        tokenId: i32,
        bundleName: *mut std::ffi::c_char,
        len: *mut i32
    ) -> i32;
}

#[cfg(not(feature = "SAFTest"))]
pub fn get_bundle_name_from_token(caller_token_id: i32) -> Result<String> {
    let mut bundle_name_buf = vec![0i8; MAX_BUNDLE_NAME_LEN];
    let mut len = MAX_BUNDLE_NAME_LEN as i32;
    
    let ret = unsafe {
        GetBundleNameFromTokenId(
            caller_token_id,
            bundle_name_buf.as_mut_ptr(),
            &mut len,
        )
    };
    
    if ret != 0 {
        loge!("GetBundleNameFromTokenId failed, ret={}", ret);
        return Err(macros_lib::log_and_into_saf_error!(ErrCode::GeneralError,
            "GetBundleNameFromTokenId failed"));
    }
    
    unsafe {
        Ok(std::ffi::CStr::from_ptr(bundle_name_buf.as_ptr())
            .to_str()
            .map_err(|e| {
                loge!("Invalid UTF-8 in bundle name: {}", e);
                macros_lib::log_and_into_saf_error!(ErrCode::GeneralError, 
                    "Invalid UTF-8 in bundle name")
            })?
            .to_string())
    }
}

/// Store grant record params structure.
#[derive(Debug, Clone)]
pub struct StoreGrantRecordParams {
    /// OS account ID.
    pub os_account_id: i32,
    /// Controlled device name.
    pub controlled_device_name: String,
    /// Controller device name.
    pub controller_device_name: String,
    /// Is self grant flag.
    pub is_self_grant: bool,
    /// Permission names.
    pub permission_names: Vec<String>,
    /// Device role.
    pub device_role: Role,
    /// Calling bundle name.
    pub calling_bundle_name: String,
    /// Grant type.
    pub grant_type: GrantType,
    /// Timestamp.
    pub timestamp: u64,
}

impl StoreGrantRecordParams {
/// Creates a new StoreGrantRecordParams from remote auth package.
    pub fn from_remote_auth_package(
        os_account_id: i32,
        remote_auth_message: &str,
        device_role: Role,
        grant_type: GrantType,
        calling_bundle_name: String,
    ) -> Result<Self> {
        let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
                "parse remote_auth_message failed: {}", e)
        })?;

        let (controlled_device_name, controller_device_name) = Self::parse_device_names(&json)?;
        let permission_names = Self::parse_permission_names(remote_auth_message)?;
        let timestamp = system_time_in_millis()?;

        Ok(Self {
            os_account_id,
            controlled_device_name,
            controller_device_name,
            is_self_grant: false,
            permission_names,
            device_role,
            calling_bundle_name,
            grant_type,
            timestamp,
        })
    }

    fn parse_device_names(json: &JsonValue) -> Result<(String, String)> {
        let controlled_device_name = get_compact_json_value(json, "controlledDeviceName")
            .map_err(|e| {
                loge!("Failed to get controlledDeviceName: {:?}", e);
                macros_lib::log_and_into_saf_error!(ErrCode::ArgEmpty,
                    "controlledDeviceName not found in remote_auth_message")
            })?;
        
        let controller_device_name = get_compact_json_value(json, "controllerDeviceName")
            .map_err(|e| {
                loge!("Failed to get controllerDeviceName: {:?}", e);
                macros_lib::log_and_into_saf_error!(ErrCode::ArgEmpty,
                    "controllerDeviceName not found in remote_auth_message")
            })?;
        
        Ok((controlled_device_name, controller_device_name))
    }
    
    fn parse_permission_names(remote_auth_message: &str) -> Result<Vec<String>> {
        let json = JsonValue::from_text(remote_auth_message).map_err(|e| {
            loge!("Failed to parse JSON: {:?}", e);
            macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError,
                "parse remote_auth_message failed: {}", e)
        })?;
        
        match &json["authResults"] {
            JsonValue::Null => {
                loge!("authResults not found or is null");
                macros_lib::log_throw_error!(ErrCode::ArgEmpty,
                    "authResults not found in remote_auth_message")
            }
            JsonValue::Array(arr) => {
                let permissions: Vec<String> = arr.iter()
                    .filter_map(|item| {
                        match (&item["permission"], &item["authResult"]) {
                            (JsonValue::String(perm), JsonValue::String(result)) 
                                if result.to_uppercase() == "GRANTED" => Some(perm.clone()),
                            _ => None,
                        }
                    })
                    .collect();
                Ok(permissions)
            }
            _ => {
                loge!("authResults is not an array");
                macros_lib::log_throw_error!(ErrCode::DataTypeMismatch,
                    "authResults is not an array")
            }
        }
    }
}

#[cfg(not(feature = "SAFTest"))]
pub fn store_grant_record(params: StoreGrantRecordParams) -> Result<()> {
    logi!("[store_grant_record] os_account_id={}, device_role={:?}, grant_type={:?}",
        params.os_account_id, params.device_role, params.grant_type);
    
    let loader = SAFPlugin::get_instance().load_plugin().map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::PluginNotSupport, "load plugin failed: {}", e)
    })?;
    
    let mut ext_map: ExtMap = HashMap::new();
    let os_account_id_u32 = u32::try_from(params.os_account_id).map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidOsAccountId,
            "os_account_id is negative: {}", params.os_account_id)
    })?;
    
    ext_map.insert(STORE_GRANT_RECORD_KEYS.os_account_id, Value::Number(os_account_id_u32));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.controlled_device_name, Value::String(params.controlled_device_name));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.controller_device_name, Value::String(params.controller_device_name));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.is_self_grant, Value::Bool(params.is_self_grant));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.permission_names, Value::StringList(params.permission_names));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.device_role, Value::Number(params.device_role as u32));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.calling_bundle_name, Value::String(params.calling_bundle_name));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.grant_type, Value::Number(params.grant_type as u32));
    ext_map.insert(STORE_GRANT_RECORD_KEYS.timestamp, Value::String(params.timestamp.to_string()));
    
    let _ = loader.process_event(EventType::StoreGrantRecord, &mut ext_map).map_err(|e| {
        macros_lib::log_and_into_saf_error!(
            ErrCode::try_from(e).unwrap_or(ErrCode::GeneralError),
            "store_grant_record process_event failed")
    })?;
    
    logi!("[store_grant_record] success");
    Ok(())
}

// ======================== SAFTest mock implementations ========================

#[cfg(feature = "SAFTest")]
use std::sync::atomic::{AtomicBool, Ordering};
#[cfg(feature = "SAFTest")]
use lazy_static::lazy_static;

#[cfg(feature = "SAFTest")]
lazy_static! {
    static ref MOCK_BUNDLE_NAME: String = "mock_bundle_name".to_string();
}

#[cfg(feature = "SAFTest")]
fn get_bundle_name_from_token(_caller_token_id: i32) -> Result<String> {
    Ok(MOCK_BUNDLE_NAME.clone())
}

#[cfg(feature = "SAFTest")]
pub fn store_grant_record(_params: StoreGrantRecordParams) -> Result<()> {
    logi!("[store_grant_record] mock implementation for test");
    Ok(())
}