/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

//! This module import macro use definition.

pub use asset_log::loge;
pub use crate::{AssetError, ErrCode};
pub use crate::{
    impl_tag_trait, impl_enum_trait, log_and_into_asset_error, log_throw_error, throw_error, impl_from_for_u32
};
pub use crate::{Conversion, DataType, Value};
