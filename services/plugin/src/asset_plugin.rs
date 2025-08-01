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

use asset_common::{CallingInfo, Counter, Group, OwnerType, TaskManager, GROUP_SEPARATOR};
use asset_db_operator::{
    database::{get_path, Database},
    database_file_upgrade::construct_splited_db_name,
    types::{column, DbMap, QueryOptions},
};
use asset_file_operator::de_operator::create_user_de_dir;
use asset_log::{loge, logi};
use asset_sdk::{
    log_throw_error,
    plugin_interface::{ExtDbMap, IAssetPlugin, IAssetPluginCtx, IAssetPluginTaskCtx, RETURN_LIMIT, RETURN_OFFSET},
    AssetError, ErrCode, Extension, Result, SyncStatus, Value,
};
use asset_utils::time;
use ylong_runtime::task::JoinHandle;
use std::{
    cell::RefCell,
    sync::{Arc, Mutex},
};

/// The asset_ext plugin.
#[derive(Default)]
pub struct AssetPlugin {
    lib: RefCell<Option<libloading::Library>>,
}

static ASSET_PLUGIN_LOCK: Mutex<()> = Mutex::new(());

unsafe impl Sync for AssetPlugin {}

impl AssetPlugin {
    fn new() -> Self {
        Self { lib: RefCell::new(None) }
    }

    /// Get the instance of AssetPlugin.
    pub fn get_instance() -> Arc<AssetPlugin> {
        static mut INSTANCE: Option<Arc<AssetPlugin>> = None;
        let _guard = ASSET_PLUGIN_LOCK.lock().unwrap();
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(AssetPlugin::new())).clone() }
    }

    /// Load the plugin.
    pub fn load_plugin(&self) -> Result<Box<dyn IAssetPlugin>> {
        unsafe {
            let _guard = ASSET_PLUGIN_LOCK.lock().unwrap();
            if self.lib.borrow().is_none() {
                logi!("start to load asset_ext plugin.");
                match libloading::Library::new("libasset_ext_ffi.z.so") {
                    Ok(lib) => *self.lib.borrow_mut() = Some(lib),
                    Err(err) => {
                        loge!("dlopen libasset_ext_ffi.z.so failed, err: {}", err);
                        return log_throw_error!(ErrCode::InvalidArgument, "dlopen failed {}", err);
                    },
                };
            }

            let Some(ref lib) = *self.lib.borrow() else {
                return log_throw_error!(ErrCode::InvalidArgument, "unexpect error");
            };

            let func = match lib
                .get::<libloading::Symbol<unsafe extern "C" fn() -> *mut dyn IAssetPlugin>>(b"_create_plugin")
            {
                Ok(func) => func,
                Err(err) => {
                    loge!("dlsym _create_plugin failed, err: {}", err);
                    return log_throw_error!(ErrCode::InvalidArgument, "dlsym failed {}", err);
                },
            };

            let plugin_ptr = func();
            if plugin_ptr.is_null() {
                loge!("_create_plugin return null.");
                return log_throw_error!(ErrCode::InvalidArgument, "_create_plugin return null.");
            }

            Ok(Box::from_raw(plugin_ptr))
        }
    }

    /// Unload plugin.
    pub fn unload_plugin(&self) {
        let _guard = ASSET_PLUGIN_LOCK.lock().unwrap();
        if self.lib.borrow().is_some() {
            *self.lib.borrow_mut() = None;
        }
    }
}

/// The asset_ext plugin context.
#[repr(C)]
pub struct AssetContext {
    /// The asset database's user id.
    pub user_id: i32,
}

/// The asset_ext plugin task context.
#[repr(C)]
pub struct AssetTaskContext {}

fn convert_db_map(attributes: &ExtDbMap) -> Result<DbMap> {
    let owner_info = attributes.get_bytes_attr(&column::OWNER)?;
    let owner_type = attributes.get_enum_attr::<OwnerType>(&column::OWNER_TYPE)?;
    let mut db_map = DbMap::new();
    db_map.insert_attr(column::OWNER, owner_info.clone());
    db_map.insert_attr(column::OWNER_TYPE, owner_type);
    Ok(db_map)
}

fn get_db_name(user_id: i32, attributes: &ExtDbMap, is_ce: bool) -> std::result::Result<String, AssetError> {
    let owner_info = attributes.get_bytes_attr(&column::OWNER)?;
    let owner_type = attributes.get_enum_attr::<OwnerType>(&column::OWNER_TYPE)?;
    let calling_info = match attributes.get(&column::GROUP_ID) {
        Some(Value::Bytes(group)) => {
            let mut parts = group.split(|&byte| byte == GROUP_SEPARATOR as u8);
            let developer_id: Vec<u8> = parts.next().unwrap().to_vec();
            let group_id: Vec<u8> = parts.next().unwrap().to_vec();
            CallingInfo::new(user_id, owner_type, owner_info.to_vec(), Some(Group { developer_id, group_id }))
        },
        _ => CallingInfo::new(user_id, owner_type, owner_info.to_vec(), None),
    };
    construct_splited_db_name(&calling_info, is_ce)
}

fn get_query_options(attrs: &ExtDbMap) -> QueryOptions {
    QueryOptions {
        offset: match attrs.get(RETURN_OFFSET) {
            Some(Value::Number(offset)) => Some(*offset),
            _ => None,
        },
        limit: match attrs.get(RETURN_LIMIT) {
            Some(Value::Number(limit)) => Some(*limit),
            _ => None,
        },
        order_by: None,
        order: None,
        amend: None,
    }
}

#[allow(dead_code)]
impl IAssetPluginCtx for AssetContext {
    /// Initializes the plugin before usage.
    fn init(&mut self, user_id: i32) -> std::result::Result<(), u32> {
        create_user_de_dir(user_id).map_err(|e| e.code as u32)?;
        self.user_id = user_id;
        Ok(())
    }

    /// Create adapt cloud table for certain asset db.
    fn create_adapt_cloud_table_for_specific_db(
        &self,
        db_info: &ExtDbMap,
        is_ce: bool,
    ) -> std::result::Result<(), u32> {
        let db_name = get_db_name(self.user_id, db_info, is_ce).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, is_ce).map_err(|e| e.code as u32)?;
        db.create_adapt_cloud_table().map_err(|e| e.code as u32)
    }

    /// Adds an asset to de db.
    fn add(&self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        let db_name = get_db_name(self.user_id, attributes, false).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, false).map_err(|e| e.code as u32)?;
        db.insert_datas(attributes).map_err(|e| e.code as u32)
    }

    /// Adds an asset to ce db.
    fn ce_add(&self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        let db_name = get_db_name(self.user_id, attributes, true).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, true).map_err(|e| e.code as u32)?;
        db.insert_datas(attributes).map_err(|e| e.code as u32)
    }

    /// Adds an asset to db in asset and adapt table.
    fn add_cloud_adapt_data(
        &self, attributes: &ExtDbMap, adapt_attributes: &ExtDbMap, is_ce: bool,
    ) -> std::result::Result<i32, u32> {
        let db_name = get_db_name(self.user_id, attributes, is_ce).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, is_ce).map_err(|e| e.code as u32)?;
        db.insert_cloud_adapt_data(attributes, adapt_attributes).map_err(|e| e.code as u32)
    }

    /// Adds an asset with replace to de db.
    fn replace(&self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32> {
        let db_name = get_db_name(self.user_id, attributes, false).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, false).map_err(|e| e.code as u32)?;
        db.replace_datas(condition, false, attributes).map_err(|e| e.code as u32)
    }

    /// Adds an asset with replace to ce db.
    fn ce_replace(&self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32> {
        let db_name = get_db_name(self.user_id, attributes, true).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, true).map_err(|e| e.code as u32)?;
        db.replace_datas(condition, false, attributes).map_err(|e| e.code as u32)
    }

    /// Queries de db.
    fn query(&self, attributes: &ExtDbMap) -> std::result::Result<Vec<ExtDbMap>, u32> {
        let de_dbs = asset_file_operator::de_operator::get_de_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut query_data = vec![];
        for db_name in de_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, false).map_err(|e| e.code as u32)?;
            query_data.extend(db.query_datas(&vec![], attributes, None, false).map_err(|e| e.code as u32)?);
        }
        Ok(query_data)
    }

    /// Queries ce db.
    fn ce_query(&self, attributes: &ExtDbMap) -> std::result::Result<Vec<ExtDbMap>, u32> {
        let ce_dbs = asset_file_operator::ce_operator::get_ce_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut query_data = vec![];
        for db_name in ce_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, true).map_err(|e| e.code as u32)?;
            query_data.extend(db.query_datas(&vec![], attributes, None, false).map_err(|e| e.code as u32)?);
        }
        Ok(query_data)
    }

    fn query_target_data(
        &self,
        db_name: &str,
        columns: &[&'static str],
        sql_where: &str,
        limit: u32,
        offset: u32,
        is_ce: bool,
    ) -> std::result::Result<Vec<ExtDbMap>, u32> {
        let mut db = Database::build_with_file_name(self.user_id, db_name, is_ce).map_err(|e| e.code as u32)?;
        let condition = ExtDbMap::new();
        let query_options = QueryOptions {
            offset: Some(offset),
            limit: Some(limit),
            order: None,
            order_by: None,
            amend: Some(sql_where.to_string()),
        };
        let query_data =
            db.query_datas(&columns.to_vec(), &condition, Some(&query_options), false).map_err(|e| e.code as u32)?;
        Ok(query_data)
    }

    /// Query db with attributes to a certain db. Normal, Group, CE.
    fn query_certain_db(
        &self,
        db_info: &ExtDbMap,
        attributes: &ExtDbMap,
        query_options: &ExtDbMap,
        is_ce: bool,
        is_filter_sync: bool,
    ) -> std::result::Result<Vec<ExtDbMap>, u32> {
        let db_name = get_db_name(self.user_id, db_info, is_ce).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, is_ce).map_err(|e| e.code as u32)?;
        db.query_datas(&vec![], attributes, Some(&get_query_options(query_options)), is_filter_sync).map_err(|e| e.code as u32)
    }

    /// Query db with attributes to a certain db. Normal, Group, CE.
    fn query_certain_db_with_connect_table(
        &self,
        db_info: &ExtDbMap,
        attributes: &ExtDbMap,
        is_ce: bool,
    ) -> std::result::Result<Vec<ExtDbMap>, u32> {
        let db_name = get_db_name(self.user_id, db_info, is_ce).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, is_ce).map_err(|e| e.code as u32)?;
        db.query_datas_with_connect_table(&vec![], attributes, None, false).map_err(|e| e.code as u32)
    }

    /// Removes an asset from de db.
    fn remove(&self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        let de_dbs = asset_file_operator::de_operator::get_de_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut total_remove_count = 0;
        for db_name in de_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, false).map_err(|e| e.code as u32)?;
            total_remove_count += db.delete_datas(attributes, None, false).map_err(|e| e.code as u32)?;
        }
        Ok(total_remove_count)
    }

    /// Removes an asset from ce db.
    fn ce_remove(&self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        let ce_dbs = asset_file_operator::ce_operator::get_ce_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut total_remove_count = 0;
        for db_name in ce_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, true).map_err(|e| e.code as u32)?;
            total_remove_count += db.delete_datas(attributes, None, false).map_err(|e| e.code as u32)?;
        }
        Ok(total_remove_count)
    }

    /// Removes assets with aliases.
    fn batch_remove(&self, attributes: &ExtDbMap, aliases: &[Vec<u8>], require_attr_encrypted: bool) -> Result<()> {
        let db_name = get_db_name(self.user_id, attributes, require_attr_encrypted)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, require_attr_encrypted)?;
        let condition = convert_db_map(attributes)?;
        let mut update_datas = DbMap::new();
        let time = time::system_time_in_millis()?;
        update_datas.insert(column::UPDATE_TIME, Value::Bytes(time));
        update_datas.insert(column::SYNC_STATUS, Value::Number(SyncStatus::SyncDel as u32));
        let total_removed_count = db.delete_batch_datas(&condition, &update_datas, aliases)?;
        logi!("total removed count = {}", total_removed_count);
        Ok(())
    }

    /// Removes an asset from a certain db. Normal, Group, CE.
    fn remove_certain_db(
        &self,
        db_info: &ExtDbMap,
        attributes: &ExtDbMap,
        is_ce: bool,
    ) -> std::result::Result<i32, u32> {
        let db_name = get_db_name(self.user_id, db_info, is_ce).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, is_ce).map_err(|e| e.code as u32)?;
        db.delete_datas(attributes, None, false).map_err(|e| e.code as u32)
    }

    /// Removes assets from de db with sepcific condition.
    fn remove_with_specific_cond(
        &self,
        specific_cond: &str,
        condition_value: &[Value],
    ) -> std::result::Result<i32, u32> {
        let de_dbs = asset_file_operator::de_operator::get_de_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut total_remove_count = 0;
        for db_name in de_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, false).map_err(|e| e.code as u32)?;
            total_remove_count +=
                db.delete_specific_condition_datas(specific_cond, condition_value).map_err(|e| e.code as u32)?;
        }
        Ok(total_remove_count)
    }

    /// Removes assets from ce db with sepcific condition.
    fn ce_remove_with_specific_cond(
        &self,
        specific_cond: &str,
        condition_value: &[Value],
    ) -> std::result::Result<i32, u32> {
        let ce_dbs = asset_file_operator::ce_operator::get_ce_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut total_remove_count = 0;
        for db_name in ce_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, true).map_err(|e| e.code as u32)?;
            total_remove_count +=
                db.delete_specific_condition_datas(specific_cond, condition_value).map_err(|e| e.code as u32)?;
        }
        Ok(total_remove_count)
    }

    /// Remove an asset to db in asset and adapt table.
    fn remove_cloud_adapt_data(
        &self,
        db_info: &ExtDbMap,
        attributes: Option<&ExtDbMap>,
        adapt_attributes: Option<&ExtDbMap>,
        is_ce: bool,
    ) -> std::result::Result<i32, u32> {
        let db_name = get_db_name(self.user_id, db_info, is_ce).map_err(|e| e.code as u32)?;
        let mut db = Database::build_with_file_name(self.user_id, &db_name, is_ce).map_err(|e| e.code as u32)?;
        db.delete_adapt_data(attributes, adapt_attributes).map_err(|e| e.code as u32)
    }

    /// Updates the attributes of an asset in de db.
    fn update(&self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> std::result::Result<i32, u32> {
        let de_dbs = asset_file_operator::de_operator::get_de_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut total_update_count = 0;
        for db_name in de_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, false).map_err(|e| e.code as u32)?;
            total_update_count += db.update_datas(attributes, false, attrs_to_update).map_err(|e| e.code as u32)?;
        }
        Ok(total_update_count)
    }

    /// Updates the attributes of an asset in ce db.
    fn ce_update(&self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> std::result::Result<i32, u32> {
        let ce_dbs = asset_file_operator::ce_operator::get_ce_user_dbs(self.user_id).map_err(|e| e.code as u32)?;
        let mut total_update_count = 0;
        for db_name in ce_dbs {
            let mut db = Database::build_with_file_name(self.user_id, &db_name, true).map_err(|e| e.code as u32)?;
            total_update_count += db.update_datas(attributes, false, attrs_to_update).map_err(|e| e.code as u32)?;
        }
        Ok(total_update_count)
    }

    /// Returns the storage path for de db.
    fn get_storage_path(&self) -> String {
        get_path()
    }
}

impl IAssetPluginTaskCtx for AssetTaskContext {
    /// Increase count
    fn increase_count(&self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().increase_count();
    }

    /// Decrease count
    fn decrease_count(&self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().decrease_count();
    }

    /// Add task
    fn add_task(&self, handle: JoinHandle<()>) {
        let task_manager = TaskManager::get_instance();
        task_manager.lock().unwrap().push_task(handle);
    }
}
