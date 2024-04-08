/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use std::{thread, time::Instant};

use system_ability_fwk::ability::{Ability, PublishHandler};

use asset_constants::CallingInfo;
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result};
use asset_ipc::SA_ID;
use asset_log::{loge, logi};

mod listener;
mod operations;
mod stub;
mod sys_event;
mod trace_scope;

use sys_event::upload_system_event;
use trace_scope::TraceScope;

struct AssetAbility;

impl Ability for AssetAbility {
    fn on_start(&self, handler: PublishHandler) {
        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        let calling_info = CallingInfo::new_self();

        let _ = upload_system_event(start_service(handler), &calling_info, start, func_name);
    }

    fn on_stop(&self) {
        logi!("[INFO]Asset service on_stop");
        listener::unsubscribe();
    }
}

fn start_service(handler: PublishHandler) -> Result<()> {
    if handler.publish(AssetService) {
        return log_throw_error!(ErrCode::IpcError, "Asset publish stub object failed");
    };

    logi!("[INFO]Asset service on_start");
    thread::spawn(listener::subscribe);
    Ok(())
}

#[used]
#[link_section = ".init_array"]
static A: extern "C" fn() = {
    extern "C" fn init() {
        let Some(sa) = AssetAbility.build_system_ability(SA_ID, true) else {
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

impl AssetService {
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
