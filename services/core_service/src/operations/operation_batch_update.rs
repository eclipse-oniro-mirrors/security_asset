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
    db_key_operator::get_db_key_by_asset_map,
};
use asset_db_operator::{
    common::{ACCESS_CONTROL_ATTRS, ASSET_SYNC_ATTRS, CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS, NORMAL_LOCAL_LABEL_ATTRS, check_required_tags, check_tag_validity, check_value_validity, into_db_map},
    database::Database,
    types::{DB_DATA_VERSION, DbMap, column},
};
use asset_definition::{
    macros_lib, Accessibility, AssetMap, AuthType, ErrCode, Result,
    Tag, Value,
};
use asset_sdk::log_throw_error;
use asset_utils::time;

use crate::operations::common::{check_group_validity, inform_asset_ext, update_cloud_sync_status};

const QUERY_VALID_ATTRS: [Tag; 1] = [Tag::Alias];
const UPDATE_OPTIONAL_ATTRS: [Tag; 1] = [Tag::Secret];

fn check_update_array(attributes_array: &[AssetMap]) -> Result<()> {
    for attrs in attributes_array {
        check_required_tags(attrs, &QUERY_VALID_ATTRS)?;
        let mut valid_tags = CRITICAL_LABEL_ATTRS.to_vec();
        valid_tags.extend_from_slice(&NORMAL_LABEL_ATTRS);
        valid_tags.extend_from_slice(&NORMAL_LOCAL_LABEL_ATTRS);
        valid_tags.extend_from_slice(&ACCESS_CONTROL_ATTRS);
        check_tag_validity(attrs, &valid_tags)?;
        check_value_validity(attrs)?;
    }
    Ok(())
}

fn check_update_array(attributes_array: &[AssetMap]) -> Result<()> {
    for attrs in attributes_array {
        if attr.empty() {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The data to update contains empty attributes.");
        }
        let mut valid_tags = NORMAL_LABEL_ATTRS.to_vec();
        valid_tags.extend_from_slice(&NORMAL_LOCAL_LABEL_ATTRS);
        valid_tags.extend_from_slice(&ASSET_SYNC_ATTRS);
        valid_tags.extend_from_slice(&UPDATE_OPTIONAL_ATTRS);
        check_tag_validity(attrs, &valid_tags)?;
        check_value_validity(attrs)?;
    }
    Ok(())
}

fn local_batch_update(
    calling_info: &CallingInfo,
    attributes_array: &[AssetMap],
    attributes_to_update_array: &[AssetMap]
) -> Result<Vec<(u32, u32)>> {
    let attributes = attributes_array[0];
    let mut db_map = into_db_map(&attributes);
    check_attrs_array(attributes_array)?;
    check_update_array(attributes_to_update_array)?;
    let db_key = get_db_key_by_asset_map(calling_info.user_id(), &attributes)?;
    let mut db = Database::build(calling_info, db_key)?;
    db.update_batch_datas(&mut db_map, attributes_array, attributes_to_update_array, &calling_info)
}

pub(crate) fn batch_update(
    calling_info: &CallingInfo,
    attributes_array: &Vec<AssetMap>,
    attributes_to_update_array: &Vec<AssetMap>
) -> Result<Vec<(u32, u32)>> {
    if attributes_array.is_empty() || attributes_to_update_array.is_empty()
    || attributes_array.len() != attributes_to_update_array.len(){
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]Batch Update argument empty.");
    }
    local_batch_update(calling_info, attributes_array, attributes_to_update_array)
}
