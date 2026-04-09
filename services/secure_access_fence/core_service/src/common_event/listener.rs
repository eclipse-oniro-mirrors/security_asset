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

use std::{slice, collections::HashMap};

use saf_log::{loge, logi};
use saf_plugin::saf_plugin::SAFPlugin;

use crate::CommonEventInfoFfi;

pub(crate) extern "C" fn on_common_event(common_event_info: CommonEventInfoFfi) {
    if let Ok(plugin) = SAFPlugin::get_instance().load_plugin() {
        let want_vec: Vec<String> = unsafe {
            slice::from_raw_parts(common_event_info.want.data,
                common_event_info.want.size as usize).to_vec()
        };

        let want_map: HashMap<String, String> = {
            let mut res = HashMap::new();
            let mut want_iter = want_vec.into_iter();
            while let Some(k) = want_iter.next() {
                if let Some(v) = want_iter.next() {
                    res.insert(k, v);
                }
            }
            res
        };

        match plugin.on_common_event(&common_event_info.event_type, &want_map) {
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
