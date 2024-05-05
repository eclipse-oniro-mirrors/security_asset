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

use std::any::Any;
use std::collections::HashMap;
pub use asset_definition::{Value};

/// Defines a type alias `ExtDbMap` as a `HashMap` with keys of type `&'static str` and values of type `Value`.
pub type ExtDbMap = HashMap<&'static str, Value>;

/// An enumeration representing different event types related to specific operations.
#[derive(Default, Hash, PartialEq, Eq, Clone)]
pub enum EventType {
    /// Sync operate.
    #[default]
    Sync = 0,

    /// Logout operate.
    Logout = 1,
}

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

    /// Adds an asset to the database.
    fn add(&mut self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Queries the asset database.
    fn query(&mut self, attributes: &ExtDbMap) -> Result<Vec<ExtDbMap>, u32>;

    /// Removes an asset from the database.
    fn remove(&mut self, attributes: &ExtDbMap) -> Result<i32, u32>;

    /// Updates the attributes of an asset in the database.
    fn update(&mut self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> Result<i32, u32>;

    /// Begins a transaction for the asset database.
    fn begin_transaction(&mut self) -> Result<(), u32>;

    /// Commits a transaction for the asset database.
    fn commit_transaction(&mut self) -> Result<(), u32>;

    /// Rolls back a transaction for the asset database.
    fn rollback_transaction(&mut self) -> Result<(), u32>;

    /// Returns the storage path for the asset database.
    fn get_storage_path(&self) -> String;
}

/// Defines a trait `IAssetPlugin` that specifies the required functionality for an asset plugin implementation.
pub trait IAssetPlugin: Any + Sync + Send + std::panic::RefUnwindSafe {
    /// Initialize the plugin.
    fn init(&self, ctx: Box<dyn IAssetPluginCtx>) -> Result<(), u32>;

    /// Uninitialize the plugin.
    fn uninit(&self);

    /// Process the event.
    fn process_event(&self, event_type: EventType, params: &ExtDbMap) -> Result<(), u32>;
}
