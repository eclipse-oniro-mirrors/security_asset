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

//! This file implement the asset param check

use std::fs;
use asset_common::{definition::{ErrCode, Result}, loge};

const PATH: &str = "data/service/el1/public/asset_service";

pub(crate) fn create_user_db_dir(user_id: u32) -> Result<()>
{
    fs::create_dir(format!("{}/{}", PATH, user_id)).map_err(|_| {
        loge!("create dir failed!");
        ErrCode::Failed
    })
}