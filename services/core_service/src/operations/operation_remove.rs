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

//! This module is used to delete the Asset, including single and batch deletion.

use asset_db_operator::database::Database;
use asset_definition::{log_throw_error, AssetMap, ErrCode, Result};
use asset_log::logi;

use crate::{calling_info::CallingInfo, operations::common};

fn check_arguments(attributes: &AssetMap) -> Result<()> {
    let mut valid_tags = common::CRITICAL_LABEL_ATTRS.to_vec();
    valid_tags.extend_from_slice(&common::NORMAL_LABEL_ATTRS);
    valid_tags.extend_from_slice(&common::ACCESS_CONTROL_ATTRS);
    common::check_tag_validity(attributes, &valid_tags)?;
    common::check_value_validity(attributes)
}

pub(crate) fn remove(query: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(query)?;

    let mut db_data = common::into_db_map(query);
    common::add_owner_info(calling_info, &mut db_data);

    let remove_num = Database::build(calling_info.user_id())?.delete_datas(&db_data)?;
    match remove_num {
        0 => {
            log_throw_error!(ErrCode::NotFound, "[FATAL]The data to be deleted does not exist.")
        },
        n => {
            logi!("[INFO]Successfully deleted {} database records", n);
            Ok(())
        },
    }
}
