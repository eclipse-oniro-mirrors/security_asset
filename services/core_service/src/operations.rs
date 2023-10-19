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

//! This crate implements the asset operations.

mod common;
mod operation_add;
mod operation_post_query;
mod operation_pre_query;
mod operation_query;
mod operation_remove;
mod operation_update;

pub(crate) use operation_add::add;
pub(crate) use operation_post_query::post_query;
pub(crate) use operation_pre_query::pre_query;
pub(crate) use operation_query::query;
pub(crate) use operation_remove::remove;
pub(crate) use operation_update::update;
