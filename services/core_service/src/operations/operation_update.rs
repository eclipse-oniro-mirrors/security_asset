/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This module is used to update the specified alias of Asset.

use asset_db_operator::{
    database_table_helper::{DatabaseHelper, COLUMN_SECRET, COLUMN_UPDATE_TIME},
    types::DbMap,
};
use asset_definition::{AssetMap, ErrCode, Result, Tag, Value};
use asset_log::{loge, logi};

use crate::{calling_info::CallingInfo, operations::common};

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    let time = common::get_system_time()?;
    db_data.insert(COLUMN_UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

const QUERY_REQUIRED_ATTRS: [Tag; 1] = [Tag::Alias];
const UPDATE_OPTIONAL_ATTRS: [Tag; 1] = [Tag::Secret];

fn check_arguments(query: &AssetMap, attrs_to_update: &AssetMap) -> Result<()> {
    // Check attributes used to query.
    common::check_required_tags(query, &QUERY_REQUIRED_ATTRS)?;
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    common::check_tag_validity(query, &valid_tags)?;
    common::check_value_validity(query)?;

    // Check attributes to update.
    valid_tags = common::NORMAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&UPDATE_OPTIONAL_ATTRS);
    common::check_tag_validity(attrs_to_update, &valid_tags)?;
    common::check_value_validity(attrs_to_update)
}

pub(crate) fn update(query: &AssetMap, update: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(query, update)?;

    let mut query_db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut query_db_data);

    let mut update_db_data = common::into_db_map(update);
    add_system_attrs(&mut update_db_data)?;

    if update.contains_key(&Tag::Secret) {
        let mut results = DatabaseHelper::query_columns(calling_info.user_id(), &vec![], &query_db_data, None)?;
        if results.len() != 1 {
            loge!("query to-be-updated asset failed, found [{}] assets", results.len());
            return Err(ErrCode::NotFound);
        }

        let result = results.get_mut(0).unwrap();
        result.insert(COLUMN_SECRET, update[&Tag::Secret].clone());
        let cipher = common::encrypt(calling_info, result)?;
        update_db_data.insert(COLUMN_SECRET, Value::Bytes(cipher));
    }

    // call sql to update
    let update_num = DatabaseHelper::update_datas(calling_info.user_id(), &query_db_data, &update_db_data)?;
    if update_num == 0 {
        loge!("[FATAL]Update asset failed, update 0 asset.");
        return Err(ErrCode::NotFound);
    }
    logi!("update {} data", update_num);
    Ok(())
}
