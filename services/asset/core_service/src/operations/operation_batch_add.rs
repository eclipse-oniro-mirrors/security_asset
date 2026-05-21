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
use asset_crypto_manager::db_key_operator::get_db_key_by_asset_map;
use asset_db_operator::{
    common::{self},
    database::Database,
    types::{DB_DATA_VERSION, DbMap, column},
};
use asset_definition::{
    AssetMap, ErrCode, Result, Tag, Value, macros_lib
};
use asset_utils::time;

use crate::operations::common::check_tags_consistency;

const CONSISTENCY_ATTRS: [Tag; 2] = [
    Tag::RequireAttrEncrypted, Tag::GroupId
];

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    db_data.insert(column::VERSION, Value::Number(DB_DATA_VERSION));

    let time = time::system_time_in_millis()?;
    db_data.insert(column::CREATE_TIME, Value::Bytes(time.clone()));
    db_data.insert(column::UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

fn local_batch_add(
    calling_info: &CallingInfo,
    attributes_array: &[AssetMap]
) -> Result<Vec<(u32, u32)>> {
    let attributes = match attributes_array.first() {
        Some(attr) => attr,
        None => return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Batch Add argument empty."),
    };
    common::check_value_validity(attributes)?;
    let mut db_map = DbMap::new();
    check_tags_consistency(&CONSISTENCY_ATTRS, attributes_array)?;
    add_system_attrs(&mut db_map)?;
    common::add_calling_info(calling_info, &mut db_map);
    common::check_system_permission(attributes)?;
    let db_key = get_db_key_by_asset_map(calling_info.user_id(), attributes)?;
    let mut db = Database::build(calling_info, db_key)?;
    db.insert_batch_datas(&db_map, attributes_array, calling_info)
}

pub(crate) fn batch_add(calling_info: &CallingInfo, attributes_array: &[AssetMap]) -> Result<Vec<(u32, u32)>> {
    local_batch_add(calling_info, attributes_array)
}
