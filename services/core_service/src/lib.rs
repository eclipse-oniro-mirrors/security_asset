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

use std::time::{Duration, Instant};

use samgr::manage::SystemAbilityManager;
use system_ability_fwk::{
    ability::{Ability, Handler},
    cxx_share::SystemAbilityOnDemandReason,
};
use ylong_runtime::time::sleep;

use asset_constants::CallingInfo;
use asset_crypto_manager::crypto_manager::CryptoManager;
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result, Tag};
use asset_ipc::SA_ID;
use asset_log::{loge, logi};
use asset_plugin::asset_plugin::{AssetPlugin, AssetContext};

mod common_event;
mod counter;
mod operations;
mod stub;
mod sys_event;
mod trace_scope;
mod unload_handler;

use sys_event::upload_system_event;
use trace_scope::TraceScope;

use crate::counter::Counter;
use crate::unload_handler::{UnloadHandler, DELAYED_UNLOAD_TIME_IN_SEC, SEC_TO_MILLISEC};

struct AssetAbility;

pub(crate) fn unload_sa(duration: u64) {
    let unload_handler = UnloadHandler::get_instance();
    unload_handler.lock().unwrap().update_task(ylong_runtime::spawn(async move {
        sleep(Duration::from_secs(duration)).await;
        SystemAbilityManager::unload_system_ability(SA_ID);
    }));
}

impl Ability for AssetAbility {
    fn on_start_with_reason(&self, reason: SystemAbilityOnDemandReason, handler: Handler) {
        logi!("[INFO]Start asset service, reason_id: {:?}", reason.reason_id);

        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        let calling_info = CallingInfo::new_self();

        let _ = upload_system_event(start_service(handler), &calling_info, start, func_name);
        common_event::handle_common_event(reason);
        unload_sa(DELAYED_UNLOAD_TIME_IN_SEC as u64);
    }

    fn on_active(&self, reason: SystemAbilityOnDemandReason) {
        logi!("[INFO]Asset service on_active.");
        common_event::handle_common_event(reason);
    }

    fn on_idle(&self, _reason: SystemAbilityOnDemandReason) -> i32 {
        let crypto_manager = CryptoManager::get_instance();
        let max_crypto_expire_duration = crypto_manager.lock().unwrap().max_crypto_expire_duration();
        if max_crypto_expire_duration > 0 {
            logi!("[INFO]Asset service on idle not success, delay time: {}s", max_crypto_expire_duration);
            return max_crypto_expire_duration as i32 * SEC_TO_MILLISEC;
        }

        let counter = Counter::get_instance();
        if counter.lock().unwrap().count() > 0 {
            logi!(
                "[INFO]Asset service on idle not success for use_account: {}, delay time: {}s",
                counter.lock().unwrap().count(),
                DELAYED_UNLOAD_TIME_IN_SEC
            );
            return DELAYED_UNLOAD_TIME_IN_SEC * SEC_TO_MILLISEC;
        }
        logi!("[INFO]Asset service on_idle.");
        0
    }

    fn on_stop(&self) {
        logi!("[INFO]Asset service on_stop");
        common_event::unsubscribe();
    }
}

fn start_service(handler: Handler) -> Result<()> {
    let asset_plugin = AssetPlugin::get_instance();
    match asset_plugin.lock().unwrap().load_plugin() {
        Ok(loader) => {
            let _tr = loader.init(Box::new(AssetContext {data_base: None}));
            logi!("load plugin success.");
        },
        Err(_) => loge!("load plugin failed."),
    }

    common_event::subscribe();
    if !handler.publish(AssetService::new(handler.clone())) {
        return log_throw_error!(ErrCode::IpcError, "Asset publish stub object failed");
    };
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

struct AssetService {
    system_ability: system_ability_fwk::ability::Handler,
}

macro_rules! execute {
    ($func:path, $args:expr) => {{
        let func_name = hisysevent::function!();
        let specific_user_id = $args.get(&Tag::UserId);
        let calling_info = CallingInfo::build(specific_user_id.cloned())?;
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        // Create database directory if not exists.
        asset_file_operator::create_user_db_dir(calling_info.user_id())?;
        upload_system_event($func($args, &calling_info), &calling_info, start, func_name)
    }};
    ($func:path, $args1:expr, $args2:expr) => {{
        let func_name = hisysevent::function!();
        let specific_user_id = $args1.get(&Tag::UserId);
        let calling_info = CallingInfo::build(specific_user_id.cloned())?;
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        // Create database directory if not exists.
        asset_file_operator::create_user_db_dir(calling_info.user_id())?;
        upload_system_event($func($args1, $args2, &calling_info), &calling_info, start, func_name)
    }};
}

impl AssetService {
    pub(crate) fn new(handler: system_ability_fwk::ability::Handler) -> Self {
        Self { system_ability: handler }
    }

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
