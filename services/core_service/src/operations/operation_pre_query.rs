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

//! This module prepares for querying Asset that required secondary identity authentication.

use asset_common::{
    definition::{AssetMap, AuthType, ErrCode, Result, Tag, Value},
    loge, logi,
};
use asset_db_operator::{
    database_table_helper::{DefaultDatabaseHelper, COLUMN_ACCESSIBILITY, COLUMN_AUTH_TYPE},
    types::DbMap,
};

use crate::{ calling_info::CallingInfo, operations::common, };

const OPTIONAL_ATTRS: [Tag; 1] = [Tag::AuthValidityPeriod];

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)
}

fn query_access_types(calling_info: &CallingInfo, db_data: &DbMap) -> Result<Vec<u32>> {
    let results = DefaultDatabaseHelper::query_columns_default_once(
        calling_info.user_id(),
        &vec![COLUMN_ACCESSIBILITY],
        db_data,
        None,
    )?;
    logi!("query found {}", results.len());
    if results.is_empty() {
        loge!("[FATAL]The data to be queried does not exist.");
        return Err(ErrCode::NotFound);
    }

    // into list
    let mut access_types = Vec::new();
    for db_result in results {
        let Value::Number(access_type) = db_result.get(&COLUMN_ACCESSIBILITY).unwrap() else {
            return Err(ErrCode::InvalidArgument);
        };
        access_types.push(*access_type);
    }
    Ok(access_types)
}

pub(crate) fn pre_query(query: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<u8>> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);
    db_data.insert(COLUMN_AUTH_TYPE, Value::Number(AuthType::Any as u32));


    let access_types = query_access_types(calling_info, &db_data)?;

    // use secret key to get challenge
    let mut challenge_vec = Vec::new();
    // todo 遍历每一个密钥，获取challenge
    let challenge_seperator = b'_';
    for (idx, access_type) in access_types.iter().enumerate() {
        let tmp_challenge = common::init_decrypt(calling_info, query, &(AuthType::Any as u32), access_type)?;
        challenge_vec.extend(tmp_challenge);
        if idx < access_types.len() - 1 {
            challenge_vec.push(challenge_seperator);
        }
        // todo 根据challenge等信息创建session
    }
    if challenge_vec.is_empty() {
        Err(ErrCode::NotFound)
    } else {
        logi!("get challenge successful!");
        Ok(challenge_vec)
    }
}
