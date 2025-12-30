/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/// the module test for core_service
use asset_common::CallingInfo;
use asset_definition::{AssetMap, Result, SyncResult};
use crate::operations;
pub use operations::ut_operation_add_stub::*;

#[allow(dead_code)]
/// add stub
pub fn add_stub(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<()> {
    operations::add(calling_info, attributes)
}

#[allow(dead_code)]
/// pre_query_stub
pub fn pre_query_stub(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<Vec<u8>> {
    operations::pre_query(calling_info, attributes)
}

#[allow(dead_code)]
/// post_query_stub
pub fn post_query_stub(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<()> {
    operations::post_query(calling_info, attributes)
}

#[allow(dead_code)]
/// query_stub
pub fn query_stub(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<Vec<AssetMap>> {
    operations::query(calling_info, attributes)
}

#[allow(dead_code)]
/// query_sync_stub
pub fn query_sync_result_stub(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<SyncResult> {
    operations::query_sync_result(calling_info, attributes)
}

#[allow(dead_code)]
/// remove_stub
pub fn remove_stub(calling_info: &CallingInfo, attributes: &AssetMap) -> Result<()> {
    operations::remove(calling_info, attributes)
}

#[allow(dead_code)]
/// update_stub
pub fn update_stub(calling_info: &CallingInfo, attributes: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
    operations::update(calling_info, attributes, attributes_to_update)
}

