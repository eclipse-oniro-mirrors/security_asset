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

use asset_common_lib::{asset_type::{AssetMap, AssetTag, AssetResult, AssetValue}, asset_log_info};
use asset_ipc_define_lib::asset_service::{AssetStub, AssetBroker, ASSET_SERVICE_ID};

use ipc_rust::{IRemoteBroker, RemoteObj};

use hilog_rust::{hilog, error, HiLogLabel, LogType};
use std::ffi::{c_char, CString};

use system_ability_fwk_rust::{
    define_system_ability, RSystemAbility, ISystemAbility, IMethod
};


/// xxx
pub struct AssetService;

impl IRemoteBroker for AssetService {}

impl AssetBroker for AssetService {
    fn insert(&self, _input: &AssetMap) -> AssetResult<AssetMap> {
        let mut map = AssetMap::new();
        map.insert(AssetTag::AssetTagAuthType, AssetValue::NUMBER(2)); // to do
        Ok(map)
    }
}

// /// xxx
// pub fn asset_server_init() {
//     // create stub
//     let service = AssetStub::new_remote_stub(AssetService).expect("create AssetService success");
//     add_service(&service.as_object().expect("get Asset service failed"), ASSET_SERVICE_ID).expect("add server to samgr failed"); 
//     join_work_thread();   
// }

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002F00, // security domain
    tag: "Asset"
};

define_system_ability!(
    sa: SystemAbility(on_start, on_stop),
);

fn on_start<T: ISystemAbility + IMethod>(ability: &T) {
    let service = AssetStub::new_remote_stub(AssetService).expect("create TestService failed");
    ability.publish(&service.as_object().expect("get ITest service failed"), ASSET_SERVICE_ID);
    asset_log_info!("on_start");
}

fn on_stop<T: ISystemAbility + IMethod>(_ability: &T) {
    asset_log_info!("on_stop");
}

#[used]
#[link_section = ".init_array"]
static A:extern fn() = {
    extern fn init() {
        let r_sa = SystemAbility::new_system_ability(ASSET_SERVICE_ID, true).expect("create TestService failed");
        r_sa.register();
    }
    init
};