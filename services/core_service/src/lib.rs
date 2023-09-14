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

//! This create implement the asset

use std::ffi::{c_char, CString};

use asset_common::{
    logi,
    definition::{AssetMap, Result},
};
use asset_ipc_interface::{IAsset, SA_ID, IpcCode};
use stub::AssetStub;

use hilog_rust::{error, hilog, HiLogLabel, LogType};
use ipc_rust::{IRemoteBroker, RemoteObj};
use system_ability_fwk_rust::{define_system_ability, IMethod, ISystemAbility, RSystemAbility};

mod stub;
mod operations;
mod calling_process_info;
use calling_process_info::CallingInfo;
use operations::check_params;

/// xxx
pub struct AssetService;

impl IRemoteBroker for AssetService {}

impl IAsset for AssetService {
    fn add(&self, input: &AssetMap) -> Result<()> {
        // get calling uid userid appid etc
        check_params(input, IpcCode::Add)?; // todo 等待ipc_code的定义，第二参数取代为枚举
        operations::add(input, &CallingInfo::build()?)
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

fn on_start<T: ISystemAbility + IMethod>(ability: &T) {
    let service = AssetStub::new_remote_stub(AssetService).expect("create AssetService failed");
    ability.publish(&service.as_object().expect("publish Asset service failed"), SA_ID);
    logi!("on_start");
}

fn on_stop<T: ISystemAbility + IMethod>(_ability: &T) {
    logi!("on_stop");
}

#[used]
#[link_section = ".init_array"]
static A: extern fn() = {
    extern fn init() {
        let r_sa = SystemAbility::new_system_ability(SA_ID, true)
            .expect("create TestService failed");
        r_sa.register();
    }
    init
};
