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

//! This module is used to subscribe common event and system ability.

use std::{
    ffi::{CStr, CString},
    fs::{self, DirEntry},
    os::raw::c_char,
    os::unix::fs::PermissionsExt,
    path::Path,
    slice,
    sync::Mutex,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

use lazy_static::lazy_static;

use asset_common::{
    AutoCounter, CallingInfo, ConstAssetBlob, ConstAssetBlobArray, GROUP_SEPARATOR, Group, ModifyAssetBlob,
    MutAssetBlobArray, OwnerType
};
use asset_crypto_manager::{crypto_manager::CryptoManager, secret_key::SecretKey,
    db_key_operator::{DbKey, get_db_key}};
use asset_db_operator::{
    database::Database,
    database_file_upgrade::{
        construct_splited_db_name, trigger_db_upgrade, update_upgrade_list,
    },
    keepdata_cache::{
        add_keepdata_entry, get_keepdata_entries_by_bundle, get_keepdata_entries_by_developer,
        remove_keepdata_entries, remove_keepdata_entry, remove_keepdata_group_ids,
        KeepdataEntry,
    },
    types::{
        column::{self},
        DbMap,
    },
};
use asset_definition::{macros_lib, ErrCode, Result, SyncType, Value, AssetError};
use asset_file_operator::{
    ce_operator::is_db_key_cipher_file_exist,
    common::{BACKUP_SUFFIX, CE_ROOT_PATH, DB_SUFFIX, DE_ROOT_PATH},
    de_operator::delete_user_de_dir,
};
use asset_log::{loge, logi, logw};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_plugin_interface::plugin_interface::{
    EventType, ExtDbMap, PARAM_NAME_APP_INDEX, PARAM_NAME_BUNDLE_NAME,
    PARAM_NAME_IS_HAP, PARAM_NAME_USER_ID, PARAM_NAME_OWNER_INFO
};

use crate::data_size_mod::handle_data_size_upload;
use crate::sys_event::{report_op_result, upload_fault_system_event, upload_statistic_system_event};
use crate::{PackageInfoFfi, upgrade_operator, upgrade_ce, ASSET_SERVICE};
use crate::usage_statistics;

/// success code.
const SUCCESS: i32 = 0;
const USER_ID_VEC_BUFFER: u32 = 5;
const MINIMUM_MAIN_USER_ID: i32 = 100;
const TWELVE_HOURS_AS_SECS: u64 = 3600 * 12;

enum DataExist {
    OwnerData(bool),
    GroupData(bool),
}

extern "C" {
    fn GetUninstallGroups(userId: i32, developer_id: *const ConstAssetBlob, group_ids: *mut MutAssetBlobArray) -> i32;
}

lazy_static! {
    static ref LAST_TRIGGER_TIME_FILE_MUTEX: Mutex<()> = Mutex::new(());
}

fn remove_db(file_path: &str, calling_info: &CallingInfo, is_ce: bool) -> Result<()> {
    let db_name = construct_splited_db_name(calling_info, is_ce)?;
    for db_path in fs::read_dir(file_path)? {
        let db_path = db_path?;
        let db_file_name = db_path.file_name().to_string_lossy().to_string();
        let origin_db_name = format!("{}{}", db_name, DB_SUFFIX);
        let backup_db_name = format!("{}{}", origin_db_name, BACKUP_SUFFIX);
        if db_file_name == origin_db_name || db_file_name == backup_db_name {
            match fs::remove_file(&db_path.path().to_string_lossy().to_string()) {
                Ok(_) => (),
                Err(e) => {
                    logw!("[WARNING]Remove db:[{}] failed, error code:[{}]", db_file_name, e);
                },
            }
        }
    }
    Ok(())
}

fn delete_in_de_db_on_package_removed(calling_info: &CallingInfo, reverse_condition: &DbMap) -> Result<DataExist> {
    let mut db = Database::build(calling_info, None)?;
    let mut delete_condition = DbMap::new();
    let check_condition = DbMap::new();
    delete_condition.insert(column::IS_PERSISTENT, Value::Bool(false));
    let _ = db.delete_datas(&delete_condition, Some(reverse_condition), false)?;
    let data_exists = db.is_data_exists(&check_condition, false)?;
    if !data_exists {
        remove_db(&format!("{}/{}", DE_ROOT_PATH, calling_info.user_id()), calling_info, false)?;
    }
    match calling_info.group() {
        Some(_) => {
            Ok(DataExist::GroupData(data_exists))
        },
        None => {
            Ok(DataExist::OwnerData(data_exists))
        },
    }
}

fn delete_in_ce_db_on_package_removed(calling_info: &CallingInfo, reverse_condition: &DbMap) -> Result<DataExist> {
    let db_key = get_db_key(calling_info.user_id(), true)?;
    let mut db = Database::build(calling_info, db_key)?;
    let mut delete_condition = DbMap::new();
    let check_condition = DbMap::new();
    delete_condition.insert(column::IS_PERSISTENT, Value::Bool(false));
    let _ = db.delete_datas(&delete_condition, Some(reverse_condition), false)?;
    let data_exists = db.is_data_exists(&check_condition, false)?;
    if !data_exists {
        remove_db(&format!("{}/{}/{}", CE_ROOT_PATH, calling_info.user_id(), ASSET_SERVICE), calling_info, true)?;
    }
    match calling_info.group() {
        Some(_) => {
            Ok(DataExist::GroupData(data_exists))
        },
        None => {
            Ok(DataExist::OwnerData(data_exists))
        },
    }
}

fn delete_on_package_removed(calling_info: &CallingInfo) -> Result<DataExist> {
    let mut reverse_condition = DbMap::new();
    reverse_condition.insert(column::SYNC_TYPE, Value::Number(SyncType::TrustedAccount as u32));
    let de_db_data_exists = delete_in_de_db_on_package_removed(calling_info, &reverse_condition)?;

    if is_db_key_cipher_file_exist(calling_info.user_id())? {
        let ce_db_data_exists = delete_in_ce_db_on_package_removed(calling_info, &reverse_condition)?;
        match (de_db_data_exists, ce_db_data_exists) {
            (DataExist::OwnerData(de_db_data_exists), DataExist::OwnerData(ce_db_data_exists)) => {
                Ok(DataExist::OwnerData(de_db_data_exists || ce_db_data_exists))
            },
            (DataExist::GroupData(de_db_data_exists), DataExist::GroupData(ce_db_data_exists)) => {
                Ok(DataExist::GroupData(de_db_data_exists || ce_db_data_exists))
            },
            _ => macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
                ErrCode::AccessDenied, "[FATAL][SA]Cannot delete owner and group data at same time"),
        }
    } else {
        Ok(de_db_data_exists)
    }
}

fn clear_cryptos(calling_info: &CallingInfo) {
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_by_calling_info(calling_info);
}

fn construct_calling_infos(
    user_id: i32,
    owner: Vec<u8>,
    developer_id: ConstAssetBlob,
    group_ids: ConstAssetBlobArray,
) -> Vec<CallingInfo> {
    let mut calling_infos = vec![CallingInfo::new(user_id, OwnerType::Hap, owner.clone(), None)];
    if !group_ids.blobs.is_null() && group_ids.size != 0 && !developer_id.data.is_null() && developer_id.size != 0 {
        let group_ids_slice = unsafe { slice::from_raw_parts(group_ids.blobs, group_ids.size as usize) };
        let mut blobs = Vec::new();
        let mut group_id_blobs = Vec::new();
        for group_id_slice in group_ids_slice {
            let data = unsafe { slice::from_raw_parts(group_id_slice.data, group_id_slice.size as usize) };
            let data_str = CString::new(String::from_utf8_lossy(data).to_string()).unwrap();
            group_id_blobs.push(ModifyAssetBlob { modify: false, blob: data_str.as_ptr() });
            blobs.push(data_str);
        }

        let mut the_group_ids = MutAssetBlobArray {
            size: group_id_blobs.len() as u32, blobs: group_id_blobs.as_mut_ptr()
        };
        match unsafe { GetUninstallGroups(user_id, &developer_id, &mut the_group_ids) } {
            SUCCESS => (),
            error => {
                loge!("[FATAL]Get GetUninstallGroups failed, res is {}.", error);
                return calling_infos;
            },
        }

        let the_group_ids_slice = unsafe { slice::from_raw_parts(the_group_ids.blobs, the_group_ids.size as usize) };
        let developer_id = unsafe { slice::from_raw_parts(developer_id.data, developer_id.size as usize) };
        let developer_id_string = String::from_utf8_lossy(developer_id);
        let mut main_developer_id = developer_id.to_vec().clone();
        if let Some(pos) = developer_id_string.find('.') {
            main_developer_id.drain(..=pos);
        }
        for group_id_slice in the_group_ids_slice {
            if group_id_slice.modify {
                continue;
            }
            let group_id = unsafe { CStr::from_ptr(group_id_slice.blob) };
            calling_infos.push(CallingInfo::new(
                user_id,
                OwnerType::HapGroup,
                owner.clone(),
                Some(Group { developer_id: main_developer_id.clone(), group_id: group_id.to_bytes().to_vec() }),
            ));
        }
    }
    calling_infos
}

/// Check whether the group is retained by a keepdata marker: any marker entry
/// of the developer whose group ids cover the group means an earlier uninstall
/// of the group chose to retain the data (spec R-3, AC-1.3).
fn is_group_retained_by_keepdata(
    user_id: i32,
    developer_id: &Option<String>,
    group_id: Option<&Vec<u8>>,
) -> bool {
    let Some(developer_id) = developer_id else { return false };
    let Some(group_id) = group_id else { return false };
    let group_id_str = String::from_utf8_lossy(group_id).to_string();
    let entries = get_keepdata_entries_by_developer(user_id, developer_id);
    logi!(
        "Check group retention: developer:[{}], group:[{}], matched entries:[{}], entries:[{:?}]",
        developer_id,
        group_id_str,
        entries.len(),
        entries
    );
    entries
        .iter()
        .any(|entry| entry.group_ids.iter().any(|id| *id == group_id_str))
}

fn delete_data_by_owner(
    user_id: i32,
    owner: ConstAssetBlob,
    developer_id: ConstAssetBlob,
    group_ids: ConstAssetBlobArray,
) {
    let _counter_user = AutoCounter::new();
    let start_time = Instant::now();
    let owner: Vec<u8> = unsafe { slice::from_raw_parts(owner.data, owner.size as usize).to_vec() };
    let developer_id_str: Option<String> = if developer_id.size != 0 && !developer_id.data.is_null() {
        Some(String::from_utf8_lossy(unsafe {
            slice::from_raw_parts(developer_id.data, developer_id.size as usize)
        }).to_string())
    } else {
        None
    };

    for calling_info in construct_calling_infos(user_id, owner.clone(), developer_id, group_ids) {
        clear_cryptos(&calling_info);
        // Skip the deletion of a group retained by a keepdata marker: an
        // earlier uninstall of the group chose to retain the data (spec R-3).
        // NOTE: group_id() is the bare group id; group() is the concatenated
        // "developer_id<sep>group_id" form and must not be used here.
        if is_group_retained_by_keepdata(user_id, &developer_id_str, calling_info.group_id()) {
            logi!("Group data is retained by a keepdata marker, skip the group deletion.");
            continue;
        }
        let res = match delete_on_package_removed(&calling_info) {
            Ok(DataExist::OwnerData(true)) => {
                logi!("Data remain in owner db after uninstallation. Do not delete owner key in HUKS.");
                Ok(())
            },
            Ok(DataExist::OwnerData(false)) => {
                logi!("No data remain in owner db after uninstallation. Delete owner key in HUKS.");
                SecretKey::delete_by_owner(&calling_info)
            },
            Ok(DataExist::GroupData(true)) => {
                logi!("Other owners' data remain in group db after uninstallation. Do not delete group key in HUKS.");
                Ok(())
            },
            Ok(DataExist::GroupData(false)) => {
                logi!("No data remain in group db after uninstallation. Delete group key in HUKS.");
                SecretKey::delete_by_owner(&calling_info)
            },
            Err(e) => {
                // Report the database operation fault event.
                upload_fault_system_event(&calling_info, start_time, "on_package_removed", "", &e,
                    &mut ExtDbMap::new());
                Ok(())
            },
        };
        if let Err(e) = res {
            // Report the key operation fault event.
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, start_time, "on_package_removed", "", &e, &mut ExtDbMap::new());
        }
    }
}

/// Safely copy an FFI blob into a Vec, tolerating a null data pointer (the
/// C side may hand over null blobs for absent optional fields).
fn blob_to_vec(blob: &ConstAssetBlob) -> Vec<u8> {
    if blob.data.is_null() || blob.size == 0 {
        return Vec::new();
    }
    unsafe { slice::from_raw_parts(blob.data, blob.size as usize).to_vec() }
}

/// Retain the critical assets on a keepdata uninstall (PACKAGE_REMOVED with
/// keepdata=true): record the keepdata marker and skip the baseline deletion
/// chain (db data, HUKS keys and plugin notification), while the in-memory
/// crypto sessions and the upgrade list are still handled as in the baseline
/// path (design ADR-6).
fn retain_data_on_keepdata_uninstall(package_info: PackageInfoFfi) {
    let bundle_name: Vec<u8> = blob_to_vec(&package_info.bundle_name);
    logi!(
        "On app -{}-{}-{}- removed with keepdata, retain the critical assets.",
        package_info.user_id,
        String::from_utf8_lossy(&bundle_name),
        package_info.app_index
    );

    let owner: Vec<u8> = blob_to_vec(&package_info.owner);
    let developer_id: String =
        if package_info.developer_id.size > 0 && !package_info.developer_id.data.is_null() {
            String::from_utf8_lossy(&blob_to_vec(&package_info.developer_id)).to_string()
        } else {
            String::new()
        };
    let group_ids: Vec<String> =
        if package_info.group_ids.size > 0 && !package_info.group_ids.blobs.is_null() {
            (0..package_info.group_ids.size as usize)
                .map(|i| {
                    let blob = unsafe { package_info.group_ids.blobs.add(i) };
                    String::from_utf8_lossy(&blob_to_vec(unsafe { &*blob })).to_string()
                })
                .collect()
        } else {
            Vec::new()
        };

    for calling_info in construct_calling_infos(
        package_info.user_id,
        owner.clone(),
        package_info.developer_id,
        package_info.group_ids,
    ) {
        clear_cryptos(&calling_info);
    }

    if package_info.app_index == 0 {
        let _ = update_upgrade_list(package_info.user_id, &String::from_utf8_lossy(&bundle_name).to_string());
    }

    // Record the keepdata marker (best-effort): developer_id and group_ids are
    // recorded only for the main app (app_index == 0), serving as the group
    // retention credential (spec R-2). The owner string is "appId_appIndex",
    // so the app id is the part before the app index suffix.
    let owner_str = String::from_utf8_lossy(&owner).to_string();
    let app_id = match owner_str.strip_suffix(&format!("_{}", package_info.app_index)) {
        Some(app_id) => app_id.to_string(),
        None => owner_str.clone(),
    };
    let entry = KeepdataEntry {
        bundle_name: String::from_utf8_lossy(&bundle_name).to_string(),
        app_id,
        app_index: package_info.app_index,
        developer_id: if package_info.app_index == 0 { developer_id.clone() } else { String::new() },
        group_ids: if package_info.app_index == 0 { group_ids.clone() } else { Vec::new() },
    };
    if let Err(e) = add_keepdata_entry(package_info.user_id, &entry) {
        loge!("Write keepdata entry failed, code:{}, msg:{}", e.code as u32, e.msg);
    }
}

pub(crate) extern "C" fn on_package_removed(package_info: PackageInfoFfi) {
    if package_info.keepdata {
        // Retain the critical assets: record the keepdata marker and skip the
        // baseline deletion chain (db data, HUKS keys and plugin notification).
        retain_data_on_keepdata_uninstall(package_info);
        return;
    }

    let bundle_name: Vec<u8> = blob_to_vec(&package_info.bundle_name);

    // Consume the retention mark of THIS app (appId/appIndex) before the
    // deletion: the baseline logic deletes its own data, so its own retention
    // ends ("the next uninstall without keepdata of this hap follows the
    // baseline rules"). Marks of other apps of the same bundle (e.g. clones
    // in their own retention cycle) are not touched (spec R-3).
    let owner_str = String::from_utf8_lossy(&blob_to_vec(&package_info.owner)).to_string();
    let app_id = match owner_str.strip_suffix(&format!("_{}", package_info.app_index)) {
        Some(app_id) => app_id.to_string(),
        None => owner_str.clone(),
    };
    let _ = remove_keepdata_entry(package_info.user_id, &app_id, package_info.app_index);

    delete_data_by_owner(package_info.user_id, package_info.owner, package_info.developer_id, package_info.group_ids);
    logi!(
        "On app -{}-{}-{}- removed.",
        package_info.user_id,
        String::from_utf8_lossy(&bundle_name),
        package_info.app_index
    );

    if package_info.app_index == 0 {
        let _ = update_upgrade_list(package_info.user_id, &String::from_utf8_lossy(&bundle_name).to_string());
    }

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(package_info.user_id as u32));
        params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(bundle_name));

        // only hap package can be removed
        params.insert(PARAM_NAME_IS_HAP, Value::Bool(true));
        params.insert(PARAM_NAME_APP_INDEX, Value::Number(package_info.app_index as u32));
        match load.process_event(EventType::OnPackageClear, &mut params) {
            Ok(()) => logi!("process package remove event success."),
            Err(code) => loge!("process package remove event failed, code: {}", code),
        }
    }
}

/// Handle the deferred cleanup event (PACKAGE_UNINSTALLED_DATA_CLEARED): clean
/// the critical assets retained by an earlier keepdata uninstall, located by
/// the keepdata markers (spec R-4, AC-2.1/AC-2.2).
pub(crate) fn on_uninstall_data_cleared(
    user_id: i32,
    bundle_name: String,
    app_id: Option<String>,
    app_index: Option<i32>,
    developer_id: Option<String>,
    group_ids: Option<Vec<String>>,
) {
    let _counter_user = AutoCounter::new();
    let start_time = Instant::now();
    logi!("On package uninstalled data cleared, bundle name is [{}].", bundle_name);

    // Locate the retained owners of the bundle by the keepdata markers, then
    // scope the cleanup to the app that triggered the event (BMS carries its
    // appId/appIndex): only its own retention entry is consumed, other
    // retained apps of the bundle (e.g. clones in their own retention
    // cycle) keep their data and marks. An empty match is tolerated.
    let entries = get_keepdata_entries_by_bundle(user_id, &bundle_name);
    let targets: Vec<KeepdataEntry> = match (&app_id, app_index) {
        (Some(app_id), Some(app_index)) => entries
            .iter()
            .filter(|e| e.app_id == *app_id && e.app_index == app_index)
            .cloned()
            .collect(),
        _ => entries.clone(),
    };
    if targets.is_empty() {
        logw!("[WARNING]No keepdata entry matched for bundle [{}] app index [{:?}], skip the cleanup.",
            bundle_name, app_index);
        return;
    }

    // Clean the retained data of each owner. The baseline deletion kernel is
    // reused: the IsPersistent=false condition and the TrustedAccount reverse
    // condition already cover the developer-retention exemption (design ADR-4).
    for entry in targets.iter() {
        let owner = format!("{}_{}", entry.app_id, entry.app_index);
        let calling_info = CallingInfo::new(user_id, OwnerType::Hap, owner.into_bytes(), None);
        clear_cryptos(&calling_info);
        let res = match delete_on_package_removed(&calling_info) {
            Ok(DataExist::OwnerData(true)) => {
                logi!("Data remain in owner db after data cleared. Do not delete owner key in HUKS.");
                Ok(())
            },
            Ok(DataExist::OwnerData(false)) => {
                logi!("No data remain in owner db after data cleared. Delete owner key in HUKS.");
                SecretKey::delete_by_owner(&calling_info)
            },
            Ok(_) => Ok(()),
            Err(e) => {
                upload_fault_system_event(&calling_info, start_time, "on_uninstall_data_cleared", "", &e,
                    &mut ExtDbMap::new());
                Ok(())
            },
        };
        if let Err(e) = res {
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, start_time, "on_uninstall_data_cleared", "", &e,
                &mut ExtDbMap::new());
        }
    }
    let _ = remove_keepdata_entries(user_id, &targets);

    // Clean the group data following the baseline uninstall group logic (spec R-5).
    if let (Some(developer_id), Some(group_ids)) = (developer_id.as_ref(), group_ids.as_ref()) {
        let owner = targets
            .iter()
            .find(|entry| entry.app_index == 0)
            .or_else(|| entries.first())
            .map(|entry| format!("{}_{}", entry.app_id, entry.app_index).into_bytes())
            .unwrap_or_default();
        clean_group_data_on_data_cleared(user_id, owner, developer_id, group_ids, start_time);
    }
}

/// FFI entry of PACKAGE_UNINSTALLED_DATA_CLEARED from the C++ subscriber:
/// convert the C string parameters and forward them to the handler.
extern "C" fn on_uninstall_data_cleared_ffi(
    user_id: i32,
    bundle_name: *const c_char,
    app_id: *const c_char,
    app_index: i32,
    developer_id: *const c_char,
    group_ids: *const c_char,
) {
    if bundle_name.is_null() {
        loge!("[FATAL]Uninstall data cleared bundle name is null.");
        return;
    }
    let bundle_name = unsafe { CStr::from_ptr(bundle_name) }.to_string_lossy().to_string();
    let app_id = if app_id.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(app_id) }.to_string_lossy().to_string())
    };
    let app_index = if app_index >= 0 { Some(app_index) } else { None };
    let developer_id = if developer_id.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(developer_id) }.to_string_lossy().to_string())
    };
    let group_ids = if group_ids.is_null() {
        None
    } else {
        let group_ids = unsafe { CStr::from_ptr(group_ids) }.to_string_lossy().to_string();
        Some(group_ids.split(GROUP_SEPARATOR).map(|group_id| group_id.to_string()).collect())
    };
    on_uninstall_data_cleared(user_id, bundle_name, app_id, app_index, developer_id, group_ids);
}

/// Clean the group data of the clear-data event, following the baseline
/// uninstall group logic (spec R-5): a group is cleaned only when no installed
/// app of the developer belongs to it, and the group marks of the queried
/// groups are consumed afterwards.
fn clean_group_data_on_data_cleared(
    user_id: i32,
    owner: Vec<u8>,
    developer_id: &str,
    group_ids: &Vec<String>,
    start_time: Instant,
) {
    let mut blobs = Vec::new();
    let mut group_id_blobs = Vec::new();
    for group_id in group_ids {
        let data_str = match CString::new(group_id.clone()) {
            Ok(data_str) => data_str,
            Err(_) => {
                logw!("[WARNING]Group id contains an interior NUL, skip it.");
                continue;
            },
        };
        group_id_blobs.push(ModifyAssetBlob { modify: false, blob: data_str.as_ptr() });
        blobs.push(data_str);
    }
    let mut the_group_ids =
        MutAssetBlobArray { size: group_id_blobs.len() as u32, blobs: group_id_blobs.as_mut_ptr() };
    let developer_id_blob = ConstAssetBlob { size: developer_id.len() as u32, data: developer_id.as_ptr() };
    match unsafe { GetUninstallGroups(user_id, &developer_id_blob, &mut the_group_ids) } {
        SUCCESS => (),
        error => {
            loge!("[FATAL]Get GetUninstallGroups failed, res is {}.", error);
            return;
        },
    }

    let the_group_ids_slice = unsafe { slice::from_raw_parts(the_group_ids.blobs, the_group_ids.size as usize) };
    let mut main_developer_id = developer_id.as_bytes().to_vec();
    if let Some(pos) = developer_id.find('.') {
        main_developer_id.drain(..=pos);
    }
    for group_id_slice in the_group_ids_slice {
        if group_id_slice.modify {
            // An installed app of the developer still belongs to the group:
            // retain the group data (spec AC-2.3).
            continue;
        }
        let group_id = unsafe { CStr::from_ptr(group_id_slice.blob) };
        let calling_info = CallingInfo::new(
            user_id,
            OwnerType::HapGroup,
            owner.clone(),
            Some(Group { developer_id: main_developer_id.clone(), group_id: group_id.to_bytes().to_vec() }),
        );
        clear_cryptos(&calling_info);
        let res = match delete_on_package_removed(&calling_info) {
            Ok(DataExist::GroupData(true)) => {
                logi!("Data remain in group db after data cleared. Do not delete group key in HUKS.");
                Ok(())
            },
            Ok(DataExist::GroupData(false)) => {
                logi!("No data remain in group db after data cleared. Delete group key in HUKS.");
                SecretKey::delete_by_owner(&calling_info)
            },
            Ok(_) => Ok(()),
            Err(e) => {
                upload_fault_system_event(&calling_info, start_time, "on_uninstall_data_cleared", "", &e,
                    &mut ExtDbMap::new());
                Ok(())
            },
        };
        if let Err(e) = res {
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, start_time, "on_uninstall_data_cleared", "", &e,
                &mut ExtDbMap::new());
        }
    }

    // Consume the group retention marks of the queried groups: the retained
    // groups have their marks cleared (AC-2.3) and the cleaned groups no
    // longer need the credential.
    let _ = remove_keepdata_group_ids(user_id, developer_id, group_ids);
}

extern "C" fn on_user_removed(user_id: i32) {
    let _counter_user = AutoCounter::new();
    let calling_info = CallingInfo::new(user_id, OwnerType::Native, ASSET_SERVICE.as_bytes().to_vec(), None);
    let start = Instant::now();
    if let Err(e) = delete_user_de_dir(user_id) {
        upload_fault_system_event(&calling_info, start, "on_user_removed_delete_de", "", &e,
            &mut ExtDbMap::new());
    }
    notify_on_user_removed(user_id);
}

extern "C" fn delete_crypto_need_unlock() {
    let _counter_user = AutoCounter::new();
    let crypto_manager = CryptoManager::get_instance();
    crypto_manager.lock().unwrap().remove_need_device_unlocked();
}

lazy_static! {
    static ref RECORD_TIME: Mutex<Option<Instant>> = Mutex::new(None);
}

pub(crate) extern "C" fn on_charging() {
    let _counter_user = AutoCounter::new();
    let cur_time = Instant::now();
    logi!("Start backup db.");

    let mut record_time = RECORD_TIME.lock().expect("Failed to lock RECORD_TIME");

    let should_record = match *record_time {
        Some(ref last_time) => cur_time.duration_since(*last_time) > Duration::new(3600, 0),
        None => true,
    };

    if should_record {
        *record_time = Some(cur_time);
        let backup_res = backup_all_db(&cur_time);
        report_op_result(&CallingInfo::new_self(), cur_time, "backup_db", "", &backup_res);
        usage_statistics::collect_all_usage_stats();
    }

    logi!("Finish backup db & usage statistics.");
}

pub(crate) extern "C" fn on_app_restore(user_id: i32, bundle_name: *const u8, app_index: i32) {
    let _counter_user = AutoCounter::new();
    let calling_info = CallingInfo::new(user_id, OwnerType::Native, ASSET_SERVICE.as_bytes().to_vec(), None);
    let start = Instant::now();
    let c_str = unsafe { CStr::from_ptr(bundle_name as _) };
    let bundle_name = match c_str.to_str() {
        Ok(s) => s.to_string(),
        Err(e) => {
            let err = AssetError::new(ErrCode::InvalidArgument, format!("parse bundle name failed: {}", e), "");
            upload_fault_system_event(&calling_info, start, "on_app_restore", "parse bundle name", &err,
                &mut ExtDbMap::new());
            return;
        },
    };
    logi!("On app -{}-{}- restore.", user_id, bundle_name);

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(bundle_name.as_bytes().to_vec()));
        params.insert(PARAM_NAME_APP_INDEX, Value::Number(app_index as u32));
        match load.process_event(EventType::OnAppRestore, &mut params) {
            Ok(()) => logi!("process app restore event success."),
            Err(code) => loge!("process app restore event failed, code: {}", code),
        }
    }
    upload_statistic_system_event(&calling_info, start, "on_app_restore",
        &format!("{}_{}_{}", user_id, bundle_name, app_index), &mut ExtDbMap::new());
}

pub(crate) extern "C" fn on_user_unlocked(user_id: i32) {
    let _counter_user = AutoCounter::new();
    let calling_info = CallingInfo::new(user_id, OwnerType::Native, ASSET_SERVICE.as_bytes().to_vec(), None);
    logi!("On user -{}- unlocked.", user_id);

    // Trigger upgrading de db version and key alias.
    let de_start = Instant::now();
    let de_res = trigger_db_upgrade(user_id, None);
    report_op_result(&calling_info, de_start, "on_user_unlocked_upgrade_de", "", &de_res);

    // Trigger upgrading ce db version and key alias.
    let get_key_start = Instant::now();
    match get_db_key(user_id, true) {
        Ok(db_key) => {
            let ce_start = Instant::now();
            let ce_res = trigger_db_upgrade(user_id, db_key);
            report_op_result(&calling_info, ce_start, "on_user_unlocked_upgrade_ce", "", &ce_res);
        },
        Err(e) => {
            upload_fault_system_event(&calling_info, get_key_start, "on_user_unlocked_get_db_key", "", &e,
                &mut ExtDbMap::new());
        },
    }

    // Upload data size; success path reports internally, only report fault here.
    let data_size_start = Instant::now();
    if let Err(e) = handle_data_size_upload() {
        upload_fault_system_event(&calling_info, data_size_start, "on_user_unlocked_data_size", "", &e,
            &mut ExtDbMap::new());
    }

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        match load.process_event(EventType::OnUserUnlocked, &mut params) {
            Ok(()) => logi!("process user unlocked event success."),
            Err(code) => loge!("process user unlocked event failed, code: {}", code),
        }
    }
    let _ = upgrade_operator::upgrade_clone_app_data(user_id);
    let _ = upgrade_ce::upgrade_ce_data(user_id, "user_unlocked");
    upgrade_ce::summary_upgrade_data_count(user_id);
}

pub(crate) fn notify_on_user_removed(user_id: i32) {
    logi!("On user remove [{}].", user_id);

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        match load.process_event(EventType::OnUserRemoved, &mut params) {
            Ok(()) => logi!("process user removed event success."),
            Err(code) => loge!("process user removed event failed, code: {}", code),
        }
    }
}

pub(crate) extern "C" fn on_user_switched(user_id: i32) {
    let _counter_user = AutoCounter::new();
    logi!("On user switched [{}].", user_id);
    trigger_sync_with_user_id(user_id, true);
}

pub(crate) extern "C" fn on_schedule_wakeup() {
    let _counter_user = AutoCounter::new();
    logi!("On SA wakes up at a scheduled time(36H).");
    trigger_sync();
}

pub(crate) extern "C" fn on_connectivity_change() {
    let _counter_user = AutoCounter::new();
    let _lock = LAST_TRIGGER_TIME_FILE_MUTEX.lock().unwrap();
    let path = format!("{}/last_trigger_time.txt", DE_ROOT_PATH);
    let last_time = read_last_trigger_time(&path).unwrap_or(0);
    let current = SystemTime::now();
    if let Ok(duration) = current.duration_since(UNIX_EPOCH) {
        let current = duration.as_secs();
        if current > last_time && current - last_time >= TWELVE_HOURS_AS_SECS {
            logi!("On SA connectivity change.");
            trigger_sync();
            if write_last_trigger_time(&path, current).is_err() {
                loge!("Write last trigger time failed.");
            }
        }
    }
}

extern "C" {
    fn GetUserIds(userIdsPtr: *mut i32, userIdsSize: *mut u32) -> i32;
    fn GetFirstUnlockUserIds(userIdsPtr: *mut i32, userIdsSize: *mut u32) -> i32;
    fn GetUsersSize(userIdsSize: *mut u32) -> i32;
}

pub(crate) fn get_first_unlock_userids() -> Vec<i32> {
    let mut user_ids_size: u32 = USER_ID_VEC_BUFFER;
    let mut user_ids: Vec<i32> = vec![0i32; user_ids_size as usize];
    let user_ids_size_ptr = &mut user_ids_size;
    let user_ids_ptr = user_ids.as_mut_ptr();
    let ret: i32 = unsafe { GetFirstUnlockUserIds(user_ids_ptr, user_ids_size_ptr) };
    if ret != SUCCESS {
        loge!("[FATAL] Get users IDs failed. Do not sync data.");
        return vec![];
    }

    if user_ids_size < USER_ID_VEC_BUFFER {
        user_ids.truncate(user_ids_size as usize);
    }
    user_ids
}

pub(crate) extern "C" fn on_data_share_ready() {
    let _counter_user = AutoCounter::new();
    // 1. get first unlock dbs.
    let user_ids = get_first_unlock_userids();

    // 2. check and upgrade ce.
    for user_id in &user_ids {
        let _ = upgrade_ce::upgrade_ce_data(*user_id, "data_share");
    }
}

fn read_last_trigger_time(path_str: &str) -> Result<u64> {
    let path: &Path = Path::new(&path_str);
    let _ = fs::set_permissions(path, fs::Permissions::from_mode(0o640));
    let time_str = match fs::read_to_string(path) {
        Ok(time_str) => time_str,
        Err(_) => "0".to_owned(),
    };
    let trim_time = time_str.trim();
    match trim_time.parse::<u64>() {
        Ok(unix_time) => Ok(unix_time),
        Err(_) => {
            eprintln!("[WARNING] Failed to parse time from file. Return 0 as a default");
            Ok(0)
        },
    }
}

fn write_last_trigger_time(path_str: &str, unix_time: u64) -> Result<()> {
    let path: &Path = Path::new(&path_str);
    let time_str = unix_time.to_string();
    fs::write(path, time_str)?;
    let _ = fs::set_permissions(path, fs::Permissions::from_mode(0o640));
    Ok(())
}

fn trigger_sync() {
    let user_ids = get_first_unlock_userids();
    for user_id in &user_ids {
        trigger_sync_with_user_id(*user_id, false);
    }
}

fn trigger_sync_with_user_id(user_id: i32, is_user_switch: bool) {
    let calling_info = CallingInfo::new(user_id, OwnerType::Native, ASSET_SERVICE.as_bytes().to_vec(), None);
    let start = Instant::now();
    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(ASSET_SERVICE.as_bytes().to_vec()));
        if is_user_switch {
            params.insert(PARAM_NAME_OWNER_INFO, Value::Bytes(ASSET_SERVICE.as_bytes().to_vec()));
        }
        match load.process_event(EventType::Sync, &mut params) {
            Ok(()) => logi!("process sync ext event {} success.", user_id),
            Err(code) => loge!("process sync ext event failed, code: {}, user_id: {}", code, user_id),
        }
    }
    upload_statistic_system_event(&calling_info, start, "sync", "", &mut ExtDbMap::new());
}

fn copy_db_to_backup(
    from_path: &str, calling_info: &CallingInfo, start_time: &Instant, func_name: &str,
) {
    let backup_path = format!("{}{}", from_path, BACKUP_SUFFIX);
    if let Err(e) = fs::copy(from_path, backup_path) {
        upload_fault_system_event(calling_info, *start_time, func_name, "copy_db_to_backup", &e.into(),
            &mut ExtDbMap::new());
    }
}

fn backup_de_db_if_accessible(entry: &DirEntry, user_id: i32, start_time: &Instant) -> Result<()> {
    let calling_info = CallingInfo::new(user_id, OwnerType::Native, ASSET_SERVICE.as_bytes().to_vec(), None);
    for db_path in fs::read_dir(entry.path().to_string_lossy().to_string())? {
        let db_path = match db_path {
            Ok(path) => path,
            Err(e) => {
                upload_fault_system_event(&calling_info, *start_time, &format!("backup_de_db_inner_{}", user_id),
                    "db_path?", &e.into(), &mut ExtDbMap::new());
                continue;
            },
        };

        // Only db files are concerned.
        let db_name = db_path.file_name().to_string_lossy().to_string();
        if !db_name.ends_with(DB_SUFFIX) {
            continue;
        }

        let from_path = db_path.path().to_string_lossy().to_string();
        let func_name = format!("backup_de_db_inner_{}_{}", user_id, db_name);

        // Make sure the db is accessible before copying.
        if let Err(e) = Database::check_db_accessible(from_path.clone(), user_id, db_name.clone(), None) {
            upload_fault_system_event(&calling_info, *start_time, &func_name, "check_db_accessible", &e,
                &mut ExtDbMap::new());
            continue;
        }
        copy_db_to_backup(&from_path, &calling_info, start_time, &func_name);
    }
    Ok(())
}

fn backup_ce_db_if_accessible(user_id: i32, start_time: &Instant) -> Result<()> {
    if user_id < MINIMUM_MAIN_USER_ID {
        return Ok(());
    }

    let db_key = DbKey::get_db_key(user_id)?;
    let calling_info = CallingInfo::new(user_id, OwnerType::Native, ASSET_SERVICE.as_bytes().to_vec(), None);
    for db_path in fs::read_dir(format!("{}/{}/{}", CE_ROOT_PATH, user_id, ASSET_SERVICE))? {
        let db_path = match db_path {
            Ok(p) => p,
            Err(e) => {
                upload_fault_system_event(&calling_info, *start_time, &format!("backup_ce_db_inner_{}", user_id),
                    "db_path?", &e.into(), &mut ExtDbMap::new());
                continue;
            },
        };

        // Only db files are concerned.
        let db_name = db_path.file_name().to_string_lossy().to_string();
        if !db_name.ends_with(DB_SUFFIX) {
            continue;
        }

        let from_path = db_path.path().to_string_lossy().to_string();
        let func_name = format!("backup_ce_db_inner_{}_{}", user_id, db_name);

        // Make sure the db is accessible before copying.
        if let Err(e) = Database::check_db_accessible(from_path.clone(), user_id, db_name.clone(),
            Some(&db_key.db_key)) {
            upload_fault_system_event(&calling_info, *start_time, &func_name, "check_db_accessible", &e,
                &mut ExtDbMap::new());
            continue;
        }

        // Copy the db file to its backup path.
        copy_db_to_backup(&from_path, &calling_info, start_time, &func_name);
    }
    Ok(())
}

fn get_all_user_ids() -> Result<Vec<i32>> {
    let mut size: u32 = 0;
    let size_ptr = &mut size;
    let ret = unsafe { GetUsersSize(size_ptr) };
    if ret != SUCCESS {
        return macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
            ErrCode::AccountError, "[FATAL][SA]Get users size failed.");
    }

    let mut user_ids: Vec<i32> = vec![0i32; (*size_ptr + USER_ID_VEC_BUFFER).try_into().unwrap()];
    let user_ids_ptr = user_ids.as_mut_ptr();
    let ret = unsafe { GetUserIds(user_ids_ptr, size_ptr) };
    if ret != SUCCESS {
        return macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
            ErrCode::AccountError, "[FATAL][SA]Get user IDs failed.");
    }

    user_ids.truncate(size as usize);
    Ok(user_ids)
}

fn backup_all_db(start_time: &Instant) -> Result<()> {
    // Backup all de db if accessible.
    for entry in fs::read_dir(DE_ROOT_PATH)? {
        let entry = match entry {
            Ok(e) => e,
            Err(e) => {
                logw!("[WARNING]Read de root entry failed, error code:[{}]", e);
                continue;
            },
        };
        if let Ok(user_id) = entry.file_name().to_string_lossy().to_string().parse::<i32>() {
            if let Err(e) = backup_de_db_if_accessible(&entry, user_id, start_time) {
                upload_fault_system_event(&CallingInfo::new_self(), *start_time,
                    &format!("backup_de_db_{}", user_id), "", &e, &mut ExtDbMap::new());
            }
        }
    }

    // Backup all ce db if accessible.
    let user_ids = get_all_user_ids()?;
    for user_id in user_ids.iter() {
        if let Err(e) = backup_ce_db_if_accessible(*user_id, start_time) {
            upload_fault_system_event(&CallingInfo::new_self(), *start_time,
                &format!("backup_ce_db_{}", *user_id), "", &e, &mut ExtDbMap::new());
        }
    }

    Ok(())
}

#[derive(Clone)]
#[repr(C)]
struct EventCallBack {
    on_package_removed: extern "C" fn(PackageInfoFfi),
    on_user_removed: extern "C" fn(i32),
    on_screen_off: extern "C" fn(),
    on_charging: extern "C" fn(),
    on_app_restore: extern "C" fn(i32, *const u8, i32),
    on_user_unlocked: extern "C" fn(i32),
    on_connectivity_change: extern "C" fn(),
    on_data_share_ready: extern "C" fn(),
    on_user_switched: extern "C" fn(i32),
    on_uninstall_data_cleared:
        extern "C" fn(i32, *const c_char, *const c_char, i32, *const c_char, *const c_char),
}

extern "C" {
    fn SubscribeSystemAbility(eventCallBack: EventCallBack) -> bool;
    fn UnSubscribeSystemAbility() -> bool;
    fn SubscribeSystemEvent(eventCallBack: EventCallBack) -> bool;
    fn UnSubscribeSystemEvent() -> bool;
}

/// Subscribe to the add and remove events of system abilities.
pub(crate) fn subscribe() {
    unsafe {
        let call_back = EventCallBack {
            on_package_removed,
            on_user_removed,
            on_screen_off: delete_crypto_need_unlock,
            on_charging,
            on_app_restore,
            on_user_unlocked,
            on_connectivity_change,
            on_data_share_ready,
            on_user_switched,
            on_uninstall_data_cleared: on_uninstall_data_cleared_ffi,
        };
        if SubscribeSystemEvent(call_back.clone()) {
            logi!("Subscribe system event success.");
        } else {
            loge!("Subscribe system event failed. Subscribe System Ability wait system event service start.");
            if SubscribeSystemAbility(call_back) {
                logi!("Subscribe system ability success.");
            } else {
                loge!("Subscribe system ability failed.")
            }
        }
    }
}

/// Unsubscribe to the add and remove events of system abilities.
pub(crate) fn unsubscribe() {
    unsafe {
        if !UnSubscribeSystemAbility() {
            loge!("Unsubscribe system ability failed.")
        }

        if !UnSubscribeSystemEvent() {
            loge!("Unsubscribe system event failed.")
        }
    }
}
