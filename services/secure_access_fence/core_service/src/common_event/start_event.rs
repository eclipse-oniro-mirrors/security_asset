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
use std::ffi::CString;

use saf_common::{AutoCounter, TaskManager};
use saf_log::logi;
use system_ability_fwk::cxx_share::SystemAbilityOnDemandReason;

use crate::{common_event::listener, unload_sa, CommonEventInfoFfi, StringArray};

fn process_common_event_async(reason: SystemAbilityOnDemandReason) {
    let _counter_user = AutoCounter::new();
    let reason_name: String = reason.name;
    let want = reason.extra_data.want();

    let want_vec: Vec<String> = want.into_iter()
        .flat_map(|(k, v)| vec![k, v])
        .collect();

    let reason_c_str = CString::new(reason_name.clone()).unwrap();

    let mut c_strings = Vec::new();
    for s in want_vec {
        let c_str = CString::new(s).unwrap();
        c_strings.push(c_str);
    }

    let size = c_strings.len() as u32;
    let mut data = Vec::with_capacity(size as usize);
    for c_str in &c_strings {
        data.push(c_str.as_ptr());
    }

    listener::on_common_event(CommonEventInfoFfi {
        event_type: reason_c_str.as_ptr(),
        want: StringArray {
            size,
            data: data.as_ptr(),
        }
    });
    logi!("Finish handle common event. [{}]", reason_name);
}

pub(crate) fn handle_common_event(reason: SystemAbilityOnDemandReason) {
    let handle = ylong_runtime::spawn_blocking(move || process_common_event_async(reason));
    let task_manager = TaskManager::get_instance();
    task_manager.lock().unwrap().push_task(handle);
    unload_sa();
}
