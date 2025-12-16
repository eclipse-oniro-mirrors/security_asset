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
use asset_definition::{AssetMap, Result};
use asset_db_operator::types::DbMap;
use crate::operations::operation_add::*;


#[allow(dead_code)]
/// encrypt_secret_stub
pub fn encrypt_secret_stub(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    encrypt_secret(calling_info, db_data)
}

#[allow(dead_code)]
/// resolve_conflict_stub
pub fn resolve_conflict_stub(
    calling: &CallingInfo,
    db: &mut Database,
    attrs: &AssetMap,
    query: &DbMap,
    db_data: &mut DbMap) -> Result<()> {
    resolve_conflict(calling, db, attrs, query, db_data)
}

#[allow(dead_code)]
/// add_system_attrs_stub
pub fn add_system_attrs_stub(db_data: &mut DbMap) -> Result<()> {
    add_system_attrs(db_data)
}

#[allow(dead_code)]
/// add_default_attrs_stub
pub fn add_default_attrs_stub(db_data: &mut DbMap) {
    add_default_attrs(db_data)
}

#[allow(dead_code)]
/// check_arguments_stub
pub fn check_arguments_stub(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    check_arguments(attributes, calling_info)
}

#[allow(dead_code)]
/// modify_sync_type_stub
pub fn modify_sync_type_stub(db: &mut DbMap) -> Result<()> {
    modify_sync_type(db)
}

#[allow(dead_code)]
/// local_add_stub
pub fn local_add_stub(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    local_add(attributes, calling_info)
}
