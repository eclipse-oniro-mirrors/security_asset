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

use asset_common::{ConstAssetBlob, ConstAssetBlobArray, GROUP_SEPARATOR};
use asset_definition::{log_throw_error, ErrCode, Result};
use asset_file_operator::de_operator::delete_user_de_dir;
use asset_log::{loge, logi};
use system_ability_fwk::cxx_share::SystemAbilityOnDemandReason;

use crate::{
    common_event::listener, unload_handler::DELAYED_UNLOAD_TIME_IN_SEC, unload_sa, PackageInfo, PackageInfoFfi,
    WantParser,
};

const USER_ID: &str = "userId";
const SANDBOX_APP_INDEX: &str = "sandbox_app_index";
const APP_ID: &str = "appId";
const BUNDLE_NAME: &str = "bundleName";
const APP_RESTORE_INDEX: &str = "index";
const DEVELOPER_ID: &str = "developerId";
const GROUP_IDS: &str = "assetAccessGroups";

impl WantParser<PackageInfo> for HashMap<String, String> {
    fn parse(&self) -> Result<PackageInfo> {
        // parse user id from want
        let Some(user_id) = self.get(USER_ID) else {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATIL]Get removed user id fail");
        };
        let user_id = match user_id.parse::<i32>() {
            Ok(parsed_value) => parsed_value,
            Err(_) => return log_throw_error!(ErrCode::InvalidArgument, "[FATIL]Parse removed userId fail"),
        };

        // parse app id from want
        let Some(app_id) = self.get(APP_ID) else {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATIL]Get removed owner info failed, get appId fail");
        };

        // parse bundle name from want
        let Some(bundle_name) = self.get(BUNDLE_NAME) else {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATIL]Get restore appIndex fail");
        };
        let mut bundle_name = bundle_name.clone();
        bundle_name.push('\0');

        // parse sandbox app index from want
        let app_index;
        match self.get(SANDBOX_APP_INDEX) {
            Some(v) => match v.parse::<i32>() {
                Ok(sandbox_app_index) => app_index = sandbox_app_index,
                Err(_) => return log_throw_error!(ErrCode::InvalidArgument, "[FATIL]Parse removed appIndex fail"),
            },
            None => return log_throw_error!(ErrCode::InvalidArgument, "[FATIL]Get removed appIndex fail"),
        };

        // parse groups from want
        let groups: Option<Vec<String>> = match (self.get(DEVELOPER_ID), self.get(GROUP_IDS)) {
            (Some(developer_id), Some(group_ids)) => {
                if app_index != 0 {
                    return log_throw_error!(
                        ErrCode::PermissionDenied,
                        "[FATIL]App with non-zero app index is not allowed to access groups!"
                    );
                }
                let groups: Vec<String> =
                    group_ids.split(GROUP_SEPARATOR).map(|group_id| group_id.to_string()).collect();
                Some(groups.iter().map(|group_id| format!("{},{}", developer_id, group_id)).collect())
            },
            _ => None,
        };

        Ok(PackageInfo { user_id, app_index, app_id: app_id.to_string(), groups, bundle_name })
    }
}

fn handle_package_removed(want: &HashMap<String, String>, is_sandbox: bool) {
    if let Ok(mut package_info) = want.parse() {
        if !is_sandbox {
            package_info.app_index = 0;
        }
        let user_id = package_info.user_id;
        let owner_str = format!("{}_{}", package_info.app_id, package_info.app_index);
        let owner = ConstAssetBlob { size: owner_str.len() as u32, data: owner_str.as_ptr() };
        let app_index = package_info.app_index;
        let groups: Option<Vec<ConstAssetBlob>> = package_info.groups.map(|group| {
            group.iter().map(|group| ConstAssetBlob { size: group.len() as u32, data: group.as_ptr() }).collect()
        });
        let groups = match groups {
            Some(groups) => ConstAssetBlobArray { size: groups.len() as u32, blobs: groups.as_ptr() },
            None => ConstAssetBlobArray { size: 0, blobs: null() },
        };
        let bundle_name = package_info.bundle_name.as_ptr();
        listener::on_package_removed(PackageInfoFfi { user_id, app_index, owner, groups, bundle_name });
    };
}

pub(crate) fn handle_common_event(reason: SystemAbilityOnDemandReason) {
    let reason_name: String = reason.name;
    if reason_name == "usual.event.PACKAGE_REMOVED" {
        let want = reason.extra_data.want();
        handle_package_removed(&want, false);
    } else if reason_name == "usual.event.SANDBOX_PACKAGE_REMOVED" {
        let want = reason.extra_data.want();
        handle_package_removed(&want, true);
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
    }
    unload_sa(DELAYED_UNLOAD_TIME_IN_SEC as u64);
}
