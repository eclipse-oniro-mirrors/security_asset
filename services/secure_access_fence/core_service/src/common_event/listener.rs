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

//! This module is used to subscribe common event and system ability.

use std::slice;

use saf_definition::Value;
use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;
use saf_plugin_interface::plugin_interface::{
    ExtMap, PARAM_NAME_COMMON_EVENT_TYPE, PARAM_NAME_COMMON_EVENT_UID, PARAM_NAME_COMMON_EVENT_APP_INDEX,
    PARAM_NAME_COMMON_EVENT_BUNDLE_NAME, PARAM_NAME_COMMON_EVENT_USER_ID
};

use crate::CommonEventInfoFfi;

pub(crate) extern "C" fn on_common_event(common_event_info: CommonEventInfoFfi) {
    if let Ok(load) = SAFPlugin::get_instance().load_plugin() {
        let mut params = ExtMap::new();
        params.insert(PARAM_NAME_COMMON_EVENT_TYPE, Value::Number(common_event_info.event_type as u32));

        let uid: Vec<u8> = unsafe {
            slice::from_raw_parts(common_event_info.uid.data, common_event_info.uid.size as usize).to_vec()
        };
        params.insert(PARAM_NAME_COMMON_EVENT_UID, Value::Bytes(uid));

        let app_index: Vec<u8> = unsafe {
            slice::from_raw_parts(common_event_info.app_index.data, common_event_info.app_index.size as usize).to_vec()
        };
        params.insert(PARAM_NAME_COMMON_EVENT_APP_INDEX, Value::Bytes(app_index));

        let bundle_name: Vec<u8> = unsafe {
            slice::from_raw_parts(common_event_info.bundle_name.data, common_event_info.bundle_name.size as usize).to_vec()
        };
        params.insert(PARAM_NAME_COMMON_EVENT_BUNDLE_NAME, Value::Bytes(bundle_name));

        let user_id: Vec<u8> = unsafe {
            slice::from_raw_parts(common_event_info.user_id.data, common_event_info.user_id.size as usize).to_vec()
        };
        params.insert(PARAM_NAME_COMMON_EVENT_USER_ID, Value::Bytes(user_id));

        match load.on_common_event(&mut params) {
            Ok(_) => logi!("process common event success."),
            Err(code) => loge!("process common event failed, code: {}", code),
        }
    }
}

#[derive(Clone)]
#[repr(C)]
struct EventCallBack {
    on_common_event: extern "C" fn(CommonEventInfoFfi),
}

extern "C" {
    fn SubscribeSystemAbility(eventCallBack: EventCallBack) -> bool;
    fn UnSubscribeSystemAbility() -> bool;
    fn SubscribeSystemEvent(eventCallBack: EventCallBack) -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

/// Subscribe to the add and remove events of system abilities.
pub(crate) fn subscribe() {
    unsafe {
        let call_back = EventCallBack {
            on_common_event,
        };
        if SubscribeSystemEvent(call_back.clone()) {
            logi!("Subscribe system event success.");
        } else {
            loge!("Subscribe system event failed. Subscribe System Ability wait system event service start.");
            if SubscribeSystemAbility(call_back) {
                logi!("Subscribe system ability success.");
            } else {
                loge!("Subscribe system ability failed.")
            }
        }
    }
}

/// Unsubscribe to the add and remove events of system abilities.
pub(crate) fn unsubscribe() {
    unsafe {
        if !UnSubscribeSystemAbility() {
            loge!("Unsubscribe system ability failed.")
        }

        if !UnSubscribeSystemEvent() {
            loge!("Unsubscribe system event failed.")
        }
    }
}
