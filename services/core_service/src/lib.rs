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

use asset_definition::{AssetMap, Result};
use asset_ipc::{IAsset, SA_ID};
use asset_log::{loge, logi};
use asset_system_ability::{subscribe_system_abillity, unsubscribe_system_ability};

mod calling_info;
mod operations;
mod stub;
mod sys_event;
mod trace_scope;

use calling_info::CallingInfo;
use stub::AssetStub;
use sys_event::upload_system_event;
use trace_scope::TraceScope;

const LOG_LABEL: HiLogLabel = HiLogLabel { log_type: LogType::LogCore, domain: 0xD002F70, tag: "Asset" };

define_system_ability!(
    sa: SystemAbility(on_start, on_stop),
);

fn on_start<T: ISystemAbility + IMethod>(ability: &T) {
    let Some(service) = AssetStub::new_remote_stub(AssetService) else {
        loge!("Create AssetService failed!");
        return;
    };

    let Some(obj) = service.as_object() else {
        loge!("Public Asset service failed!");
        return;
    };

    ability.publish(&obj, SA_ID);
    logi!("[INFO]Asset service on_start");
    thread::spawn(subscribe_system_abillity);
}

fn on_stop<T: ISystemAbility + IMethod>(_ability: &T) {
    logi!("[INFO]Asset service on_stop");
    unsubscribe_system_ability();
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

impl AssetService {
    fn execute<T, F: Fn(&AssetMap, &CallingInfo) -> Result<T>>(
        func_name: &str,
        attrs: &AssetMap,
        func: F,
    ) -> Result<T> {
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        let calling_info = CallingInfo::build()?;
        upload_system_event(func(attrs, &calling_info), &calling_info, start, func_name)
    }
}

impl IRemoteBroker for AssetService {}
/// todo yyd 把调用函数的使用宏来代替 统一代码格式
impl IAsset for AssetService {
    fn add(&self, attributes: &AssetMap) -> Result<()> {
        AssetService::execute(hisysevent::function!(), attributes, operations::add)
    }

    fn remove(&self, query: &AssetMap) -> Result<()> {
        AssetService::execute(hisysevent::function!(), query, operations::remove)
    }

    fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        let calling_info = CallingInfo::build()?;
        upload_system_event(
            operations::update(query, attributes_to_update, &calling_info),
            &calling_info,
            start,
            func_name,
        )
    }

    fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        AssetService::execute(hisysevent::function!(), query, operations::pre_query)
    }

    fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        AssetService::execute(hisysevent::function!(), query, operations::query)
    }

    fn post_query(&self, query: &AssetMap) -> Result<()> {
        AssetService::execute(hisysevent::function!(), query, operations::post_query)
    }
}
