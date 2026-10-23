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

//! This module is used to provide common capabilities for the Asset operations.

use asset_common::{CallingInfo, OwnerType, TaskManager, OWNER_INFO_SEPARATOR};
use asset_db_operator::database_file_upgrade::construct_splited_db_name;
use asset_db_operator::database_util::construct_hap_owner_info;
use asset_db_operator::types::{column, DbMap};
use asset_definition::{macros_lib, AssetMap, ErrCode, OperationType, Result, SyncType, Tag, Value};
use asset_file_operator::common::{BACKUP_SUFFIX, CE_ROOT_PATH, DB_SUFFIX, DE_ROOT_PATH};
use asset_log::{loge, logi, logw};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_plugin_interface::plugin_interface::{
    EventType, ExtDbMap, PARAM_NAME_BUNDLE_NAME, PARAM_NAME_OWNER_INFO, PARAM_NAME_USER_ID,
};

use std::collections::HashSet;
use std::fs;

use crate::sys_event::PARTITION;

pub(crate) fn inform_asset_ext(calling_info: &CallingInfo, input: &AssetMap) {
    if let Some(Value::Number(operation_type)) = input.get(&Tag::OperationType) {
        match operation_type {
            x if *x == OperationType::NeedSync as u32 => {
                if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
                    let owner_info_str = String::from_utf8_lossy(calling_info.owner_info()).to_string();
                    let owner_info_vec: Vec<_> = owner_info_str.split(OWNER_INFO_SEPARATOR).collect();
                    let caller_name = owner_info_vec[0];
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
                    params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(caller_name.as_bytes().to_vec()));
                    match load.process_event(EventType::Sync, &mut params) {
                        Ok(()) => logi!("process sync ext event success."),
                        Err(code) => loge!("process sync ext event failed, code: {}", code),
                    }
                }
            },
            x if *x == OperationType::NeedLogout as u32 => {
                if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
                    let owner_info_str = String::from_utf8_lossy(calling_info.owner_info()).to_string();
                    let owner_info_vec: Vec<_> = owner_info_str.split(OWNER_INFO_SEPARATOR).collect();
                    let caller_name = owner_info_vec[0];
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
                    params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(caller_name.as_bytes().to_vec()));
                    match load.process_event(EventType::CleanCloudFlag, &mut params) {
                        Ok(()) => logi!("process clean cloud flag ext event success."),
                        Err(code) => loge!("process clean cloud flag ext event failed, code: {}", code),
                    }
                }
            },
            x if *x == OperationType::NeedDeleteCloudData as u32 => {
                if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
                    let mut params = ExtDbMap::new();
                    params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
                    params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(calling_info.owner_info().clone()));
                    match load.process_event(EventType::DeleteCloudData, &mut params) {
                        Ok(()) => logi!("process delete cloud data ext event success."),
                        Err(code) => loge!("process delete cloud data ext event failed, code: {}", code),
                    }
                }
            },
            _ => {},
        }
    }
}

pub(crate) fn check_group_validity(attrs: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if attrs.get(&Tag::GroupId).is_some() {
        if let Some(Value::Bool(true)) = attrs.get(&Tag::IsPersistent) {
            let load = AssetPlugin::get_instance()
                .load_plugin()
                .map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?;
            let mut params = ExtDbMap::new();
            params.insert(PARAM_NAME_USER_ID, Value::Number(calling_info.user_id() as u32));
            if load.process_event(EventType::IsPermissionEnabled, &mut params).is_err() {
                return macros_lib::log_throw_error!(
                    macros_lib::hisysevent::function!(),
                    ErrCode::InvalidArgument,
                    "[FATAL]The value of the tag [{}] cannot be set to true when the tag [{}] is specified.",
                    &Tag::IsPersistent,
                    &Tag::GroupId
                );
            }
        }
        if calling_info.owner_type_enum() == OwnerType::Native {
            return macros_lib::log_throw_error!(
                macros_lib::hisysevent::function!(),
                ErrCode::Unsupported,
                "[FATAL]The tag [{}] is not yet supported for [{}] owner.",
                &Tag::GroupId,
                OwnerType::Native
            );
        }
        if calling_info.app_index() > 0 {
            return macros_lib::log_throw_error!(
                macros_lib::hisysevent::function!(),
                ErrCode::Unsupported,
                "[FATAL]The tag [{}] is not yet supported for clone or sandbox app.",
                &Tag::GroupId
            );
        }
    }
    Ok(())
}

pub(crate) fn update_cloud_sync_status(calling_info: &CallingInfo, db_map_vec: &Vec<DbMap>) {
    let mut need_update: bool = false;
    let trusted_account = SyncType::TrustedAccount as u32;
    for db_map in db_map_vec {
        let sync_type = match db_map.get(column::SYNC_TYPE) {
            Some(Value::Number(value)) => *value,
            _ => 0,
        };
        if sync_type & trusted_account == trusted_account {
            need_update = true;
            break;
        }
    }
    if !need_update {
        return;
    }

    let user_id = calling_info.user_id();
    let owner_info = calling_info.owner_info().clone();
    let handle = ylong_runtime::spawn_blocking(move || {
        if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
            let mut params = ExtDbMap::new();
            params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
            params.insert(PARAM_NAME_OWNER_INFO, Value::Bytes(owner_info));
            match load.process_event(EventType::UpdateCloudSyncStatus, &mut params) {
                Ok(()) => logi!("process update_cloud_sync_status ext event success."),
                Err(code) => loge!("process update_cloud_sync_status ext event failed, code: {}", code),
            }
        }
    });
    let task_manager = TaskManager::get_instance();
    task_manager.lock().unwrap().push_task(handle);
}

fn get_default_value(tag: &Tag) -> Result<Value> {
    match tag {
        Tag::RequireAttrEncrypted => Ok(Value::Bool(false)),
        Tag::GroupId => Ok(Value::Bytes(vec![])),
        // Add other Tags if needed.
        _ => macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::InvalidArgument,
            "[FATAL][OPERATIONS]Tag {:?} does not have default value",
            tag
        ),
    }
}

pub(crate) fn check_tags_consistency(tags: &[Tag], attributes_array: &[AssetMap]) -> Result<()> {
    let first = &attributes_array[0];

    for tag in tags {
        let ref_value = match first.get(tag) {
            Some(value) => value.clone(),
            None => {
                get_default_value(tag).map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?
            },
        };

        for (idx, attrs) in attributes_array.iter().enumerate() {
            let value = match attrs.get(tag) {
                Some(value) => value.clone(),
                None => get_default_value(tag)
                    .map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?,
            };

            if ref_value != value {
                return macros_lib::log_throw_error!(
                    macros_lib::hisysevent::function!(),
                    ErrCode::InconsistentAttribute,
                    "[FATAL][OPERATIONS]Tag {:?} at index {} has inconsistent value",
                    tag,
                    idx
                );
            }
        }
    }

    Ok(())
}

/// Threshold for free storage space (MB).
pub(crate) const FREE_SPACE_THRESHOLD_MB: f64 = 100.0;

/// Threshold for owner data size (bytes).
pub(crate) const OWNER_DATA_SIZE_THRESHOLD_BYTES: u64 = 300 * 1024 * 1024;

/// Check storage thresholds. Returns ErrCode::DatabaseError if:
///   1. /data free space < FREE_SPACE_THRESHOLD_MB, or
///   2. owner's data size (DE + CE + group dbs) > OWNER_DATA_SIZE_THRESHOLD_BYTES.
/// Must be called after parameter validation, before any DB write.
pub(crate) fn check_storage_threshold(calling_info: &CallingInfo) -> Result<()> {
    let remain_mb = crate::data_size_mod::get_remain_partition_size(PARTITION)
        .map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?;
    if remain_mb < FREE_SPACE_THRESHOLD_MB {
        let result = macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::DatabaseError,
            "insufficient storage space, remain={:.2}MB, threshold={:.2}MB",
            remain_mb,
            FREE_SPACE_THRESHOLD_MB
        );
        if let Err(ref e) = result {
            crate::sys_event::upload_fault_system_event(
                calling_info,
                std::time::Instant::now(),
                "check_storage_space",
                &format!("remain={:.2}MB, threshold={:.2}MB", remain_mb, FREE_SPACE_THRESHOLD_MB),
                e,
                &mut ExtDbMap::new(),
            );
        }
        return result;
    }

    let total = compute_owner_data_size(calling_info)
        .map_err(|e| macros_lib::track_error!(e, macros_lib::hisysevent::function!()))?;
    if total > OWNER_DATA_SIZE_THRESHOLD_BYTES {
        let result = macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::DatabaseError,
            "owner data size exceeds threshold, total={} bytes, threshold={} bytes",
            total,
            OWNER_DATA_SIZE_THRESHOLD_BYTES
        );
        if let Err(ref e) = result {
            crate::sys_event::upload_fault_system_event(
                calling_info,
                std::time::Instant::now(),
                "check_owner_data_size",
                &format!("total={} bytes, threshold={} bytes", total, OWNER_DATA_SIZE_THRESHOLD_BYTES),
                e,
                &mut ExtDbMap::new(),
            );
        }
        return result;
    }

    Ok(())
}

fn compute_owner_data_size(calling_info: &CallingInfo) -> Result<u64> {
    let user_id = calling_info.user_id();

    // own_db_file 始终用 normal 的 db 文件名（含 .db 后缀）
    // 群组 db 由 group_prefix（Group_{dev_id}_）匹配，不需要单独作为 own_db_file
    let (de_db_file, ce_db_file) = match calling_info.owner_type_enum() {
        OwnerType::Hap | OwnerType::HapGroup => get_hap_db_file_names(calling_info.owner_info())?,
        OwnerType::Native => {
            let name = construct_splited_db_name(calling_info, false)?;
            (format!("{}{}", name, DB_SUFFIX), format!("enc_{}{}", name, DB_SUFFIX))
        },
    };

    let mut total: u64 = 0;

    let de_dir = format!("/{}/{}", DE_ROOT_PATH, user_id);
    total += sum_owner_dbs_in_dir(&de_dir, &de_db_file, calling_info.hap_developer_id(), false)?;

    let ce_dir = format!("/{}/{}/asset_service", CE_ROOT_PATH, user_id);
    total += sum_owner_dbs_in_dir(&ce_dir, &ce_db_file, calling_info.hap_developer_id(), true)?;

    Ok(total)
}

/// 构造该应用自身的普通 HAP 数据库文件名（含 .db 后缀）。
/// 直接调用 construct_hap_owner_info，不走 construct_splited_db_name 的 owner_type 分支。
fn get_hap_db_file_names(owner_info: &[u8]) -> Result<(String, String)> {
    let hap_name = construct_hap_owner_info(owner_info)?;
    Ok((format!("{}{}", hap_name, DB_SUFFIX), format!("enc_{}{}", hap_name, DB_SUFFIX)))
}

fn sum_owner_dbs_in_dir(dir: &str, own_db_file: &str, developer_id: Option<&Vec<u8>>, is_ce: bool) -> Result<u64> {
    let mut total: u64 = 0;
    let mut counted: HashSet<String> = HashSet::new();

    let group_prefix = match developer_id {
        Some(dev_id) => {
            let dev_id_str = String::from_utf8_lossy(dev_id).trim_end_matches('\0').to_string();
            let prefix = if is_ce { "enc_Group_" } else { "Group_" };
            Some(format!("{}{}_", prefix, dev_id_str))
        },
        None => None,
    };

    let entries = match fs::read_dir(dir) {
        Ok(e) => e,
        Err(_) => {
            logw!("sum_owner_dbs_in_dir: failed to read dir {}, skipping", dir);
            return Ok(0);
        },
    };
    for entry in entries {
        let entry = match entry {
            Ok(e) => e,
            Err(_) => continue,
        };
        let path = entry.path();
        let file_name = match path.file_name().map(|s| s.to_string_lossy().to_string()) {
            Some(n) => n,
            None => continue,
        };

        if file_name.ends_with(BACKUP_SUFFIX) || !file_name.ends_with(DB_SUFFIX) {
            continue;
        }

        let matched = file_name == own_db_file || group_prefix.as_ref().map_or(false, |p| file_name.starts_with(p));

        if matched {
            let path_str = path.to_string_lossy().to_string();
            if counted.insert(path_str) {
                if let Ok(meta) = fs::metadata(&path) {
                    total = total.saturating_add(meta.len());
                }
            }
        }
    }

    Ok(total)
}
