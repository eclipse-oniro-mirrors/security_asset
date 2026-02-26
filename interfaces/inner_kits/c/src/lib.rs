/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

//! This module implements the function of Asset SDK from C to RUST.

#[cfg(not(feature = "AssetEmptyMode"))]
pub mod full;

#[cfg(feature = "AssetEmptyMode")]
pub mod empty;

#[repr(C)]
pub(crate) struct AssetResult {
    count: u32,
    attrs: *mut AssetAttr,
}

/// Attribute of Asset with a c representation.
#[repr(C)]
pub struct AssetAttr {
    tag: u32,
    value: AssetValue,
}

/// Blob of Asset with a c representation.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct AssetBlob {
    size: u32,
    data: *mut u8,
}

#[repr(C)]
pub(crate) union AssetValue {
    boolean: bool,
    uint32: u32,
    blob: AssetBlob,
}