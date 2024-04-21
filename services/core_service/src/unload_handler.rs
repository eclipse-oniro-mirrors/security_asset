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

//! This module is used to Asset service unload handler.

/// Manages the unload request.
use std::sync::{Arc, Mutex};

use ylong_runtime::task::JoinHandle;

pub(crate) struct UnloadHandler {
    task: Option<JoinHandle<()>>,
}

pub(crate) static DELAYED_UNLOAD_TIME_IN_SEC: i32 = 20;
pub(crate) static SEC_TO_MILLISEC: i32 = 1000;

impl UnloadHandler {
    fn new() -> Self {
        Self { task: None }
    }

    /// Get the single instance of UnloadHandler.
    pub(crate) fn get_instance() -> Arc<Mutex<UnloadHandler>> {
        static mut INSTANCE: Option<Arc<Mutex<UnloadHandler>>> = None;
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(Mutex::new(UnloadHandler::new()))).clone() }
    }

    /// update task in unload handler
    pub(crate) fn update_task(&mut self, new_task: JoinHandle<()>) {
        if let Some(t) = &self.task {
            t.cancel();
        };
        self.task = Some(new_task);
    }
}
