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

//! This module implements the stub of the Asset service.

use std::time::Instant;

use asset_log::{loge, logi};
// pub(crate) use hisysevent::{build_number_param, build_str_param};
use hisysevent::{build_number_param, build_str_param, write, EventType, HiSysEventParam};

use asset_definition::Result;

use crate::calling_info::CallingInfo;

/// System events structure which base on `Hisysevent`.
pub(crate) struct SysEvent<'a> {
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

    pub(crate) fn task_fault() -> Self {
        Self { event_kind: EventKind::Fault, inner_type: EventType::Fault, params: Vec::new() }
    }

    pub(crate) fn task_info_statistics() -> Self {
        Self { event_kind: EventKind::Statistics, inner_type: EventType::Statistic, params: Vec::new() }
    }

    pub(crate) fn param(mut self, param: HiSysEventParam<'a>) -> Self {
        self.params.push(param);
        self
    }

    pub(crate) fn write(self) {
        write(Self::DOMAIN, self.event_kind.as_str(), self.inner_type, self.params.as_slice());
    }
}

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

pub(crate) fn sys_event_log<T>(
    result: Result<T>,
    calling_info: &CallingInfo,
    start_time: Instant,
    fun_name: &str,
) -> Result<T> {
    match &result {
        Ok(_) => {
            let duration = start_time.elapsed();
            SysEvent::task_info_statistics()
                .param(build_str_param!(SysEvent::FUNCTION, fun_name))
                .param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
                .param(build_str_param!(SysEvent::CALLER, calling_info.owner_info_str()))
                .param(build_number_param!(SysEvent::RUN_TIME, duration.as_millis() as u32))
                .param(build_str_param!(SysEvent::EXTRA, ""))
                .write();
            logi!(
                "[INFO] use fun:[{}], user_id:[{}], caller:[{}], run_time:[{}]",
                fun_name,
                calling_info.user_id(),
                calling_info.owner_info_str(),
                duration.as_millis()
            )
        },
        Err(e) => {
            SysEvent::task_fault()
                .param(build_str_param!(SysEvent::FUNCTION, "add"))
                .param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
                .param(build_str_param!(SysEvent::CALLER, calling_info.owner_info_str()))
                .param(build_number_param!(SysEvent::ERROR_CODE, e.code as i32))
                .param(build_str_param!(SysEvent::EXTRA, e.msg.clone()))
                .write();
            loge!(
                "[ERROR] use fun:[{}], user_id:[{}], caller:[{}], error_code:[{}], error_msg:[{}]",
                fun_name,
                calling_info.user_id(),
                calling_info.owner_info_str(),
                e.code,
                e.msg.clone()
            );
        },
    }
    result
}
