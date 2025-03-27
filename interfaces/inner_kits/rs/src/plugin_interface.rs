/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

//! This module defines the interface of the Asset Rust SDK.

pub use asset_definition::Value;
use ipc::parcel::MsgParcel;
use std::any::Any;
use std::collections::HashMap;

/// Defines a type alias `ExtDbMap` as a `HashMap` with keys of type `&'static str` and values of type `Value`.
pub type ExtDbMap = HashMap<&'static str, Value>;

/// An enumeration representing different event types related to specific operations.
#[derive(Default, Hash, PartialEq, Eq, Clone)]
pub enum EventType {
    /// Sync operate.
    #[default]
    Sync = 0,

    /// Clean cloud flag.
    CleanCloudFlag = 1,

    /// Delete cloud data.
    DeleteCloudData,

    /// Device upgrade event.
    OnDeviceUpgrade,

    /// App upgrade event.
    OnAppRestore,

    /// User unlock envent.
    OnUserUnlocked,

    /// App call event.
    OnAppCall,

    /// Package clear event.
    OnPackageClear,

    /// User removed.
    OnUserRemoved,
}

/// param name for bundle name
pub const PARAM_NAME_BUNDLE_NAME: &str = "BundleName";

/// param name for user id
pub const PARAM_NAME_USER_ID: &str = "UserId";

/// param name for app index
pub const PARAM_NAME_APP_INDEX: &str = "AppIndex";

/// param name for owner type
pub const PARAM_NAME_IS_HAP: &str = "IsHap";

/// param name for return offset
pub const RETURN_OFFSET: &str = "ReturnOffset";

/// param name for return limit
pub const RETURN_LIMIT: &str = "ReturnLimit";

/// An enumeration representing different plugin types.
#[derive(Default, Hash, PartialEq, Eq, Clone)]
pub enum PluginType {
    /// Default plugin.
    #[default]
    Asset = 0,
}

/// Defines an interface for an asset plugin context, which outlines the basic methods for
/// an asset plugin to operate on an asset database.
pub trait IAssetPluginCtx: Any + Sync + Send + std::panic::RefUnwindSafe {
    /// Initializes the plugin before usage.
    fn init(&mut self, user_id: i32) -> Result<(), u32>;

    /// Adds an asset to de db.
    fn add(&mut self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Adds an asset to ce cb.
    fn ce_add(&mut self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Adds an asset with replace to de db.
    fn replace(&mut self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32>;

    /// Adds an asset with replace to ce db.
    fn ce_replace(&mut self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32>;

    /// Queries de db.
    fn query(&mut self, attributes: &ExtDbMap) -> Result<Vec<ExtDbMap>, u32>;

    /// Queries ce db.
    fn ce_query(&mut self, attributes: &ExtDbMap) -> Result<Vec<ExtDbMap>, u32>;

    /// Queries for temp db.
    fn query_temp(&mut self, db_name: &str, columns: &[&'static str], is_ce: bool) -> Result<Vec<ExtDbMap>, u32>;

    /// Query db with attributes to a certain db. Normal, Group, CE.
    fn query_certain_db(
        &mut self,
        db_info: &ExtDbMap,
        attributes: &ExtDbMap,
        query_options: &ExtDbMap,
        is_ce: bool,
    ) -> Result<Vec<ExtDbMap>, u32>;

    /// Removes an asset from de db.
    fn remove(&mut self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Removes an asset from ce db.
    fn ce_remove(&mut self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Removes an asset from a certain db. Normal, Group, CE.
    fn remove_certain_db(&mut self, db_info: &ExtDbMap, attributes: &ExtDbMap, is_ce: bool) -> Result<i32, u32>;

    /// Removes assets from de db with specific condition.
    fn remove_with_specific_cond(&mut self, specific_cond: &str, condition_value: &[Value]) -> Result<i32, u32>;

    /// Removes assets from ce db with specific condition.
    fn ce_remove_with_specific_cond(&mut self, specific_cond: &str, condition_value: &[Value]) -> Result<i32, u32>;

    /// Updates the attributes of an asset in de db.
    fn update(&mut self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> Result<i32, u32>;

    /// Updates the attributes of an asset in ce db.
    fn ce_update(&mut self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> Result<i32, u32>;

    /// Returns the storage path for de db.
    fn get_storage_path(&self) -> String;

    /// Increase count
    fn increase_count(&mut self);

    /// Decrease count
    fn decrease_count(&mut self);
}

/// Defines a trait `IAssetPlugin` that specifies the required functionality for an asset plugin implementation.
pub trait IAssetPlugin: Any + Sync + Send + std::panic::RefUnwindSafe {
    /// Initialize the plugin.
    fn init(&self, ctx: Box<dyn IAssetPluginCtx>) -> Result<(), u32>;

    /// Uninitialize the plugin.
    fn uninit(&self);

    /// Process the event.
    fn process_event(&self, event_type: EventType, params: &ExtDbMap) -> Result<(), u32>;

    /// Redirect request.
    fn redirect_request(&self, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> Result<(), i32>;

    /// On SA Extension.
    fn on_sa_extension(&self, extension: String, data: &mut MsgParcel, reply: &mut MsgParcel) -> Result<(), i32>;
}
