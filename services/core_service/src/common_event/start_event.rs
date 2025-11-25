/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

//! This module is used to handle start event.

use std::{collections::HashMap, ptr::null};

use asset_common::{AutoCounter, ConstAssetBlob, ConstAssetBlobArray, TaskManager, GROUP_SEPARATOR};
use asset_definition::{macros_lib, ErrCode, Result};
use asset_file_operator::de_operator::delete_user_de_dir;
use asset_log::{loge, logi, logw};
use system_ability_fwk::cxx_share::SystemAbilityOnDemandReason;

use crate::{common_event::listener, unload_sa, PackageInfo, PackageInfoFfi, WantParser};

const USER_ID: &str = "userId";
const SANDBOX_APP_INDEX: &str = "sandbox_app_index";
const APP_INDEX: &str = "appIndex";
const APP_RESTORE_INDEX: &str = "index";
const APP_ID: &str = "appId";
const BUNDLE_NAME: &str = "bundleName";
const DEVELOPER_ID: &str = "developerId";
const GROUP_IDS: &str = "assetAccessGroups";
const NET_CONN_STATE_CONNECTED: &str = "3";

struct PackageRemovedWant<'a>(&'a HashMap<String, String>);
impl WantParser<PackageInfo> for PackageRemovedWant<'_> {
    fn parse(&self) -> Result<PackageInfo> {
        // parse user id from want
        let Some(user_id) = self.0.get(USER_ID) else {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get removed userId fail!");
        };
        let user_id = match user_id.parse::<i32>() {
            Ok(user_id) => user_id,
            _ => return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Parse removed userId fail!"),
        };

        // parse app id from want
        let Some(app_id) = self.0.get(APP_ID) else {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get removed ownerInfo fail, get appId fail!");
        };
        let app_id = app_id.to_string();

        // parse bundle name from want
        let Some(bundle_name) = self.0.get(BUNDLE_NAME) else {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get restore appInfo fail, get bundleName fail!");
        };
        let bundle_name = bundle_name.to_string();

        // parse app index from want
        let app_index = match self.0.get(SANDBOX_APP_INDEX) {
            Some(sandbox_app_index) => sandbox_app_index,
            _ => {
                logw!("[WARNING]Not sandbox app, getting non-sandbox(main or clone) appIndex!");
                match self.0.get(APP_INDEX) {
                    Some(app_index) => app_index,
                    _ => return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Get removed appIndex fail!"),
                }
            },
        };
        let app_index = match app_index.parse::<i32>() {
            Ok(app_index) => app_index,
            _ => return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Parse removed appIndex fail!"),
        };

        // parse groups from want
        let (developer_id, group_ids) = match (self.0.get(DEVELOPER_ID), self.0.get(GROUP_IDS)) {
            (Some(developer_id), Some(group_ids)) => {
                let group_ids: Vec<String> =
                    group_ids.split(GROUP_SEPARATOR).map(|group_id| group_id.to_string()).collect();
                (Some(developer_id.to_string()), Some(group_ids))
            },
            _ => (None, None),
        };

        Ok(PackageInfo { user_id, app_index, app_id, developer_id, group_ids, bundle_name })
    }
}

fn handle_package_removed(want: &HashMap<String, String>) {
    if let Ok(package_info) = PackageRemovedWant(want).parse() {
        let owner_str = format!("{}_{}", package_info.app_id, package_info.app_index);
        let owner = ConstAssetBlob { size: owner_str.len() as u32, data: owner_str.as_ptr() };
        let developer_id = match package_info.developer_id() {
            Some(developer_id) => ConstAssetBlob { size: developer_id.len() as u32, data: developer_id.as_ptr() },
            None => ConstAssetBlob { size: 0, data: null() },
        };
        let mut group_id_blobs: Vec<ConstAssetBlob> = Vec::new();
        let group_ids = match package_info.group_ids() {
            Some(group_ids) => {
                for group_id in group_ids {
                    group_id_blobs.push(ConstAssetBlob { size: group_id.len() as u32, data: group_id.as_ptr() });
                }
                ConstAssetBlobArray { size: group_id_blobs.len() as u32, blobs: group_id_blobs.as_ptr() }
            },
            None => ConstAssetBlobArray { size: 0, blobs: null() },
        };
        let bundle_name =
            ConstAssetBlob { size: package_info.bundle_name.len() as u32, data: package_info.bundle_name.as_ptr() };
        listener::on_package_removed(PackageInfoFfi {
            user_id: package_info.user_id,
            app_index: package_info.app_index,
            owner,
            developer_id,
            group_ids,
            bundle_name,
        });
    };
}

fn process_common_event_async(reason: SystemAbilityOnDemandReason) {
    let _counter_user = AutoCounter::new();
    let reason_name: String = reason.name;
    if reason_name == "usual.event.PACKAGE_REMOVED" || reason_name == "usual.event.SANDBOX_PACKAGE_REMOVED" {
        let want = reason.extra_data.want();
        handle_package_removed(&want);
    } else if reason_name == "usual.event.USER_REMOVED" {
        logi!("on_start by user remove");
        let _ = delete_user_de_dir(reason.extra_data.code);
        listener::notify_on_user_removed(reason.extra_data.code);
    } else if reason_name == "usual.event.CHARGING" {
        listener::backup_db();
    } else if reason_name == "usual.event.RESTORE_START" {
        let want = reason.extra_data.want();
        let user_id = match want.get(USER_ID) {
            Some(v) => match v.parse::<i32>() {
                Ok(parsed_value) => parsed_value,
                Err(_) => {
                    loge!("[FATIL]Get restore app info failed, failed to parse user_id");
                    return;
                },
            },
            None => {
                loge!("[FATIL]Get restore app info failed, get userId fail");
                return;
            },
        };
        let Some(bundle_name) = want.get(BUNDLE_NAME) else {
            loge!("[FATIL]Get restore app info failed, get bundle name failed.");
            return;
        };
        let mut bundle_name = bundle_name.clone();
        bundle_name.push('\0');
        let app_index = match want.get(APP_RESTORE_INDEX) {
            Some(v) => match v.parse::<i32>() {
                Ok(parsed_value) => parsed_value,
                Err(_) => {
                    loge!("[FATAL]Get restore app info failed, failed to parse appIndex");
                    return;
                },
            },
            None => {
                loge!("[FATIL]Get restore app info failed, failed to get appIndex");
                return;
            },
        };
        listener::on_app_restore(user_id, bundle_name.as_ptr(), app_index);
    } else if reason_name == "usual.event.USER_UNLOCKED" {
        listener::on_user_unlocked(reason.extra_data.code);
    } else if reason_name == "loopevent" {
        listener::on_schedule_wakeup();
    } else if reason_name == "USER_PIN_CREATED_EVENT" {
        logi!("[INFO]On user -{}- pin created.", reason.extra_data.code);
        listener::on_user_unlocked(reason.extra_data.code);
    } else if reason_name == "usual.event.CONNECTIVITY_CHANGE" && reason.value == NET_CONN_STATE_CONNECTED {
        listener::on_connectivity_change();
    }
    logi!("[INFO]Finish handle common event. [{}]", reason_name);
}

pub(crate) fn handle_common_event(reason: SystemAbilityOnDemandReason) {
    let handle = ylong_runtime::spawn_blocking(move || process_common_event_async(reason));
    let task_manager = TaskManager::get_instance();
    task_manager.lock().unwrap().push_task(handle);
    unload_sa();
}
