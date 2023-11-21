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

//! This module is used to manage the life cycle of Asset.

pub mod crypto;
pub mod crypto_manager;
mod identity_scope;
pub mod secret_key;

use asset_definition::Accessibility;

#[repr(C)]
struct HksBlob {
    size: u32,
    data: *const u8,
}

#[repr(C)]
struct OutBlob {
    size: u32,
    data: *mut u8,
}

#[repr(C)]
enum HksAuthStorageLevel {
    Ece = 1,
    Ce = 2,
    De = 3,
}

impl From<Accessibility> for HksAuthStorageLevel {
    fn from(value: Accessibility) -> Self {
        match value {
            Accessibility::DeviceUnlocked => HksAuthStorageLevel::Ece,
            Accessibility::DeviceFirstUnlocked => HksAuthStorageLevel::Ce,
            Accessibility::DevicePowerOn => HksAuthStorageLevel::De,
        }
    }
}

#[repr(C)]
struct KeyId {
    user_id: i32,
    alias: HksBlob,
    storage_level: HksAuthStorageLevel,
}

impl KeyId {
    fn new(user_id: i32, alias: HksBlob, accessibility: Accessibility) -> Self {
        let storage_level = HksAuthStorageLevel::from(accessibility);
        Self { user_id, alias, storage_level }
    }
}
