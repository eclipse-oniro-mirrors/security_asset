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

use asset_definition::Result;
use asset_log::{loge, logi};

use hisysevent::{build_number_param, build_str_param, write, EventType, HiSysEventParam};

use crate::calling_info::CallingInfo;

enum EventKind {
    Fault,
    Statistics,
}

impl EventKind {
    const ASSET_FAULT: &str = "ASSET_FAULT";
    const ASSET_STATISTICS: &str = "ASSET_STATISTICS";

    fn as_str(&self) -> &str {
        match self {
            EventKind::Fault => Self::ASSET_FAULT,
            EventKind::Statistics => Self::ASSET_STATISTICS,
        }
    }
}

/// System events structure which base on `Hisysevent`.
struct SysEvent<'a> {
    event_kind: EventKind,
    inner_type: EventType,
    params: Vec<HiSysEventParam<'a>>,
}

impl<'a> SysEvent<'a> {
    const DOMAIN: &str = "ASSET";

    pub(crate) const FUNCTION: &str = "FUNCTION";
    pub(crate) const USER_ID: &str = "USER_ID";
    pub(crate) const CALLER: &str = "CALLER";
    pub(crate) const ERROR_CODE: &str = "ERROR_CODE";
    pub(crate) const RUN_TIME: &str = "RUN_TIME";
    pub(crate) const EXTRA: &str = "EXTRA";

    fn new_fault() -> Self {
        Self { event_kind: EventKind::Fault, inner_type: EventType::Fault, params: Vec::new() }
    }

    fn new_statistics() -> Self {
        Self { event_kind: EventKind::Statistics, inner_type: EventType::Statistic, params: Vec::new() }
    }

    fn set_param(mut self, param: HiSysEventParam<'a>) -> Self {
        self.params.push(param);
        self
    }

    fn write(self) {
        write(Self::DOMAIN, self.event_kind.as_str(), self.inner_type, self.params.as_slice());
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
            SysEvent::new_statistics()
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
            SysEvent::new_fault()
                .set_param(build_str_param!(SysEvent::FUNCTION, "add"))
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
