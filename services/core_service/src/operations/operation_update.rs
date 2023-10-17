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

//! This create implement the asset

use asset_common::{
    definition::{AssetMap, Result, ErrCode, Tag, Value},
    logi, loge,
};

use db_operator::{database_table_helper::{COLUMN_SECRET, COLUMN_UPDATE_TIME, DefaultDatabaseHelper}, types::DbMap};

// use crypto_manager::hukkey::Crypto;
use crate::{
    operations::operation_common::{
        encrypt, add_owner_info,
        db_adapter::into_db_map
    },
    calling_info::CallingInfo,
};

use super::operation_common::get_system_time;

fn add_system_attrs(db_data: &mut DbMap) -> Result<()> {
    let time = get_system_time()?;
    db_data.insert(COLUMN_UPDATE_TIME, Value::Bytes(time));
    Ok(())
}

pub(crate) fn update(query: &AssetMap, update: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    let mut query_db_data = into_db_map(query);
    add_owner_info(calling_info, &mut query_db_data);

    let mut update_db_data = into_db_map(update);
    add_system_attrs(&mut update_db_data)?;

    if update.contains_key(&Tag::Secret) {
        let results =
            DefaultDatabaseHelper::query_columns_default_once(calling_info.user_id(), &vec![], &query_db_data, None)?;
        if results.len() != 1 {
            loge!("query to-be-updated asset failed, found [{}] assets", results.len());
            return Err(ErrCode::NotFound);
        }

        let result = results.get(0).unwrap();
        let cipher = encrypt(calling_info, result)?;
        update_db_data.insert(COLUMN_SECRET, Value::Bytes(cipher));
    }

    // call sql to update
    let update_num =
        DefaultDatabaseHelper::update_datas_default_once(calling_info.user_id(), &query_db_data, &update_db_data)?;

    logi!("update {} data", update_num);
    Ok(())
}
