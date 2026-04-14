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

//! This module defines the interface of the SAF Rust SDK.

use ipc::parcel::MsgParcel;
use std::any::Any;
use std::collections::HashMap;
use ylong_runtime::task::JoinHandle;

use saf_sdk::Value;

/// Defines a type alias `ExtMap` as a `HashMap` with keys of type `&'static str` and values of type `Value`.
pub type ExtMap = HashMap<&'static str, Value>;

/// An enumeration representing different event types related to specific operations.
#[derive(Default, Hash, PartialEq, Eq, Clone)]
pub enum EventType {
    /// is in black list operate, not impliment.
    #[default]
    BlackList = 0,
}

/// param name for common event type.
pub const PARAM_NAME_COMMON_EVENT_TYPE: &str = "CommonEventType";

/// param name for common event uid
pub const PARAM_NAME_COMMON_EVENT_UID: &str = "CommonEventUid";

/// param name for common event app index
pub const PARAM_NAME_COMMON_EVENT_APP_INDEX: &str = "CommonEventAppIndex";

/// param name for common event bundle name
pub const PARAM_NAME_COMMON_EVENT_BUNDLE_NAME: &str = "CommonEventBundleName";

/// param name for common event user id
pub const PARAM_NAME_COMMON_EVENT_USER_ID: &str = "CommonEventUserId";


/// Defines an interface for an saf plugin context, which outlines the basic methods for
/// an saf plugin to operate.
pub trait ISAFPluginCtx: Any + Sync + Send + std::panic::RefUnwindSafe {
    /// Initializes the plugin before usage.
    fn init(&mut self, user_id: i32) -> Result<(), u32>;

    /// Increase count
    fn increase_count(&self);

    /// Decrease count
    fn decrease_count(&self);

    /// Add task
    fn add_task(&self, handle: JoinHandle<()>);
}

/// Defines a trait `ISAFPlugin` that specifies the required functionality for an SAF plugin implementation.
pub trait ISAFPlugin: Any + Sync + Send + std::panic::RefUnwindSafe {
    /// Initialize the plugin.
    fn init(&self, ctx: Box<dyn ISAFPluginCtx>) -> Result<(), u32>;

    /// Uninitialize the plugin.
    fn uninit(&self);

    /// Process on start event.
    fn on_start(&self);

    /// Process on stop event.
    fn on_stop(&self);

    /// Process on idle event.
    fn on_idle(&self) -> i32;

    /// Get wroking request num count.
    fn get_working_request_num(&self) -> u32;

    /// Process common event.
    fn on_common_event(&self, params: &str, want: &HashMap<String, String>);

    /// Process the event.
    fn process_event(&self, event_type: EventType, params: &mut ExtMap) -> Result<ExtMap, u32>;

    /// on remote request.
    fn on_remote_request(&self, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> Result<(), i32>;

    /// On SA Extension.
    fn on_sa_extension(&self, extension: String, data: &mut MsgParcel, reply: &mut MsgParcel) -> Result<(), i32>;
}
