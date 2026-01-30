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

use std::cmp::min;
use std::time::Instant;

use hisysevent::{
    build_array_params, build_number_param, build_str_param, build_string_array_params, write, EventType,
    HiSysEventParam,
};
use ipc::Skeleton;

use asset_common::CallingInfo;
use asset_definition::{
    Accessibility, AssetError, AssetMap, AuthType, ConflictResolution, IsArray, Extension, OperationType, Result,
    ReturnType, SyncType, Tag, WrapType,
};
use asset_log::{loge, logi};

/// Component name.
const COMPONENT: &str = "asset";
/// Partition name.
pub const PARTITION: &str = "/data";
/// Max asset file dir number.
const MAX_DIR_NUMBER: usize = 7;

/// System events structure which base on `Hisysevent`.
struct SysEvent<'a> {
    event_type: EventType,
    params: Vec<HiSysEventParam<'a>>,
    domain: &'static str,
    event_name: &'static str,
}

impl<'a> SysEvent<'a> {
    const ASSET_DOMAIN: &str = "ASSET";
    const FILEMANAGEMENT_DOMAIN: &str = "FILEMANAGEMENT";
    const ASSET_FAULT: &str = "SECRET_STORE_OPERATION_FAILED";
    const ASSET_STATISTIC: &str = "SECRET_STORE_INFO_COLLECTION";
    const FILEMANAGEMENT_STATISTIC: &str = "USER_DATA_SIZE";

    pub(crate) const FUNCTION: &str = "FUNCTION";
    pub(crate) const USER_ID: &str = "USER_ID";
    pub(crate) const CALLER: &str = "CALLER";
    pub(crate) const ERROR_CODE: &str = "ERROR_CODE";
    pub(crate) const RUN_TIME: &str = "RUN_TIME";
    pub(crate) const EXTRA: &str = "EXTRA";
    pub(crate) const COMPONENT_NAME: &str = "COMPONENT_NAME";
    pub(crate) const PARTITION_NAME: &str = "PARTITION_NAME";
    pub(crate) const REMAIN_PARTITION_SIZE: &str = "REMAIN_PARTITION_SIZE";
    pub(crate) const FILE_OR_FOLDER_PATH: &str = "FILE_OR_FOLDER_PATH";
    pub(crate) const FILE_OR_FOLDER_SIZE: &str = "FILE_OR_FOLDER_SIZE";

    fn new(event_type: EventType, domain: &'static str, event_name: &'static str) -> Self {
        Self { event_type, domain, event_name, params: Vec::new() }
    }

    fn set_param(mut self, param: HiSysEventParam<'a>) -> Self {
        self.params.push(param);
        self
    }

    fn write(self) {
        write(self.domain, self.event_name, self.event_type, self.params.as_slice());
    }
}

const EXTRA_ATTRS: [Tag; 12] = [
    Tag::Alias,
    Tag::SyncType,
    Tag::Accessibility,
    Tag::RequirePasswordSet,
    Tag::AuthType,
    Tag::OperationType,
    Tag::ReturnType,
    Tag::RequireAttrEncrypted,
    Tag::GroupId,
    Tag::WrapType,
    Tag::IsPersistent,
    Tag::ConflictResolution,
];

const ANONYMOUS_SPLIT_PART: usize = 4;

fn anonymous_vec(val: &[u8]) -> String {
    let mut bytes: String = val.iter().map(|&b| format!("{:02x}", b)).collect();
    let start_pos = bytes.len() / ANONYMOUS_SPLIT_PART;
    let end_pos = bytes.len() - start_pos;
    let rep_str = "*".repeat(end_pos - start_pos);
    bytes.replace_range(start_pos..end_pos, &rep_str);
    bytes
}

fn transfer_tag_to_string(tags: &[Tag], attributes: &AssetMap) -> Result<String> {
    let mut ext_info = "".to_string();
    for tag in tags {
        if attributes.get(tag).is_none() {
            continue;
        }
        let tag_value = match tag {
            Tag::Alias => anonymous_vec(attributes.get_bytes_attr(tag).unwrap_or(&vec![])).to_string(),
            Tag::SyncType => format!("{}", attributes.get_num_attr(tag).unwrap_or(SyncType::default() as u32)),
            Tag::Accessibility => format!("{}", attributes.get_enum_attr(tag).unwrap_or(Accessibility::default())),
            Tag::RequirePasswordSet => format!("{}", attributes.get_bool_attr(tag).unwrap_or(false)),
            Tag::AuthType => format!("{}", attributes.get_enum_attr(tag).unwrap_or(AuthType::default())),
            Tag::ReturnType => format!("{}", attributes.get_enum_attr(tag).unwrap_or(ReturnType::default())),
            Tag::RequireAttrEncrypted => format!("{}", attributes.get_bool_attr(tag).unwrap_or(false)),
            Tag::OperationType => {
                format!("{}", attributes.get_num_attr(tag).unwrap_or(OperationType::default() as u32))
            },
            Tag::GroupId => anonymous_vec(attributes.get_bytes_attr(tag).unwrap_or(&vec![])).to_string(),
            Tag::WrapType => format!("{}", attributes.get_enum_attr(tag).unwrap_or(WrapType::default())),
            Tag::IsPersistent => format!("{}", attributes.get_bool_attr(tag).unwrap_or(false)),
            Tag::ConflictResolution => {
                format!("{}", attributes.get_enum_attr(tag).unwrap_or(ConflictResolution::default()))
            }
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
    SysEvent::new(EventType::Statistic, SysEvent::ASSET_DOMAIN, SysEvent::ASSET_STATISTIC)
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
    logi!(
        "[INFO]Calling fun:[{}], user_id:[{}], caller:[{}], start_time:[{:?}], run_time:[{}], ext_info=[{}]",
        func_name,
        calling_info.user_id(),
        owner_info,
        start_time,
        duration.as_millis(),
        ext_info
    )
}

pub(crate) fn upload_fault_system_event(
    calling_info: &CallingInfo,
    start_time: Instant,
    func_name: &str,
    ext_info: &str,
    e: &AssetError,
) {
    let owner_info = String::from_utf8_lossy(calling_info.owner_info()).to_string();
    SysEvent::new(EventType::Fault, SysEvent::ASSET_DOMAIN, SysEvent::ASSET_FAULT)
        .set_param(build_str_param!(SysEvent::FUNCTION, func_name))
        .set_param(build_number_param!(SysEvent::USER_ID, calling_info.user_id()))
        .set_param(build_str_param!(SysEvent::CALLER, owner_info.clone()))
        .set_param(build_number_param!(SysEvent::ERROR_CODE, e.code as i32))
        .set_param(build_str_param!(SysEvent::EXTRA, format!(
            "error code={} error msg={} ext_info={}",
            e.code,
            e.msg.clone(),
            ext_info
        )))
        .write();
    loge!(
        "[ERROR]Calling fun:[{}], user_id:[{}], caller:[{}], start_time:[{:?}], code:[{}], msg:[{}], ext_info:[{}]",
        func_name,
        calling_info.user_id(),
        owner_info,
        start_time,
        e.code,
        e.msg.clone(),
        ext_info
    );
}

pub(crate) fn upload_system_event<T: IsArray>(
    result: Result<T>,
    calling_info: &CallingInfo,
    start_time: Instant,
    func_name: &str,
    attributes: &AssetMap,
) -> Result<T> {
    let mut ext_info = construct_ext_info(attributes)?;
    match &result {
        Ok(val) => {
            if val.is_array() {
                ext_info += &format!("res count:{};", val.array_len());
            }
            upload_statistic_system_event(calling_info, start_time, func_name, &ext_info)
        },
        Err(e) => upload_fault_system_event(calling_info, start_time, func_name, &ext_info, e),
    }
    result
}

/// upload data size
pub(crate) fn upload_data_size(
    remain_partition_size: f64,
    file_or_folder_path: Vec<String>,
    file_or_folder_size: Vec<u64>,
) {
    let folder_path: Vec<&str> = file_or_folder_path.iter().map(|s| s.as_str()).collect();
    let mut folder_size = [0u64; MAX_DIR_NUMBER];
    let min_size = min(MAX_DIR_NUMBER, folder_path.len());
    folder_size[..min_size].copy_from_slice(&file_or_folder_size[..min_size]);

    SysEvent::new(EventType::Statistic, SysEvent::FILEMANAGEMENT_DOMAIN, SysEvent::FILEMANAGEMENT_STATISTIC)
        .set_param(build_str_param!(SysEvent::COMPONENT_NAME, COMPONENT))
        .set_param(build_str_param!(SysEvent::PARTITION_NAME, PARTITION))
        .set_param(build_number_param!(SysEvent::REMAIN_PARTITION_SIZE, remain_partition_size))
        .set_param(build_string_array_params!(SysEvent::FILE_OR_FOLDER_PATH, &folder_path))
        .set_param(build_array_params!(SysEvent::FILE_OR_FOLDER_SIZE, &folder_size))
        .write();
}
