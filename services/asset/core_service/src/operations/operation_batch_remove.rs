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
    common::{check_tag_validity, check_value_validity, add_calling_info, check_system_permission},
    database::Database,
    types::{DbMap, column},
};
use asset_definition::{
    AssetMap, Result, SyncStatus, Tag, Value,
};
use asset_log::logi;
use asset_sdk::Extension;
use asset_utils::time;

use crate::operations::common::check_tags_consistency;

const OPTIONAL_ATTRS: [Tag; 3] = [Tag::RequireAttrEncrypted, Tag::GroupId, Tag::Alias];
const CONSISTENCY_ATTRS: [Tag; 2] = [
    Tag::RequireAttrEncrypted, Tag::GroupId
];

fn check_and_get_aliases(attributes_array: &[AssetMap]) -> Result<Vec<Vec<u8>>> {
    check_tags_consistency(&CONSISTENCY_ATTRS, attributes_array)?;
    let mut aliases = Vec::new();
    for attrs in attributes_array {
        check_tag_validity(attrs, &OPTIONAL_ATTRS)?;
        check_value_validity(attrs)?;
        let alias = attrs.get_bytes_attr(&Tag::Alias)?;
        aliases.push(alias.clone());
    }
    Ok(aliases)
}

fn loacl_batch_remove(attributes_array: &[AssetMap], calling_info: &CallingInfo) -> Result<()> {
    let attributes = match attributes_array.first() {
        Some(attr) => attr,
        None => return Ok(()),
    };
    let aliases = check_and_get_aliases(attributes_array)?;

    let db_key = get_db_key_by_asset_map(calling_info.user_id(), attributes)?;
    let mut db = Database::build(calling_info, db_key)?;
    let mut condition = DbMap::new();
    add_calling_info(calling_info, &mut condition);
    check_system_permission(attributes)?;
    let mut update_datas = DbMap::new();
    let time = time::system_time_in_millis()?;
    update_datas.insert(column::UPDATE_TIME, Value::Bytes(time));
    update_datas.insert(column::SYNC_STATUS, Value::Number(SyncStatus::SyncDel as u32));

    let total_removed_count: i32 = db.delete_batch_datas(&condition, &update_datas, &aliases)?;
    logi!("total removed count = {}", total_removed_count);
    Ok(())
}

pub(crate) fn batch_remove(calling_info: &CallingInfo, attributes_array: &[AssetMap]) -> Result<()> {
    loacl_batch_remove(attributes_array, calling_info)
}
