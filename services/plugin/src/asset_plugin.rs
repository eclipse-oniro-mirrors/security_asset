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

use asset_common::{CallingInfo, Counter, OwnerType};
use asset_db_operator::database::{get_path, Database};
use asset_db_key_operator::get_db_key;
use asset_definition::{log_throw_error, ErrCode, Result};
use asset_file_operator::create_user_de_dir;
use asset_log::{loge, logi};
use asset_sdk::plugin_interface::{ExtDbMap, IAssetPlugin, IAssetPluginCtx};
use std::{
    cell::RefCell,
    sync::{Arc, Mutex},
};

/// The asset_ext plugin.
#[derive(Default)]
pub struct AssetPlugin {
    lib: RefCell<Option<libloading::Library>>,
}

static ASSET_OLUGIN_LOCK: Mutex<()> = Mutex::new(());

unsafe impl Sync for AssetPlugin {}

impl AssetPlugin {
    fn new() -> Self {
        Self { lib: RefCell::new(None) }
    }

    /// Get the instance of AssetPlugin.
    pub fn get_instance() -> Arc<AssetPlugin> {
        static mut INSTANCE: Option<Arc<AssetPlugin>> = None;
        let _guard = ASSET_OLUGIN_LOCK.lock().unwrap();
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(AssetPlugin::new())).clone() }
    }

    /// Load the plugin.
    pub fn load_plugin(&self) -> Result<Box<dyn IAssetPlugin>> {
        unsafe {
            let _guard = ASSET_OLUGIN_LOCK.lock().unwrap();
            if self.lib.borrow().is_none() {
                logi!("start to load asset_ext plugin.");
                match libloading::Library::new("/system/lib64/libasset_ext_ffi.z.so") {
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

            logi!("load asset_ext plugin success.");
            Ok(Box::from_raw(plugin_ptr))
        }
    }

    /// Unload plugin.
    pub fn unload_plugin(&self) {
        let _guard = ASSET_OLUGIN_LOCK.lock().unwrap();
        if self.lib.borrow().is_some() {
            *self.lib.borrow_mut() = None;
        }
    }
}

/// The asset_ext plugin context.
#[repr(C)]
pub struct AssetContext {
    /// The asset de db instance.
    pub de_db: Option<Database>,
    /// The asset ce db instance.
    pub ce_db: Option<Database>,
}

#[allow(dead_code)]
impl IAssetPluginCtx for AssetContext {
    /// Initializes the plugin before usage.
    fn init(&mut self, user_id: u32, owner_type: u32, owner_info: Vec<u8>) -> std::result::Result<(), u32> {
        create_user_de_dir(user_id as i32).map_err(|e| e.code as u32)?;
        let de_db = Database::build(user_id as i32, None).map_err(|e| e.code as u32)?;
        self.de_db = Some(de_db);

        let owner_type = match owner_type {
            0 => OwnerType::Hap,
            1 => OwnerType::Native,
            _ => return Err(ErrCode::InvalidArgument as u32),
        };
        let calling_info = CallingInfo::new(user_id as i32, owner_type, owner_info);
        let db_key = get_db_key(&calling_info).map_err(|e| e.code as u32)?;
        let ce_db = Database::build(calling_info.user_id(), Some(&db_key)).map_err(|e| e.code as u32)?;
        self.ce_db = Some(ce_db);

        Ok(())
    }

    /// Adds an asset to de db.
    fn add(&mut self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        self.de_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .insert_datas(attributes)
            .map_err(|e| e.code as u32)
    }

    /// Adds an asset to ce db.
    fn ce_add(&mut self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        self.ce_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .insert_datas(attributes)
            .map_err(|e| e.code as u32)
    }

    /// Adds an asset with replace to de db.
    fn replace(&mut self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32> {
        self.de_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .replace_datas(condition, false, attributes)
            .map_err(|e| e.code as u32)
    }

    /// Adds an asset with replace to ce db.
    fn ce_replace(&mut self, condition: &ExtDbMap, attributes: &ExtDbMap) -> std::result::Result<(), u32> {
        self.ce_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .replace_datas(condition, false, attributes)
            .map_err(|e| e.code as u32)
    }

    /// Queries de db.
    fn query(&mut self, attributes: &ExtDbMap) -> std::result::Result<Vec<ExtDbMap>, u32> {
        self.de_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .query_datas(&vec![], attributes, None, false)
            .map_err(|e| e.code as u32)
    }

    /// Queries ce db.
    fn ce_query(&mut self, attributes: &ExtDbMap) -> std::result::Result<Vec<ExtDbMap>, u32> {
        self.ce_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .query_datas(&vec![], attributes, None, false)
            .map_err(|e| e.code as u32)
    }

    /// Removes an asset from de db.
    fn remove(&mut self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        self.de_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .delete_datas(attributes, None, false)
            .map_err(|e| e.code as u32)
    }

    /// Removes an asset from ce db.
    fn ce_remove(&mut self, attributes: &ExtDbMap) -> std::result::Result<i32, u32> {
        self.ce_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .delete_datas(attributes, None, false)
            .map_err(|e| e.code as u32)
    }

    /// Updates the attributes of an asset in de db.
    fn update(&mut self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> std::result::Result<i32, u32> {
        self.de_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .update_datas(attributes, false, attrs_to_update)
            .map_err(|e| e.code as u32)
    }

    /// Updates the attributes of an asset in ce db.
    fn ce_update(&mut self, attributes: &ExtDbMap, attrs_to_update: &ExtDbMap) -> std::result::Result<i32, u32> {
        self.ce_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .update_datas(attributes, false, attrs_to_update)
            .map_err(|e| e.code as u32)
    }

    /// Begins a transaction for de db.
    fn begin_transaction(&mut self) -> std::result::Result<(), u32> {
        self.de_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .exec("begin immediate")
            .map_err(|e| e.code as u32)
    }

    /// Begins a transaction for ce db.
    fn ce_begin_transaction(&mut self) -> std::result::Result<(), u32> {
        self.ce_db
            .as_mut()
            .ok_or(ErrCode::InvalidArgument as u32)?
            .exec("begin immediate")
            .map_err(|e| e.code as u32)
    }

    /// Commits a transaction for de db.
    fn commit_transaction(&mut self) -> std::result::Result<(), u32> {
        self.de_db.as_mut().ok_or(ErrCode::InvalidArgument as u32)?.exec("commit").map_err(|e| e.code as u32)
    }

    /// Commits a transaction for ce db.
    fn ce_commit_transaction(&mut self) -> std::result::Result<(), u32> {
        self.ce_db.as_mut().ok_or(ErrCode::InvalidArgument as u32)?.exec("commit").map_err(|e| e.code as u32)
    }

    /// Rolls back a transaction for de db.
    fn rollback_transaction(&mut self) -> std::result::Result<(), u32> {
        self.de_db.as_mut().ok_or(ErrCode::InvalidArgument as u32)?.exec("rollback").map_err(|e| e.code as u32)
    }

    /// Rolls back a transaction for ce db.
    fn ce_rollback_transaction(&mut self) -> std::result::Result<(), u32> {
        self.ce_db.as_mut().ok_or(ErrCode::InvalidArgument as u32)?.exec("rollback").map_err(|e| e.code as u32)
    }

    /// Returns the storage path for de db.
    fn get_storage_path(&self) -> String {
        get_path()
    }

    /// Returns the storage path for ce db.
    fn ce_get_storage_path(&self) -> String {
        get_path()
    }

    /// Increase count
    fn increase_count(&mut self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().increase_count();
    }

    /// Decrease count
    fn decrease_count(&mut self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().decrease_count();
    }
}
