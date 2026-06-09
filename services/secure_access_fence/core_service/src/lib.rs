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
use saf_ipc::{SA_ID, VerifyTicketInfo};
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

    fn batch_generate_ticket(&self, os_account_id: i32, caller_id: &str, messages: &[String]) ->
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
            messages
        )
    }

    fn batch_verify_ticket(&self, os_account_id: i32, caller_id: &str, verify_infos: &[VerifyTicketInfo]) ->
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
}

#[cfg(feature = "SAFTest")]
/// stub for test
pub mod ut_core_service_lib_stub {
    include!{"../../../test/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs"}
}
