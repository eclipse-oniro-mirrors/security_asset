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

//! RemoteMessage JSON serialization and deserialization utilities.

use saf_utils::{JsonBuilder, JsonValue, Object, new_object, object_add_string, take_optional_string,
    take_required_string, take_required_value, json_into_object};
use saf_definition::{macros_lib, ErrCode, Result, DeviceIdHeader, RemoteMessage};

/// Remote message JSON field keys for serialization/deserialization.
pub struct RemoteMessageKeys {
    /// The device info object key
    pub device_info: &'static str,
    /// The controller device ID key (inside deviceInfo)
    pub controller_device_id: &'static str,
    /// The controlled device ID key (inside deviceInfo)
    pub controlled_device_id: &'static str,
    /// The remote auth message key
    pub remote_auth_message: &'static str,
    /// The caller bundle name key
    pub caller_bundle_name: &'static str,
}

/// Global constant instance for remote message JSON field keys.
pub const REMOTE_MESSAGE_KEYS: RemoteMessageKeys = RemoteMessageKeys {
    device_info: "deviceInfo",
    controller_device_id: "controllerDeviceId",
    controlled_device_id: "controlledDeviceId",
    remote_auth_message: "remoteAuthMessage",
    caller_bundle_name: "callerBundleName",
};

/// Serialize DeviceIdHeader to JSON string
pub fn serialize_device_id_header_to_json(header: &DeviceIdHeader) -> Result<String> {
    let mut builder = JsonBuilder::new();
    builder.add_string(REMOTE_MESSAGE_KEYS.controller_device_id, &header.controller_device_id);
    builder.add_string(REMOTE_MESSAGE_KEYS.controlled_device_id, &header.controlled_device_id);
    builder.build()
}

/// Deserialize DeviceIdHeader from an Object (the "deviceInfo" sub-object), consuming the fields
pub fn parse_device_id_header_from_json(device_info_obj: &mut Object) -> Result<DeviceIdHeader> {
    let controller_device_id = take_optional_string(device_info_obj, REMOTE_MESSAGE_KEYS.controller_device_id)?;
    let controlled_device_id = take_optional_string(device_info_obj, REMOTE_MESSAGE_KEYS.controlled_device_id)?;

    Ok(DeviceIdHeader {
        controller_device_id,
        controlled_device_id
    })
}

/// Serialize RemoteMessage to JSON string.
pub fn serialize_remote_message_to_json(message: &RemoteMessage) -> Result<String> {
    let mut device_info_obj = new_object();
    object_add_string(&mut device_info_obj, REMOTE_MESSAGE_KEYS.controller_device_id, 
        &message.device_info.controller_device_id);
    object_add_string(&mut device_info_obj, REMOTE_MESSAGE_KEYS.controlled_device_id, 
        &message.device_info.controlled_device_id);
    
    let mut builder = JsonBuilder::new();
    builder.add_object(REMOTE_MESSAGE_KEYS.device_info, device_info_obj);
    builder.add_string(REMOTE_MESSAGE_KEYS.remote_auth_message, &message.remote_auth_message);
    builder.add_string(REMOTE_MESSAGE_KEYS.caller_bundle_name, &message.caller_bundle_name);
    builder.build()
}

/// Deserialize RemoteMessage from JSON string.
pub fn deserialize_remote_message_from_json(json_str: &str) -> Result<RemoteMessage> {
    let json = JsonValue::from_text(json_str).map_err(|e| {
        macros_lib::log_and_into_saf_error!(
            ErrCode::JsonParseError,
            "[FATAL][IPC]Failed to parse remoteMessage JSON: {}", e
        )
    })?;
    let mut obj = json_into_object(json)?;
    let mut device_info_obj = json_into_object(take_required_value(&mut obj, REMOTE_MESSAGE_KEYS.device_info)?)?;
    let device_info = parse_device_id_header_from_json(&mut device_info_obj)?;
    let remote_auth_message = take_required_string(&mut obj, REMOTE_MESSAGE_KEYS.remote_auth_message)?;
    let caller_bundle_name = take_optional_string(&mut obj, REMOTE_MESSAGE_KEYS.caller_bundle_name)?;
    
    Ok(RemoteMessage {
        device_info,
        remote_auth_message,
        caller_bundle_name
    })
}
