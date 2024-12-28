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

//! This module is used to Asset service hisysevent.

use std::time::Instant;

use ipc::Skeleton;

use asset_common::CallingInfo;
use asset_definition::{
    Accessibility, AssetError, AssetMap, AuthType, Extension, OperationType, Result, ReturnType, SyncType, Tag,
};
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

const EXTRA_ATTRS: [Tag; 7] = [
    Tag::SyncType,
    Tag::Accessibility,
    Tag::RequirePasswordSet,
    Tag::AuthType,
    Tag::OperationType,
    Tag::ReturnType,
    Tag::RequireAttrEncrypted,
];

fn transfer_tag_to_string(tags: &[Tag], attributes: &AssetMap) -> Result<String> {
    let mut ext_info = "".to_string();
    for tag in tags {
        if attributes.get(tag).is_none() {
            continue;
        }
        let tag_value = match tag {
            Tag::SyncType => format!("{}", attributes.get_num_attr(tag).unwrap_or(SyncType::default() as u32)),
            Tag::Accessibility => format!("{}", attributes.get_enum_attr(tag).unwrap_or(Accessibility::default())),
            Tag::RequirePasswordSet => format!("{}", attributes.get_bool_attr(tag).unwrap_or(false)),
            Tag::AuthType => format!("{}", attributes.get_enum_attr(tag).unwrap_or(AuthType::default())),
            Tag::ReturnType => format!("{}", attributes.get_enum_attr(tag).unwrap_or(ReturnType::default())),
            Tag::RequireAttrEncrypted => format!("{}", attributes.get_bool_attr(tag).unwrap_or(false)),
            Tag::OperationType => {
                format!("{}", attributes.get_num_attr(tag).unwrap_or(OperationType::default() as u32))
            },
            _ => String::new(),
        };
        ext_info += &format!("{}:{};", tag, tag_value);
    }
    Ok(ext_info)
}

fn construct_ext_info(attributes: &AssetMap) -> Result<String> {
    let tags = EXTRA_ATTRS.to_vec();
    transfer_tag_to_string(&tags, attributes)
}

pub(crate) fn upload_statistic_system_event(
    calling_info: &CallingInfo,
    start_time: Instant,
    func_name: &str,
    ext_info: &str,
) {
    let duration = start_time.elapsed();
    let owner_info = String::from_utf8_lossy(calling_info.owner_info()).to_string();
    SysEvent::new(EventType::Statistic)
        .set_param(build_str_param!(SysEvent::FUNCTION, func_name))
        .set_param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
        .set_param(build_str_param!(SysEvent::CALLER, owner_info.clone()))
        .set_param(build_number_param!(SysEvent::RUN_TIME, duration.as_millis() as u32))
        .set_param(build_str_param!(
            SysEvent::EXTRA,
            format!(
                "CallingUid={} ext_info={} caller_owner_type={}",
                Skeleton::calling_uid(),
                ext_info,
                calling_info.owner_type()
            )
        ))
        .write();
    logi!("[INFO]CallingUid=[{}] ext_info=[{}]", Skeleton::calling_uid(), ext_info);
    logi!(
        "[INFO]Calling fun:[{}], user_id:[{}], caller:[{}], start_time:[{:?}], run_time:[{}]",
        func_name,
        calling_info.user_id(),
        owner_info,
        start_time,
        duration.as_millis()
    )
}

pub(crate) fn upload_fault_system_event(
    calling_info: &CallingInfo,
    start_time: Instant,
    func_name: &str,
    e: &AssetError,
) {
    let owner_info = String::from_utf8_lossy(calling_info.owner_info()).to_string();
    SysEvent::new(EventType::Fault)
        .set_param(build_str_param!(SysEvent::FUNCTION, func_name))
        .set_param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
        .set_param(build_str_param!(SysEvent::CALLER, owner_info.clone()))
        .set_param(build_number_param!(SysEvent::ERROR_CODE, e.code as i32))
        .set_param(build_str_param!(SysEvent::EXTRA, e.msg.clone()))
        .write();
    loge!(
        "[ERROR]Calling fun:[{}], user_id:[{}], caller:[{}], start_time:[{:?}], error_code:[{}], error_msg:[{}]",
        func_name,
        calling_info.user_id(),
        owner_info,
        start_time,
        e.code,
        e.msg.clone()
    );
}

pub(crate) fn upload_system_event<T>(
    result: Result<T>,
    calling_info: &CallingInfo,
    start_time: Instant,
    func_name: &str,
    attributes: &AssetMap,
) -> Result<T> {
    let ext_info = construct_ext_info(attributes)?;
    match &result {
        Ok(_) => upload_statistic_system_event(calling_info, start_time, func_name, &ext_info),
        Err(e) => upload_fault_system_event(calling_info, start_time, func_name, e),
    }
    result
}
