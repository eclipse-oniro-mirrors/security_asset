/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

//! This module is used to handle start event.

use std::collections::HashMap;

use saf_common::{AutoCounter, ConstSAFBlob, TaskManager};
use saf_definition::Result;
use saf_log::logi;
use system_ability_fwk::cxx_share::SystemAbilityOnDemandReason;

use crate::{common_event::listener, unload_sa, CommonEventType, CommonEventInfo, CommonEventInfoFfi, WantParser};

const USER_ID: &str = "userId";
const APP_INDEX: &str = "appIndex";
const BUNDLE_NAME: &str = "bundleName";
const UID: &str = "uid";
const DEFAULT_VAL: &str = "-1";

struct CommonEventWant<'a>(&'a HashMap<String, String>, String);
impl WantParser<CommonEventInfo> for CommonEventWant<'_> {
    fn parse(&self) -> Result<CommonEventInfo> {
        let user_id = match self.0.get(USER_ID) {
            Some(val) => val.clone(),
            None => DEFAULT_VAL.to_string()
        };

        let app_index = match self.0.get(APP_INDEX) {
            Some(val) => val.clone(),
            None => DEFAULT_VAL.to_string()
        };

        let bundle_name = match self.0.get(BUNDLE_NAME) {
            Some(val) => val.clone(),
            None => DEFAULT_VAL.to_string()
        };

        let uid = match self.0.get(UID) {
            Some(val) => val.clone(),
            None => DEFAULT_VAL.to_string()
        };

        let event_type = CommonEventType::try_from(self.1.as_str())?;

        Ok(CommonEventInfo { event_type, uid, app_index, bundle_name, user_id })
    }
}

fn process_common_event_async(reason: SystemAbilityOnDemandReason) {
    let _counter_user = AutoCounter::new();
    let reason_name: String = reason.name;
    let want = reason.extra_data.want();
    if let Ok(common_event_info) = CommonEventWant(&want, reason_name.clone()).parse() {
        let uid = ConstSAFBlob {
            size: common_event_info.uid().len() as u32, data: common_event_info.uid().as_ptr()
        };

        let app_index = ConstSAFBlob {
            size: common_event_info.app_index().len() as u32, data: common_event_info.app_index().as_ptr()
        };

        let bundle_name = ConstSAFBlob {
            size: common_event_info.bundle_name().len() as u32, data: common_event_info.bundle_name().as_ptr()
        };

        let user_id = ConstSAFBlob {
            size: common_event_info.user_id().len() as u32, data: common_event_info.user_id().as_ptr()
        };

        listener::on_common_event(CommonEventInfoFfi {
            event_type: common_event_info.event_type(),
            uid,
            app_index,
            bundle_name,
            user_id
        });
    };
    logi!("[INFO]Finish handle common event. [{}]", reason_name);
}

pub(crate) fn handle_common_event(reason: SystemAbilityOnDemandReason) {
    let handle = ylong_runtime::spawn_blocking(move || process_common_event_async(reason));
    let task_manager = TaskManager::get_instance();
    task_manager.lock().unwrap().push_task(handle);
    unload_sa();
}
