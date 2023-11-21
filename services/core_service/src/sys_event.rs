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

//! This module is used to Asset service hisysevent.

use std::time::Instant;

use asset_constants::CallingInfo;
use asset_definition::Result;
use asset_log::{loge, logi};

use hisysevent::{build_number_param, build_str_param, write, EventType, HiSysEventParam};

/// System events structure which base on `Hisysevent`.
struct SysEvent<'a> {
    event_type: EventType,
    params: Vec<HiSysEventParam<'a>>,
}

impl<'a> SysEvent<'a> {
    const DOMAIN: &str = "ASSET";
    const ASSET_FAULT: &str = "SECRET_STORE_OPERATION_FAILED";
    const ASSET_STATISTIC: &str = "SECRET_STORE_INFO_COLLECTION";

    pub(crate) const FUNCTION: &str = "FUNCTION";
    pub(crate) const USER_ID: &str = "USER_ID";
    pub(crate) const CALLER: &str = "CALLER";
    pub(crate) const ERROR_CODE: &str = "ERROR_CODE";
    pub(crate) const RUN_TIME: &str = "RUN_TIME";
    pub(crate) const EXTRA: &str = "EXTRA";

    fn new(event_type: EventType) -> Self {
        Self { event_type, params: Vec::new() }
    }

    fn set_param(mut self, param: HiSysEventParam<'a>) -> Self {
        self.params.push(param);
        self
    }

    fn write(self) {
        let event_name = match self.event_type {
            EventType::Fault => Self::ASSET_FAULT,
            EventType::Statistic => Self::ASSET_STATISTIC,
            _ => "UNKNOWN_EVENT",
        };
        write(Self::DOMAIN, event_name, self.event_type, self.params.as_slice());
    }
}

pub(crate) fn upload_system_event<T>(
    result: Result<T>,
    calling_info: &CallingInfo,
    start_time: Instant,
    func_name: &str,
) -> Result<T> {
    let owner_info = String::from_utf8_lossy(calling_info.owner_info()).to_string();
    match &result {
        Ok(_) => {
            let duration = start_time.elapsed();
            SysEvent::new(EventType::Statistic)
                .set_param(build_str_param!(SysEvent::FUNCTION, func_name))
                .set_param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
                .set_param(build_str_param!(SysEvent::CALLER, owner_info.clone()))
                .set_param(build_number_param!(SysEvent::RUN_TIME, duration.as_millis() as u32))
                .set_param(build_str_param!(SysEvent::EXTRA, ""))
                .write();
            logi!(
                "[INFO]Calling fun:[{}], user_id:[{}], caller:[{}], run_time:[{}]",
                func_name,
                calling_info.user_id(),
                owner_info,
                duration.as_millis()
            )
        },
        Err(e) => {
            SysEvent::new(EventType::Fault)
                .set_param(build_str_param!(SysEvent::FUNCTION, func_name))
                .set_param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
                .set_param(build_str_param!(SysEvent::CALLER, owner_info.clone()))
                .set_param(build_number_param!(SysEvent::ERROR_CODE, e.code as i32))
                .set_param(build_str_param!(SysEvent::EXTRA, e.msg.clone()))
                .write();
            loge!(
                "[ERROR]Calling fun:[{}], user_id:[{}], caller:[{}], error_code:[{}], error_msg:[{}]",
                func_name,
                calling_info.user_id(),
                owner_info,
                e.code,
                e.msg.clone()
            );
        },
    }
    result
}
