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

use super::{AssetTag, AssetStatusCode, AssetResult, AssetType};

impl TryFrom<u32> for AssetTag {
    type Error = AssetStatusCode;
    fn try_from(code: u32) -> AssetResult<Self> {
        match code {
            _ if code == AssetTag::AssetTagAlias as u32 => Ok(AssetTag::AssetTagAlias),
            _ if code == AssetTag::AssetTagAuthType as u32 => Ok(AssetTag::AssetTagAuthType),
            _ => Err(AssetStatusCode::Failed),
        }
    }
}

impl AssetTag {
    /// sss
    pub fn get_type(&self) -> AssetResult<AssetType> {
        match self {
            _ if ((*self as u32) & (AssetType::Bool as u32)) != 0 => {
                Ok(AssetType::Bool)
            }
            _ if ((*self as u32) & (AssetType::U32 as u32)) != 0 => {
                Ok(AssetType::U32)
            }
            _ if ((*self as u32) & (AssetType::Uint8Array as u32)) != 0 => {
                Ok(AssetType::Uint8Array)
            }
            _ => {
                Err(AssetStatusCode::Failed)
            }
        }
    }
}
