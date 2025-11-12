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
    os::unix::fs::PermissionsExt,
    path::Path,
    slice,
    sync::Mutex,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

use lazy_static::lazy_static;

use asset_common::{
    AutoCounter, CallingInfo, ConstAssetBlob, ConstAssetBlobArray, Group, ModifyAssetBlob, MutAssetBlobArray, OwnerType
};
use asset_crypto_manager::{crypto_manager::CryptoManager, secret_key::SecretKey, 
    db_key_operator::{DbKey, get_db_key}};
use asset_db_operator::{
    database::Database,
    database_file_upgrade::{
        construct_splited_db_name, trigger_db_upgrade, update_upgrade_list,
    },
    types::{
        column::{self},
        DbMap,
    },
};
use asset_definition::{log_throw_error, ErrCode, Result, SyncType, Value};
use asset_file_operator::{
    ce_operator::is_db_key_cipher_file_exist,
    common::{BACKUP_SUFFIX, CE_ROOT_PATH, DB_SUFFIX, DE_ROOT_PATH},
    de_operator::delete_user_de_dir,
};
use asset_log::{loge, logi, logw};
use asset_plugin::asset_plugin::AssetPlugin;
use asset_sdk::plugin_interface::{
    EventType, ExtDbMap, PARAM_NAME_APP_INDEX, PARAM_NAME_BUNDLE_NAME, PARAM_NAME_IS_HAP, PARAM_NAME_USER_ID,
};

use crate::data_size_mod::handle_data_size_upload;
use crate::{sys_event::upload_fault_system_event, PackageInfoFfi, upgrade_operator, upgrade_ce};

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
    match calling_info.group() {
        Some(_) => {
            let _ = db.delete_datas(&delete_condition, Some(reverse_condition), false)?;
            let data_exists = db.is_data_exists(&check_condition, false)?;
            if !data_exists {
                remove_db(&format!("{}/{}", DE_ROOT_PATH, calling_info.user_id()), calling_info, false)?;
            }
            Ok(DataExist::GroupData(data_exists))
        },
        None => {
            delete_condition.insert(column::IS_PERSISTENT, Value::Bool(false));
            let _ = db.delete_datas(&delete_condition, Some(reverse_condition), false)?;
            let data_exists = db.is_data_exists(&check_condition, false)?;
            if !data_exists {
                remove_db(&format!("{}/{}", DE_ROOT_PATH, calling_info.user_id()), calling_info, false)?;
            }
            Ok(DataExist::OwnerData(data_exists))
        },
    }
}

fn delete_in_ce_db_on_package_removed(calling_info: &CallingInfo, reverse_condition: &DbMap) -> Result<DataExist> {
    let db_key = get_db_key(calling_info.user_id(), true)?;
    let mut db = Database::build(calling_info, db_key)?;
    let mut delete_condition = DbMap::new();
    let check_condition = DbMap::new();
    match calling_info.group() {
        Some(_) => {
            let _ = db.delete_datas(&delete_condition, Some(reverse_condition), false)?;
            let data_exists = db.is_data_exists(&check_condition, false)?;
            if !data_exists {
                remove_db(&format!("{}/{}/asset_service", CE_ROOT_PATH, calling_info.user_id()), calling_info, true)?;
            }
            Ok(DataExist::GroupData(data_exists))
        },
        None => {
            delete_condition.insert(column::IS_PERSISTENT, Value::Bool(false));
            let _ = db.delete_datas(&delete_condition, Some(reverse_condition), false)?;
            let data_exists = db.is_data_exists(&check_condition, false)?;
            if !data_exists {
                remove_db(&format!("{}/{}/asset_service", CE_ROOT_PATH, calling_info.user_id()), calling_info, true)?;
            }
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
            _ => log_throw_error!(ErrCode::AccessDenied, "[FATAL][SA]Cannot delete owner and group data at same time"),
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

        let mut the_group_ids = MutAssetBlobArray { size: group_id_blobs.len() as u32, blobs: group_id_blobs.as_mut_ptr() };

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

fn delete_data_by_owner(
    user_id: i32,
    owner: ConstAssetBlob,
    developer_id: ConstAssetBlob,
    group_ids: ConstAssetBlobArray,
) {
    let _counter_user = AutoCounter::new();
    let start_time = Instant::now();
    let owner: Vec<u8> = unsafe { slice::from_raw_parts(owner.data, owner.size as usize).to_vec() };

    for calling_info in construct_calling_infos(user_id, owner.clone(), developer_id, group_ids) {
        clear_cryptos(&calling_info);
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
                upload_fault_system_event(&calling_info, start_time, "on_package_removed", &e);
                Ok(())
            },
        };
        if let Err(e) = res {
            // Report the key operation fault event.
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, start_time, "on_package_removed", &e);
        }
    }
}

pub(crate) extern "C" fn on_package_removed(package_info: PackageInfoFfi) {
    delete_data_by_owner(package_info.user_id, package_info.owner, package_info.developer_id, package_info.group_ids);

    let bundle_name: Vec<u8> = unsafe {
        slice::from_raw_parts(package_info.bundle_name.data, package_info.bundle_name.size as usize).to_vec()
    };
    logi!(
        "[INFO]On app -{}-{}-{}- removed.",
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

extern "C" fn on_user_removed(user_id: i32) {
    let _counter_user = AutoCounter::new();
    let _ = delete_user_de_dir(user_id);
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

pub(crate) extern "C" fn backup_db() {
    let _counter_user = AutoCounter::new();
    let cur_time = Instant::now();
    logi!("[INFO]Start backup db.");

    let mut record_time = RECORD_TIME.lock().expect("Failed to lock RECORD_TIME");

    let should_backup = match *record_time {
        Some(ref last_time) => cur_time.duration_since(*last_time) > Duration::new(3600, 0),
        None => true,
    };

    if should_backup {
        *record_time = Some(cur_time);
        if let Err(e) = backup_all_db(&cur_time) {
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, cur_time, "backup_db", &e);
        }
    }
    logi!("[INFO]Finish backup db.");
}

pub(crate) extern "C" fn on_app_restore(user_id: i32, bundle_name: *const u8, app_index: i32) {
    let c_str = unsafe { CStr::from_ptr(bundle_name as _) };
    let bundle_name = match c_str.to_str() {
        Ok(s) => s.to_string(),
        Err(e) => {
            loge!("[FATAL]Parse sting from bundle name failed, error is {}.", e);
            return;
        },
    };
    logi!("[INFO]On app -{}-{}- restore.", user_id, bundle_name);

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
}

pub(crate) extern "C" fn on_user_unlocked(user_id: i32) {
    logi!("[INFO]On user -{}- unlocked.", user_id);

    // Trigger upgrading de db version and key alias
    match trigger_db_upgrade(user_id, None) {
        Ok(()) => logi!("upgrade de db version and key alias on user-unlocked success."),
        Err(e) => loge!("upgrade de db version and key alias on user-unlocked failed, err is: {}", e),
    }

    // Trigger upgrading ce db version and key alias
    match get_db_key(user_id, true) {
        Ok(db_key) => {
            match trigger_db_upgrade(user_id, db_key) {
                Ok(()) => logi!("upgrade ce db version and key alias on user-unlocked success."),
                Err(e) => loge!("upgrade ce db version and key alias on user-unlocked failed, err is: {}", e),
            }
        },
        Err(e) => loge!("get db key on user-unlocked failed, err is: {}", e),
    }

    if let Err(e) = handle_data_size_upload() {
        loge!("Failed to handle data upload: {}", e);
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
    let _ = upgrade_ce::upgrade_ce_data(user_id);
}

pub(crate) fn notify_on_user_removed(user_id: i32) {
    logi!("[INFO]On user remove [{}].", user_id);

    if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
        let mut params = ExtDbMap::new();
        params.insert(PARAM_NAME_USER_ID, Value::Number(user_id as u32));
        match load.process_event(EventType::OnUserRemoved, &mut params) {
            Ok(()) => logi!("process user removed event success."),
            Err(code) => loge!("process user removed event failed, code: {}", code),
        }
    }
}

pub(crate) extern "C" fn on_schedule_wakeup() {
    logi!("[INFO]On SA wakes up at a scheduled time(36H).");
    trigger_sync();
}

pub(crate) extern "C" fn on_connectivity_change() {
    let _lock = LAST_TRIGGER_TIME_FILE_MUTEX.lock().unwrap();
    let path = format!("{}/last_trigger_time.txt", DE_ROOT_PATH);
    let last_time = read_last_trigger_time(&path).unwrap_or(0);
    let current = SystemTime::now();
    if let Ok(duration) = current.duration_since(UNIX_EPOCH) {
        let current = duration.as_secs();
        if current > last_time && current - last_time >= TWELVE_HOURS_AS_SECS {
            logi!("[INFO]On SA connectivity change.");
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
    let mut user_ids_size: u32 = USER_ID_VEC_BUFFER;
    let mut user_ids: Vec<i32> = vec![0i32; user_ids_size as usize];
    let user_ids_size_ptr = &mut user_ids_size;
    let user_ids_ptr = user_ids.as_mut_ptr();
    let ret: i32 = unsafe { GetFirstUnlockUserIds(user_ids_ptr, user_ids_size_ptr) };
    if ret != SUCCESS {
        return loge!("[FATAL] Get users IDs failed. Do not sync data.");
    }

    if user_ids_size < USER_ID_VEC_BUFFER {
        user_ids.truncate(user_ids_size as usize);
    }
    let self_bundle_name = "asset_service";
    for user_id in &user_ids {
        if let Ok(load) = AssetPlugin::get_instance().load_plugin() {
            let mut params = ExtDbMap::new();
            params.insert(PARAM_NAME_USER_ID, Value::Number(*user_id as u32));
            params.insert(PARAM_NAME_BUNDLE_NAME, Value::Bytes(self_bundle_name.as_bytes().to_vec()));
            match load.process_event(EventType::Sync, &mut params) {
                Ok(()) => logi!("process sync ext event {} success.", *user_id),
                Err(code) => loge!("process sync ext event failed, code: {}, user_id: {}", code, *user_id),
            }
        }
    }
}

fn backup_de_db_if_accessible(entry: &DirEntry, user_id: i32) -> Result<()> {
    for db_path in fs::read_dir(format!("{}", entry.path().to_string_lossy()))? {
        let db_path = db_path?;
        let db_name = db_path.file_name().to_string_lossy().to_string();
        if db_name.ends_with(DB_SUFFIX) {
            let from_path = db_path.path().to_string_lossy().to_string();
            Database::check_db_accessible(from_path.clone(), user_id, db_name.clone(), None)?;
            let backup_path = format!("{}{}", from_path, BACKUP_SUFFIX);
            fs::copy(from_path, backup_path)?;
        }
    }
    Ok(())
}

fn backup_ce_db_if_accessible(user_id: i32) -> Result<()> {
    if user_id < MINIMUM_MAIN_USER_ID {
        return Ok(());
    }
    let ce_path = format!("{}/{}/asset_service", CE_ROOT_PATH, user_id);
    for db_path in fs::read_dir(ce_path)? {
        let db_path = db_path?;
        let db_name = db_path.file_name().to_string_lossy().to_string();
        if db_name.ends_with(DB_SUFFIX) {
            let from_path = db_path.path().to_string_lossy().to_string();
            let db_key = DbKey::get_db_key(user_id)?;
            Database::check_db_accessible(from_path.clone(), user_id, db_name.clone(), Some(&db_key.db_key))?;
            let backup_path = format!("{}{}", from_path, BACKUP_SUFFIX);
            fs::copy(from_path, backup_path)?;
        }
    }

    Ok(())
}

fn backup_all_db(start_time: &Instant) -> Result<()> {
    // Backup all de db if accessible.
    for entry in fs::read_dir(DE_ROOT_PATH)? {
        let entry = entry?;
        if let Ok(user_id) = entry.file_name().to_string_lossy().to_string().parse::<i32>() {
            if let Err(e) = backup_de_db_if_accessible(&entry, user_id) {
                let calling_info = CallingInfo::new_self();
                upload_fault_system_event(&calling_info, *start_time, &format!("backup_de_db_{}", user_id), &e);
            }
        }
    }

    // Backup all ce db if accessible.
    let mut user_ids_size: u32 = 0;
    let user_ids_size_ptr = &mut user_ids_size;
    let mut ret: i32;
    ret = unsafe { GetUsersSize(user_ids_size_ptr) };
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::AccountError, "[FATAL][SA]Get users size failed.");
    }

    let mut user_ids: Vec<i32> = vec![0i32; (*user_ids_size_ptr + USER_ID_VEC_BUFFER).try_into().unwrap()];
    let user_ids_ptr = user_ids.as_mut_ptr();
    ret = unsafe { GetUserIds(user_ids_ptr, user_ids_size_ptr) };
    if ret != SUCCESS {
        return log_throw_error!(ErrCode::AccountError, "[FATAL][SA]Get user IDs failed.");
    }

    let user_ids_slice = unsafe { slice::from_raw_parts_mut(user_ids_ptr, (*user_ids_size_ptr).try_into().unwrap()) };
    for user_id in user_ids_slice.iter() {
        if let Err(e) = backup_ce_db_if_accessible(*user_id) {
            let calling_info = CallingInfo::new_self();
            upload_fault_system_event(&calling_info, *start_time, &format!("backup_ce_db_{}", *user_id), &e);
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
            on_charging: backup_db,
            on_app_restore,
            on_user_unlocked,
            on_connectivity_change,
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
