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

//! This module is used to delete the Asset, including single and batch deletion.

use asset_constants::CallingInfo;
use asset_db_operator::{
    database::Database,
    types::{column, DbMap},
};
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result, Tag, Value, SyncStatus, SyncType};
use asset_utils::time;
use asset_log::logi;

use crate::operations::common;

const OPTIONAL_ATTRS: [Tag; 1] = [Tag::UserId];

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    let time = time::system_time_in_millis()?;
    db_data.insert(column::UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn add_normal_attrs(db_data: &mut DbMap) {
    db_data.insert(column::SYNC_STATUS, Value::Number(SyncStatus::SyncDel as u32));
}

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::NORMAL_LOCAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&common::ASSET_SYNC_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)?;
    common::check_system_permission(attributes)
}

pub(crate) fn remove(query: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);

    let mut update_db_data = DbMap::new();
    add_system_attrs(&mut update_db_data)?;
    add_normal_attrs(&mut update_db_data);

    let results = Database::build(calling_info.user_id())?.query_datas(&vec![], &db_data, None, true)?;
    if results.is_empty() {
        return log_throw_error!(ErrCode::NotFound, "[FATAL]The data to be deleted does not exist.");
    }

    let update_num = Database::build(calling_info.user_id())?.update_datas(&db_data, true, &update_db_data)?;
    if update_num == 0 {
        return log_throw_error!(ErrCode::NotFound, "[FATAL]The data to be deleted does not exist.");
    }

    let mut reverse_condition = DbMap::new();
    reverse_condition.insert(column::SYNC_TYPE, Value::Number(SyncType::TrustedAccount as u32));
    let remove_num = Database::build(calling_info.user_id())?.delete_datas(&db_data, Some(&reverse_condition), false)?;
    logi!("Delete num: {}", remove_num);

    common::inform_asset_ext(query, calling_info.user_id());

    Ok(())
}
