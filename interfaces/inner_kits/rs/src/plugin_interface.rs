/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

pub use asset_definition::{AssetError, Value};
use ipc::parcel::MsgParcel;
use ylong_runtime::task::JoinHandle;
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

    /// Query the result of synchronization.
    QuerySyncResult,
}

/// param name for bundle name
pub const PARAM_NAME_BUNDLE_NAME: &str = "BundleName";

/// param name for user id
pub const PARAM_NAME_USER_ID: &str = "UserId";

/// param name for app index
pub const PARAM_NAME_APP_INDEX: &str = "AppIndex";

/// param name for owner type
pub const PARAM_NAME_OWNER_TYPE: &str = "OwnerType";

/// param name for owner info
pub const PARAM_NAME_OWNER_INFO: &str = "OwnerInfo";

/// param name for developer id
pub const PARAM_NAME_DEVELOPER_ID: &str = "DeveloperId";

/// param name for group id
pub const PARAM_NAME_GROUP_ID: &str = "GroupId";

/// param name for attribute encryption type
pub const PARAM_NAME_REQUIRE_ATTR_ENCRYPTED: &str = "RequireAttrEncrypted";

/// param name for result code
pub const PARAM_NAME_RESULT_CODE: &str = "ResultCode";

/// param name for total count
pub const PARAM_NAME_TOTAL_COUNT: &str = "TotalCount";

/// param name for failed count
pub const PARAM_NAME_FAILED_COUNT: &str = "FailedCount";

/// param name for hap type
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

    /// Create adapt cloud table for certain asset db.
    fn create_adapt_cloud_table_for_specific_db(
        &self,
        db_info: &ExtDbMap,
        is_ce: bool,
    ) -> Result<(), u32>;

    /// Adds an asset to de db.
    fn add(&self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Adds an asset to ce cb.
    fn ce_add(&self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Adds an asset to db in asset and adapt table.
    fn add_cloud_adapt_data(
        &self,
        attributes: &ExtDbMap,
        adapt_attributes: &ExtDbMap,
        is_ce: bool,
    ) -> Result<i32, u32>;

    /// Adds an asset with replace to de db.
    fn replace(&self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32>;

    /// Adds an asset with replace to ce db.
    fn ce_replace(&self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32>;

    /// Queries de db.
    fn query(&self, attributes: &ExtDbMap) -> Result<Vec<ExtDbMap>, u32>;

    /// Queries ce db.
    fn ce_query(&self, attributes: &ExtDbMap) -> Result<Vec<ExtDbMap>, u32>;

    /// Query target data.
    fn query_target_data(
        &self,
        db_name: &str,
        columns: &[&'static str],
        sql_where: &str,
        limit: u32,
        offset: u32,
        is_ce: bool,
    ) -> Result<Vec<ExtDbMap>, u32>;

    /// Query db with attributes to a certain db. Normal, Group, CE.
    fn query_certain_db(
        &self,
        db_info: &ExtDbMap,
        attributes: &ExtDbMap,
        query_options: &ExtDbMap,
        is_ce: bool,
        is_filter_sync: bool,
    ) -> Result<Vec<ExtDbMap>, u32>;

    /// Query db with attributes to a certain db. Normal, CE.
    fn query_certain_db_with_connect_table(
        &self,
        db_info: &ExtDbMap,
        attributes: &ExtDbMap,
        is_ce: bool,
    ) -> Result<Vec<ExtDbMap>, u32>;

    /// Removes an asset from de db.
    fn remove(&self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Removes an asset from ce db.
    fn ce_remove(&self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Removes an asset from a certain db. Normal, Group, CE.
    fn remove_certain_db(&self, db_info: &ExtDbMap, attributes: &ExtDbMap, is_ce: bool) -> Result<i32, u32>;

    /// Removes assets from de db with specific condition.
    fn remove_with_specific_cond(&self, specific_cond: &str, condition_value: &[Value]) -> Result<i32, u32>;

    /// Removes assets from ce db with specific condition.
    fn ce_remove_with_specific_cond(&self, specific_cond: &str, condition_value: &[Value]) -> Result<i32, u32>;

    /// Removes assets from de db with aliases
    fn batch_remove(
        &self,
        attributes: &ExtDbMap,
        aliases: &[Vec<u8>],
        require_attr_encrypted: bool,
    ) -> Result<(), AssetError>;

    /// Remove an asset to db in asset and adapt table.
    fn remove_cloud_adapt_data(
        &self,
        db_info: &ExtDbMap,
        attributes: Option<&ExtDbMap>,
        adapt_attributes: Option<&ExtDbMap>,
        is_ce: bool,
    ) -> Result<i32, u32>;

    /// Updates the attributes of an asset in de db.
    fn update(&self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> Result<i32, u32>;

    /// Updates the attributes of an asset in ce db.
    fn ce_update(&self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> Result<i32, u32>;

    /// Returns the storage path for de db.
    fn get_storage_path(&self) -> String;

    /// Increase count
    fn increase_count(&self);

    /// Decrease count
    fn decrease_count(&self);

    /// Add task
    fn add_task(&self, handle: JoinHandle<()>);
}

/// Defines a trait `IAssetPlugin` that specifies the required functionality for an asset plugin implementation.
pub trait IAssetPlugin: Any + Sync + Send + std::panic::RefUnwindSafe {
    /// Initialize the plugin.
    fn init(&self, ctx: Box<dyn IAssetPluginCtx>) -> Result<(), u32>;

    /// Uninitialize the plugin.
    fn uninit(&self);

    /// Process the event.
    fn process_event(&self, event_type: EventType, params: &mut ExtDbMap) -> Result<(), u32>;

    /// Redirect request.
    fn redirect_request(&self, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> Result<(), i32>;

    /// On SA Extension.
    fn on_sa_extension(&self, extension: String, data: &mut MsgParcel, reply: &mut MsgParcel) -> Result<(), i32>;
}
