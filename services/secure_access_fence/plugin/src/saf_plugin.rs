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

use std::{
    cell::RefCell,
    sync::{Arc, Mutex, OnceLock},
};
use ylong_runtime::task::JoinHandle;

use saf_common::{Counter, TaskManager};

use saf_log::{loge, logi, logw};
use saf_sdk::{
    macros_lib,
    ErrCode, Result,
};
use saf_plugin_interface::plugin_interface::{ISAFPlugin, ISAFPluginCtx};

/// The saf_ext plugin.
#[derive(Default)]
pub struct SAFPlugin {
    lib: RefCell<Option<libloading::Library>>,
}

static SAF_PLUGIN_LOCK: Mutex<()> = Mutex::new(());

const EXT_SO_NAME: &str = "libsecure_access_fence_ext_ffi.z.so";

unsafe impl Sync for SAFPlugin {}

impl SAFPlugin {
    fn new() -> Self {
        Self { lib: RefCell::new(None) }
    }

    /// Get the instance of SAFPlugin.
    pub fn get_instance() -> Arc<SAFPlugin> {
        static INSTANCE: OnceLock<Arc<SAFPlugin>> = OnceLock::new();
        INSTANCE.get_or_init(|| {
            logw!("Create instance for SAFPlugin.");
            Arc::new(SAFPlugin::new())
        }).clone()
    }

    /// Load the plugin.
    pub fn load_plugin(&self) -> Result<Box<dyn ISAFPlugin>> {
        unsafe {
            let _guard = SAF_PLUGIN_LOCK.lock().unwrap();
            if self.lib.borrow().is_none() {
                logi!("start to load saf_ext plugin.");
                match libloading::Library::new(EXT_SO_NAME) {
                    Ok(lib) => *self.lib.borrow_mut() = Some(lib),
                    Err(err) => {
                        return macros_lib::log_throw_error!(ErrCode::ParamVerificationFailed, "dlopen {} failed {}", EXT_SO_NAME, err);
                    },
                };
            }

            let Some(ref lib) = *self.lib.borrow() else {
                return macros_lib::log_throw_error!(ErrCode::ParamVerificationFailed, "unexpected error");
            };

            let func = match lib
                .get::<libloading::Symbol<unsafe extern "C" fn() -> *mut dyn ISAFPlugin>>(b"create_plugin_manager")
            {
                Ok(func) => func,
                Err(err) => {
                    loge!("dlsym create_plugin_manager failed, err: {}", err);
                    return macros_lib::log_throw_error!(ErrCode::ParamVerificationFailed, "dlsym failed {}", err);
                },
            };

            let plugin_ptr = func();
            if plugin_ptr.is_null() {
                loge!("create_plugin_manager return null.");
                return macros_lib::log_throw_error!(ErrCode::ParamVerificationFailed, "create_plugin_manager return null.");
            }

            Ok(Box::from_raw(plugin_ptr))
        }
    }
}

/// The saf_ext plugin context.
#[repr(C)]
pub struct SAFContext { }

#[allow(dead_code)]
impl ISAFPluginCtx for SAFContext {
    /// Initializes the plugin before usage.
    fn init(&mut self, _user_id: i32) -> std::result::Result<(), u32> {
        Ok(())
    }

    /// Increase count.
    fn increase_count(&self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().increase_count();
    }

    /// Decrease count.
    fn decrease_count(&self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().decrease_count();
    }

    /// Add task.
    fn add_task(&self, handle: JoinHandle<()>) {
        let task_manager = TaskManager::get_instance();
        task_manager.lock().unwrap().push_task(handle);
    }
}
