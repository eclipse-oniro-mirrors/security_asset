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

//! This module defines asset-related data structures only used in service.

#![allow(dead_code)]

/// An enum type indicates the delete type of the Asset.
pub(crate) enum DeleteType {
    Never = 0,
    WhenUninstallApp = 1 << 0,
    WhenRemoveUser = 1 << 1,
    WhenClearAppData = 1 << 2,
}

pub(crate) enum OperationCode {
    /// Code for add params.
    Add,

    /// Code for remove params.
    Remove,

    /// Code for update params.
    Update,

    /// Code for update match params.
    UpdateQuery,

    /// Code for pre-query params.
    PreQuery,

    /// Code for query params.
    Query,

    /// Code for post params.
    PostQuery,
}