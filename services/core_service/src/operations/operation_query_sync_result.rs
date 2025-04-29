/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

//! This module is used to query the result of synchronization.

use asset_common::CallingInfo;
use asset_definition::{AssetMap, Result, SyncResult};

pub(crate) fn query_sync_result(_calling_info: &CallingInfo, _query: &AssetMap) -> Result<SyncResult> {
    // TODO: 实现逻辑
    Ok(SyncResult::default())
}