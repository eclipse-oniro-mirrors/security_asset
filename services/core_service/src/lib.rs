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
    time::Duration,
};

use hilog_rust::{error, hilog, HiLogLabel, LogType};
use ipc_rust::{IRemoteBroker, RemoteObj};
use system_ability_fwk_rust::{define_system_ability, IMethod, ISystemAbility, RSystemAbility};

use asset_definition::{AssetMap, Result};
use asset_ipc::{IAsset, SA_ID};
use asset_log::logi;

mod calling_info;
mod operations;
mod stub;

use calling_info::CallingInfo;
use stub::AssetStub;

const LOG_LABEL: HiLogLabel = HiLogLabel { log_type: LogType::LogCore, domain: 0xD002F70, tag: "Asset" };

define_system_ability!(
    sa: SystemAbility(on_start, on_stop),
);

const MAX_DELAY_TIMES: u32 = 100;
const DELAY_INTERVAL: u64 = 200000;

extern "C" {
    fn SubscribeSystemEvent() -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

fn on_start<T: ISystemAbility + IMethod>(ability: &T) {
    let service = AssetStub::new_remote_stub(AssetService).expect("create AssetService failed");
    ability.publish(&service.as_object().expect("publish Asset service failed"), SA_ID);
    logi!("[INFO]Asset service on_start");
    unsafe {
        for i in 0..MAX_DELAY_TIMES {
            thread::sleep(Duration::from_millis(DELAY_INTERVAL));
            if SubscribeSystemEvent() {
                logi!("SubscribeSystemEvent success, i = {}", i);
                return;
            }
            logi!("SubscribeSystemEvent failed {} times", i);
        }
        logi!("SubscribeSystemEvent failed");
    }
}

fn on_stop<T: ISystemAbility + IMethod>(_ability: &T) {
    logi!("[INFO]Asset service on_stop");
    unsafe {
        UnSubscribeSystemEvent();
    }
}

#[used]
#[link_section = ".init_array"]
static A: extern "C" fn() = {
    extern "C" fn init() {
        let r_sa = SystemAbility::new_system_ability(SA_ID, true).expect("create Asset service failed");
        r_sa.register();
    }
    init
};

struct AssetService; // 默认调用CryptoManager::new

impl IRemoteBroker for AssetService {}

impl IAsset for AssetService {
    fn add(&self, attributes: &AssetMap) -> Result<()> {
        operations::add(attributes, &CallingInfo::build()?)
    }

    fn remove(&self, query: &AssetMap) -> Result<()> {
        operations::remove(query, &CallingInfo::build()?)
    }

    fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        operations::update(query, attributes_to_update, &CallingInfo::build()?)
    }

    fn pre_query(&self, query: &AssetMap) -> Result<Vec<u8>> {
        operations::pre_query(query, &CallingInfo::build()?) // todo 传CryptoManager实例
    }

    fn query(&self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        operations::query(query, &CallingInfo::build()?)
    }

    fn post_query(&self, query: &AssetMap) -> Result<()> {
        operations::post_query(query, &CallingInfo::build()?)
    }
}
