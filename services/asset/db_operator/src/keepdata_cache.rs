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

//! This module manages the keepdata cache, which records the owners whose critical
//! assets are retained on uninstall (PACKAGE_REMOVED with keepData=true) until the
//! deferred cleanup event (PACKAGE_UNINSTALLED_DATA_CLEARED) arrives. The cache is
//! persisted at `{DE_ROOT_PATH}/{user_id}/keepdata.cache`, following the exact
//! same patterns as the upgrade cache in database_file_upgrade.rs.

use std::fs::File;
use std::fs::OpenOptions;
use std::{
    fs,
    os::unix::fs::{OpenOptionsExt, PermissionsExt},
    path::Path,
    sync::Mutex,
};

use asset_definition::{macros_lib, ErrCode, Result};
use asset_log::logi;
use lazy_static::lazy_static;
use serde::{Deserialize, Serialize};
use ylong_json::{from_reader, to_writer};

use crate::database::DE_ROOT_PATH;

lazy_static! {
    static ref KEEPDATA_FILE_LOCK: Mutex<()> = Mutex::new(());
}

const FIELD_SEPARATOR: char = '|';
const GROUP_ID_SEPARATOR: char = ',';

/// A retained owner record, written at keepData uninstall and consumed at the
/// clear-data event. `bundle_name` is the join key of the clear-data event;
/// `developer_id` and `group_ids` are recorded only when `app_index == 0`
/// and serve as the group-retention credential (empty = absent).
#[derive(Clone, Debug, PartialEq, Default)]
pub struct KeepdataEntry {
    /// The bundle name, used to match the clear-data event.
    pub bundle_name: String,
    /// The app id at uninstall time, used to locate the owner db.
    pub app_id: String,
    /// The app index, distinguishing clone apps.
    pub app_index: i32,
    /// The developer id, recorded only when app_index == 0.
    pub developer_id: String,
    /// The group ids the app belongs to, recorded only when app_index == 0.
    pub group_ids: Vec<String>,
}

impl KeepdataEntry {
    /// Encode into a single line: `bundle|appId|appIndex|devId|gids`.
    fn encode(&self) -> String {
        format!(
            "{}{}{}{}{}{}{}{}{}",
            self.bundle_name,
            FIELD_SEPARATOR,
            self.app_id,
            FIELD_SEPARATOR,
            self.app_index,
            FIELD_SEPARATOR,
            self.developer_id,
            FIELD_SEPARATOR,
            self.group_ids.join(&GROUP_ID_SEPARATOR.to_string())
        )
    }

    /// Decode from a single line; returns None on malformed input.
    fn decode(line: &str) -> Option<KeepdataEntry> {
        let fields: Vec<&str> = line.split(FIELD_SEPARATOR).collect();
        if fields.len() != 5 {
            return None;
        }
        let app_index = fields[2].parse::<i32>().ok()?;
        Some(KeepdataEntry {
            bundle_name: fields[0].to_string(),
            app_id: fields[1].to_string(),
            app_index,
            developer_id: fields[3].to_string(),
            group_ids: if fields[4].is_empty() {
                Vec::new()
            } else {
                fields[4].split(GROUP_ID_SEPARATOR).map(|s| s.to_string()).collect()
            },
        })
    }
}

/// The cache persisted as JSON, same pattern as UpgradeData.
#[derive(Debug, Serialize, Deserialize, PartialEq, Default)]
struct KeepdataCache {
    /// Same field pattern as UpgradeData.upgrade_list.
    entries: Vec<String>,
}

#[inline(always)]
fn fmt_file_path(user_id: i32) -> String {
    format!("{}/{}/keepdata.cache", DE_ROOT_PATH, user_id)
}

/// Get content from the keepdata cache file.
fn get_file_content(user_id: i32) -> Result<KeepdataCache> {
    let _lock = KEEPDATA_FILE_LOCK.lock().unwrap();
    let path = fmt_file_path(user_id);
    let file_path = Path::new(&path);
    let _ = fs::set_permissions(file_path, fs::Permissions::from_mode(0o640));
    let file = File::open(path)?;
    match from_reader(file) {
        Ok(content) => Ok(content),
        Err(_) => macros_lib::log_throw_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::FileOperationError,
            "Get content from keepdata cache failed."
        ),
    }
}

/// Get content, degrading to an empty cache when the file is missing or damaged.
fn get_file_content_or_default(user_id: i32) -> KeepdataCache {
    match get_file_content(user_id) {
        Ok(content) => content,
        Err(_) => KeepdataCache::default(),
    }
}

/// Decode all entries from the cache.
fn get_entries(user_id: i32) -> Vec<KeepdataEntry> {
    let content = get_file_content_or_default(user_id);
    content.entries.iter().filter_map(|line| KeepdataEntry::decode(line)).collect()
}

/// Save entries to the keepdata cache file. An empty list removes the file.
fn save_entries(user_id: i32, entries: &[KeepdataEntry]) -> Result<()> {
    let _lock = KEEPDATA_FILE_LOCK.lock().unwrap();
    let path_str = fmt_file_path(user_id);
    let file_path = Path::new(&path_str);
    if entries.is_empty() {
        let _ = fs::remove_file(file_path);
        return Ok(());
    }
    let content = KeepdataCache { entries: entries.iter().map(|e| e.encode()).collect() };
    let _ = fs::set_permissions(file_path, fs::Permissions::from_mode(0o640));
    let mut file = match OpenOptions::new().write(true).create(true).truncate(true).mode(0o640).open(file_path) {
        Ok(file) => file,
        Err(_) => {
            return macros_lib::log_throw_error!(
                macros_lib::hisysevent::function!(),
                ErrCode::FileOperationError,
                "Create file failed in save keepdata cache."
            );
        },
    };
    to_writer(&content, &mut file).map_err(|e| {
        macros_lib::log_and_into_asset_error!(
            macros_lib::hisysevent::function!(),
            ErrCode::FileOperationError,
            "Write file failed in save keepdata cache. error: {}",
            e
        )
    })
}

/// Insert or refresh a keepdata entry keyed by (app_id, app_index).
pub fn add_keepdata_entry(user_id: i32, entry: &KeepdataEntry) -> Result<()> {
    let mut entries = get_entries(user_id);
    match entries.iter().position(|e| e.app_id == entry.app_id && e.app_index == entry.app_index) {
        Some(index) => entries[index] = entry.clone(),
        None => entries.push(entry.clone()),
    }
    logi!(
        "Keepdata entry written. bundle:{}, app_id:{}, app_index:{}",
        entry.bundle_name,
        entry.app_id,
        entry.app_index
    );
    save_entries(user_id, &entries)
}

/// Query keepdata entries by bundle name (the clear-data event join key).
pub fn get_keepdata_entries_by_bundle(user_id: i32, bundle_name: &str) -> Vec<KeepdataEntry> {
    get_entries(user_id).into_iter().filter(|e| e.bundle_name == bundle_name).collect()
}

/// Query keepdata entries carrying the given developer id.
pub fn get_keepdata_entries_by_developer(user_id: i32, developer_id: &str) -> Vec<KeepdataEntry> {
    get_entries(user_id).into_iter().filter(|e| e.developer_id == developer_id).collect()
}

/// Consume the given entries after their retained data has been cleaned.
pub fn remove_keepdata_entries(user_id: i32, entries: &[KeepdataEntry]) -> Result<()> {
    if entries.is_empty() {
        return Ok(());
    }
    let mut cache = get_entries(user_id);
    cache.retain(|e| !entries.iter().any(|t| t.app_id == e.app_id && t.app_index == e.app_index));
    save_entries(user_id, &cache)
}

/// Consume the retention mark of the app (appId/appIndex).
pub fn remove_keepdata_entry(user_id: i32, app_id: &str, app_index: i32) -> Result<()> {
    let mut cache = get_entries(user_id);
    let before = cache.len();
    cache.retain(|e| !(e.app_id == app_id && e.app_index == app_index));
    if cache.len() == before {
        return Ok(());
    }
    save_entries(user_id, &cache)
}

/// Consume the group credentials of the given groups under the developer.
pub fn remove_keepdata_group_ids(user_id: i32, developer_id: &str, group_ids: &[String]) -> Result<()> {
    if group_ids.is_empty() {
        return Ok(());
    }
    let mut cache = get_entries(user_id);
    for entry in cache.iter_mut() {
        if entry.developer_id != developer_id {
            continue;
        }
        entry.group_ids.retain(|id| !group_ids.contains(id));
    }
    save_entries(user_id, &cache)
}
