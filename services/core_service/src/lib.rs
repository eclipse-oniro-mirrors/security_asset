/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This module implements the Asset service.

use std::{
    ffi::{c_char, CString},
    thread,
    time::Instant,
};

use hilog_rust::{error, hilog, HiLogLabel, LogType};
use ipc_rust::{IRemoteBroker, RemoteObj};
use system_ability_fwk_rust::{define_system_ability, IMethod, ISystemAbility, RSystemAbility};

use asset_constants::CallingInfo;
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result};
use asset_ipc::{IAsset, SA_ID};
use asset_log::{loge, logi};

mod listener;
mod operations;
mod stub;
mod sys_event;
mod trace_scope;

use stub::AssetStub;
use sys_event::upload_system_event;
use trace_scope::TraceScope;

const LOG_LABEL: HiLogLabel = HiLogLabel { log_type: LogType::LogCore, domain: 0xD002F08, tag: "Asset" };

define_system_ability!(
    sa: SystemAbility(on_start, on_stop),
);

fn start_service<T: ISystemAbility + IMethod>(ability: &T) -> Result<()> {
    let Some(service) = AssetStub::new_remote_stub(AssetService) else {
        return log_throw_error!(ErrCode::IpcError, "Create AssetService failed!");
    };

    let Some(obj) = service.as_object() else {
        return log_throw_error!(ErrCode::IpcError, "Asset service as_object failed!");
    };

    ability.publish(&obj, SA_ID);
    logi!("[INFO]Asset service on_start");
    thread::spawn(listener::subscribe);
    Ok(())
}

fn on_start<T: ISystemAbility + IMethod>(ability: &T) {
    let func_name = hisysevent::function!();
    let start = Instant::now();
    let _trace = TraceScope::trace(func_name);
    let calling_info = CallingInfo::new_self();
    let _ = upload_system_event(start_service(ability), &calling_info, start, func_name);
}

fn on_stop<T: ISystemAbility + IMethod>(_ability: &T) {
    logi!("[INFO]Asset service on_stop");
    listener::unsubscribe();
}

#[used]
#[link_section = ".init_array"]
static A: extern "C" fn() = {
    extern "C" fn init() {
        let Some(sa) = SystemAbility::new_system_ability(SA_ID, true) else {
            loge!("Create Asset service failed.");
            return;
        };
        sa.register();
    }
    init
};

struct AssetService;

macro_rules! execute {
    ($func:path, $($args:expr), *) => {{
        let func_name = hisysevent::function!();
        let calling_info = CallingInfo::build()?;
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        upload_system_event($func($($args), *, &calling_info), &calling_info, start, func_name)
    }};
}

impl IRemoteBroker for AssetService {}
impl IAsset for AssetService {
    fn add(&self, attributes: &AssetMap) -> Result<()> {
        execute!(operations::add, attributes)
    }

    fn remove(&self, query: &AssetMap) -> Result<()> {
        execute!(operations::remove, query)
    }

    fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        execute!(operations::update, query, attributes_to_update)
    }

    fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        execute!(operations::pre_query, query)
    }

    fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        execute!(operations::query, query)
    }

    fn post_query(&self, query: &AssetMap) -> Result<()> {
        execute!(operations::post_query, query)
    }
}
