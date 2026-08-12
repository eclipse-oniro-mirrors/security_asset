/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! This module provides common utilities for SAF.

mod json_builder;
mod time;

pub use json_builder::{JsonBuilder, new_object, object_add_string, object_add_number, 
    get_compact_json_value, take_optional_string, take_required_string, take_required_value, json_into_object,
    get_string_array_from_json, get_array_from_json};

pub use time::{system_time_in_millis, system_time_in_seconds};

// Re-export ylong_json types for convenience
pub use ylong_json::{JsonValue, Object};
