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

//! This crate implements the asset

use std::ffi::{c_char, CString};

use asset_common::{
    logi,
    definition::{AssetMap, Result},
};
use asset_ipc_interface::{IAsset, SA_ID};
use stub::AssetStub;

use hilog_rust::{error, hilog, HiLogLabel, LogType};
use ipc_rust::{IRemoteBroker, RemoteObj};
use system_ability_fwk_rust::{define_system_ability, IMethod, ISystemAbility, RSystemAbility};

mod stub;
mod operations;
mod calling_info;
mod definition_inner;
mod argument_check;

use calling_info::CallingInfo; // todo: calling_process_info -> calling_info

/// xxx
pub struct AssetService;

impl IRemoteBroker for AssetService {}

impl IAsset for AssetService {
    fn add(&self, input: &AssetMap) -> Result<()> { // todo param -> argument
        // check the validity and comprehensiveness of input params
        argument_check::check_argument(input, &argument_check::ArgumentCode::Add)?;

        // get calling uid userid appid etc and do add
        operations::add(input, &CallingInfo::build()?)
    }

    fn query(&self, input: &AssetMap) -> Result<Vec<AssetMap>> {
        // check the validity and comprehensiveness of input params
        argument_check::check_argument(input, &argument_check::ArgumentCode::Query)?;

        // get calling uid userid appid etc and do query
        operations::query(input, &CallingInfo::build()?)
    }

    fn pre_query(&self, input: &AssetMap) -> Result<Vec<u8>> {
        // check the validity and comprehensiveness of input params
        argument_check::check_argument(input, &argument_check::ArgumentCode::Query)?;

        // get calling uid userid appid etc and do query
        operations::pre_query(input, &CallingInfo::build()?)
    }

    fn update(&self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        // check the validity and comprehensiveness of input params
        argument_check::check_argument(query, &argument_check::ArgumentCode::UpdateQuery)?;
        argument_check::check_argument(attributes_to_update, &argument_check::ArgumentCode::Update)?;

        // get calling uid userid appid etc and do add
        operations::update(query, attributes_to_update, &CallingInfo::build()?)
    }

    fn remove(&self, input: &AssetMap) -> Result<()> {
        // check the validity and comprehensiveness of input params
        argument_check::check_argument(input, &argument_check::ArgumentCode::Remove)?;

        // get calling uid userid appid etc and do remove
        operations::remove(input, &CallingInfo::build()?)
    }
}

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002F70,
    tag: "Asset",
};

define_system_ability!(
    sa: SystemAbility(on_start, on_stop),
);

extern "C" {
    fn SubscribeSystemEvent() -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

use std::thread;
use std::time::Duration;

const MAX_DELAY_TIMES: u32 = 100;
const DELAY_INTERVAL: u64 = 200000;

fn on_start<T: ISystemAbility + IMethod>(ability: &T) {
    let service = AssetStub::new_remote_stub(AssetService).expect("create AssetService failed");
    ability.publish(&service.as_object().expect("publish Asset service failed"), SA_ID);
    logi!("on_start");
    unsafe{
        for i in 0..MAX_DELAY_TIMES {
            if SubscribeSystemEvent() {
                logi!("SubscribeSystemEvent success, i = {}", i);
                return;
            } else {
                logi!("SubscribeSystemEvent failed {} times", i);
                thread::sleep(Duration::from_millis(DELAY_INTERVAL));
            }
        }
        logi!("SubscribeSystemEvent failed");
    }
}

fn on_stop<T: ISystemAbility + IMethod>(_ability: &T) {
    logi!("on_stop");
    unsafe{ UnSubscribeSystemEvent(); }
}

#[used]
#[link_section = ".init_array"]
static A: extern fn() = {
    extern fn init() {
        let r_sa = SystemAbility::new_system_ability(SA_ID, true)
            .expect("create Asset service failed");
        r_sa.register();
    }
    init
};
