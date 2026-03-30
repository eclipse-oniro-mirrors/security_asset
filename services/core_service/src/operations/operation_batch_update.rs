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
    common::{self, into_db_map},
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

extern "C" {
    fn CheckSystemHapPermission() -> bool;
}

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    db_data.insert(column::VERSION, Value::Number(DB_DATA_VERSION));

    let time = time::system_time_in_millis()?;
    db_data.insert(column::CREATE_TIME, Value::Bytes(time.clone()));
    db_data.insert(column::UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn add_default_attrs(db_data: &mut DbMap) {
    db_data.entry(column::ACCESSIBILITY).or_insert(Value::Number(Accessibility::default() as u32));
    db_data.entry(column::AUTH_TYPE).or_insert(Value::Number(AuthType::default() as u32));
    db_data.entry(column::REQUIRE_PASSWORD_SET).or_insert(Value::Bool(bool::default()));
}

fn local_batch_update(
    calling_info: &CallingInfo,
    attributes_array: &[AssetMap],
    attributes_to_update_array: &[AssetMap]
) -> Result<Vec<(u32, u32)>> {
    
    common::check_value_validity(&attributes)?;
    let db_map = into_db_map(&attributes)?;
    add_default_attrs(db_map);
    add_system_attrs(db_map)?;
    common::add_calling_info(calling_info, db_map);
    common::check_system_permission(&attributes)?;
    let db_key = get_db_key_by_asset_map(calling_info.user_id(), &attributes)?;
    let mut db = Database::build(calling_info, db_key)?;
    db.insert_batch_datas(db_map, attributes_array, &calling_info)

    let attributes = attributes_array[0];
    attributes.entry(Tag::RequireAttrEncrypted).or_insert(Value::Bool(bool::default()));
    common::add_group(&calling_info, db_map);
    let db_key = get_db_key_by_asset_map(calling_info.user_id(), &attributes)?;
    let mut db = Database::build(calling_info, db_key)?;
    db.update_batch_datas(db_map, attributes_array, attributes_to_update_array, &calling_info)
}

pub(crate) fn batch_update(
    calling_info: &CallingInfo,
    attributes_array: &Vec<AssetMap>,
    attributes_to_update_array: &Vec<AssetMap>
) -> Result<Vec<(u32, u32)>> {
    if attributes_array.is_empty() || attributes_to_update_array.is_empty() {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Batch Update argument empty.");
    }
    local_batch_update(calling_info, attributes_array, attributes_to_update_array)
}
