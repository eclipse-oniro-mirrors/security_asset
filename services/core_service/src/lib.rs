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

//! This module implements the Asset service.

use ipc::parcel::MsgParcel;
use samgr::manage::SystemAbilityManager;
use std::{
    fs,
    time::{Duration, Instant},
};
use system_ability_fwk::{
    ability::{Ability, Handler},
    cxx_share::SystemAbilityOnDemandReason,
};
use ylong_runtime::{builder::RuntimeBuilder, time::sleep};

use asset_common::{AutoCounter, CallingInfo, ConstAssetBlob, ConstAssetBlobArray, Counter};
use asset_crypto_manager::crypto_manager::CryptoManager;
use asset_db_operator::database_file_upgrade::check_and_split_db;
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result, SyncResult};
use asset_file_operator::{common::DE_ROOT_PATH, de_operator::create_user_de_dir};
use asset_ipc::SA_ID;
use asset_log::{loge, logi};
use asset_plugin::asset_plugin::{AssetContext, AssetPlugin};

mod common_event;
mod operations;
mod stub;
mod sys_event;
mod trace_scope;
mod unload_handler;

use sys_event::upload_system_event;
use trace_scope::TraceScope;

use crate::unload_handler::{UnloadHandler, DELAYED_UNLOAD_TIME_IN_SEC, SEC_TO_MILLISEC};

struct AssetAbility;

trait WantParser<T> {
    fn parse(&self) -> Result<T>;
}

struct PackageInfo {
    user_id: i32,
    app_index: i32,
    app_id: String,
    developer_id: Option<String>,
    group_ids: Option<Vec<String>>,
    bundle_name: String,
}

#[repr(C)]
struct PackageInfoFfi {
    user_id: i32,
    app_index: i32,
    owner: ConstAssetBlob,
    developer_id: ConstAssetBlob,
    group_ids: ConstAssetBlobArray,
    bundle_name: ConstAssetBlob,
}

impl PackageInfo {
    fn developer_id(&self) -> &Option<String> {
        &self.developer_id
    }

    fn group_ids(&self) -> &Option<Vec<String>> {
        &self.group_ids
    }
}

pub(crate) fn unload_sa(duration: u64) {
    let unload_handler = UnloadHandler::get_instance();
    unload_handler.lock().unwrap().update_task(ylong_runtime::spawn(async move {
        sleep(Duration::from_secs(duration)).await;
        logi!("[INFO]Start unload asset service");
        SystemAbilityManager::unload_system_ability(SA_ID);
    }));
}

impl Ability for AssetAbility {
    fn on_start_with_reason(&self, reason: SystemAbilityOnDemandReason, handler: Handler) {
        logi!("[INFO]Start asset service, reason_id: {:?}", reason.reason_id);
        if let Err(e) = RuntimeBuilder::new_multi_thread().worker_num(1).max_blocking_pool_size(1).build_global() {
            loge!("[WARNING]Ylong new global thread failed! {}", e);
        };
        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        let calling_info = CallingInfo::new_self();

        let _ = upload_system_event(start_service(handler), &calling_info, start, func_name, &AssetMap::new());
        common_event::handle_common_event(reason);
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
        let counter = Counter::get_instance();
        counter.lock().unwrap().stop();
        common_event::unsubscribe();
    }

    fn on_extension(&self, extension: String, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
        logi!("[INFO]Asset on_extension, extension is {}", extension);
        if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
            match load.on_sa_extension(extension, data, reply) {
                Ok(()) => logi!("process sa extension event success."),
                Err(code) => loge!("process sa extension event failed, code: {}", code),
            };
        }
        logi!("[INFO]Asset on_extension end");
        0
    }
}

async fn upgrade_process() -> Result<()> {
    let _counter_user = AutoCounter::new();
    for entry in fs::read_dir(DE_ROOT_PATH)? {
        let entry = entry?;
        if let Ok(user_id) = entry.file_name().to_string_lossy().parse::<i32>() {
            logi!("[INFO]start to check and split db in upgrade process.");
            check_and_split_db(user_id)?;
        }
    }
    Ok(())
}

fn start_service(handler: Handler) -> Result<()> {
    let asset_plugin = AssetPlugin::get_instance();
    match asset_plugin.load_plugin() {
        Ok(loader) => {
            let _tr = loader.init(Box::new(AssetContext { user_id: 0 }));
            logi!("load plugin success.");
        },
        Err(_) => loge!("load plugin failed."),
    }

    common_event::subscribe();
    if !handler.publish(AssetService::new(handler.clone())) {
        return log_throw_error!(ErrCode::IpcError, "Asset publish stub object failed");
    };
    let _handle = ylong_runtime::spawn(upgrade_process());
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
    ($func:path, $calling_info:expr, $first_arg:expr) => {{
        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        // Create de database directory if not exists.
        create_user_de_dir($calling_info.user_id())?;
        upload_system_event($func($calling_info, $first_arg), $calling_info, start, func_name, $first_arg)
    }};
    ($func:path, $calling_info:expr, $first_arg:expr, $second_arg:expr) => {{
        let func_name = hisysevent::function!();
        let start = Instant::now();
        let _trace = TraceScope::trace(func_name);
        // Create de database directory if not exists.
        create_user_de_dir($calling_info.user_id())?;
        upload_system_event($func($calling_info, $first_arg, $second_arg), $calling_info, start, func_name, $first_arg)
    }};
}

impl AssetService {
    pub(crate) fn new(handler: system_ability_fwk::ability::Handler) -> Self {
        Self { system_ability: handler }
    }

    fn add(&self, calling_info: &CallingInfo, attributes: &AssetMap) -> Result<()> {
        execute!(operations::add, calling_info, attributes)
    }

    fn remove(&self, calling_info: &CallingInfo, query: &AssetMap) -> Result<()> {
        execute!(operations::remove, calling_info, query)
    }

    fn update(&self, calling_info: &CallingInfo, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        execute!(operations::update, calling_info, query, attributes_to_update)
    }

    fn pre_query(&self, calling_info: &CallingInfo, query: &AssetMap) -> Result<Vec<u8>> {
        execute!(operations::pre_query, calling_info, query)
    }

    fn query(&self, calling_info: &CallingInfo, query: &AssetMap) -> Result<Vec<AssetMap>> {
        execute!(operations::query, calling_info, query)
    }

    fn post_query(&self, calling_info: &CallingInfo, query: &AssetMap) -> Result<()> {
        execute!(operations::post_query, calling_info, query)
    }

    fn query_sync_result(&self, calling_info: &CallingInfo, query: &AssetMap) -> Result<SyncResult> {
        execute!(operations::query_sync_result, calling_info, query)
    }
}
