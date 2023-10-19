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

use std::collections::HashSet;

use asset_common::{
    definition::{AssetMap, AuthType, ErrCode, Result, Tag, Value},
    loge, logi,
};

use crate::{
    calling_info::CallingInfo,
    operations::{common, operation_query::query_attrs},
};

const OPTIONAL_ATTRS: [Tag; 1] = [Tag::AuthValidityPeriod];

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    valid_tags.extend_from_slice(&OPTIONAL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)
}

pub(crate) fn pre_query(query: &AssetMap, calling_info: &CallingInfo) -> Result<Vec<u8>> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);

    // todo: 不依赖query中封装的函数，直接调用数据库的查询接口，只查询authType和accessType
    let all_data = query_attrs(calling_info, &db_data, query)?;
    // get all secret key
    let mut secret_key_set = HashSet::new();
    for map in all_data.iter() {
        let auth_type = match map.get(&Tag::AuthType) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get auth type failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        let access_type = match map.get(&Tag::Accessibility) {
            Some(Value::Number(res)) => res,
            _ => {
                loge!("get access type failed!");
                return Err(ErrCode::SqliteError);
            },
        };
        // filter auth type
        if *auth_type == AuthType::Any as u32 {
            secret_key_set.insert((*auth_type, *access_type));
        }
    }
    // use secret key to get challenge
    let mut challenge_vec = Vec::new();
    // todo 遍历每一个密钥，获取challenge
    let challenge_seperator = b'_';
    for (idx, (auth_type, access_type)) in secret_key_set.iter().enumerate() {
        let tmp_challenge = common::init_decrypt(calling_info, query, auth_type, access_type)?;
        challenge_vec.extend(tmp_challenge);
        if idx < secret_key_set.len() - 1 {
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
