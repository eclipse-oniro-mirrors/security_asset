/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! This module is used to insert batch Asset with a series of specified aliases.

use asset_common::CallingInfo;
use asset_crypto_manager::{
    crypto::Crypto,
    db_key_operator::{generate_secret_key_if_needed, get_db_key_by_asset_map},
};
use asset_db_operator::{
    common::{self, add_calling_info},
    database::Database,
    types::{DB_DATA_VERSION, DbMap, column},
};
use asset_definition::{
    macros_lib, Accessibility, AssetMap, AuthType, ConflictResolution, ErrCode, Extension, LocalStatus, Result,
    SyncStatus, SyncType, Tag, Value,
};
use asset_log::logw;
use asset_sdk::{WrapType, log_throw_error};
use asset_utils::time;

use crate::operations::common::{check_group_validity, inform_asset_ext, update_cloud_sync_status};

fn batch_remove_inner(&self, attributes_array: &Vec<AssetMap>, calling_info: &CallingInfo) -> Result<i32> {
    let attributes = attributes_array[0];
    check // 把不要的都剔除了

    let db_key = get_db_key_by_asset_map(calling_info.user_id(), &attributes)?;
    let mut db = Database::build(calling_info, db_key)?;
    let condition = convert_db_map(attributes)?;
    let mut update_datas = DbMap::new();
    let time = time::system_time_in_millis()?;
    update_datas.insert(column::UPDATE_TIME, Value::Bytes(time));
    update_datas.insert(column::SYNC_STATUS, Value::Number(SyncStatus::SyncDel as u32));
    let total_removed_count: i32 = db.delete_batch_datas(&condition, &update_datas, aliases)?;
    logi!("total removed count = {}", total_removed_count);
    Ok(total_removed_count)
}

pub(crate) fn batch_remove(calling_info: &CallingInfo, attributes_array: &Vec<AssetMap>) -> Result<()> {
    if attributes_array.is_empty() {
        return Ok(());
    }
    Ok(())
}
