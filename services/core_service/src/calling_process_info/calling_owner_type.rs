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

//! This crate implements the asset
#![allow(dead_code)]

/// OwnerType
pub(crate) enum OwnerType {
    Hap(Vec<u8>),
    Native(Vec<u8>)
}

impl OwnerType {
    /// xx
    pub(crate) fn get_type_num(&self) -> u32 {
        match self {
            Self::Hap(_) => {
                1
            },
            Self::Native(_) => {
                2
            }
        }
    }

    /// xx
    pub(crate) fn get_owner_text(&self) -> &Vec<u8>{
        match self {
            Self::Hap(owner_text) => {
                owner_text
            },
            Self::Native(owner_text) => {
                owner_text
            }
        }
    }
}

fn get_native_owner_info(uid: u64) -> OwnerType {
    OwnerType::Native(Vec::from(format!("{}", uid).as_bytes()))
}

/// xxx
pub(crate) fn get_calling_owner_type(uid: u64) -> OwnerType {
    // Ok(OwnerType::Native(Vec::from("123"))) // to do
    get_native_owner_info(uid)
}