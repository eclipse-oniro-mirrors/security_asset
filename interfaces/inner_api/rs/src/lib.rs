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

mod asset_request;

pub use asset_common::definition;
use asset_common::{
    logi,
    definition::{AssetMap, Result},
};
use crate::asset_request::AssetProxy;

/// add an asset
pub fn add(input: AssetMap) -> Result<()> {
    logi!("[YZT][RUST SDK]enter asset add");
    AssetProxy::build()?.add(&input)
}