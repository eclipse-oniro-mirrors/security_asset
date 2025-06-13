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

//! This module is used to Asset service task manager.

/// Manages the count.
use std::sync::{Arc, Mutex};

use ylong_runtime::task::JoinHandle;

/// Manager asset tasks execute state.
pub struct TaskManager {
    task_pool: Vec<JoinHandle<()>>
}

impl TaskManager {
    fn new() -> Self {
        Self { task_pool: vec![] }
    }

    /// Get the single instance of TaskManager.
    pub fn get_instance() -> Arc<Mutex<TaskManager>> {
        static mut INSTANCE: Option<Arc<Mutex<TaskManager>>> = None;
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(Mutex::new(TaskManager::new()))).clone() }
    }

    /// Push task.
    pub fn push_task(&mut self, join_handle: JoinHandle<()>) {
        self.task_pool.push(join_handle);
    }

    /// Is task_pool empty.
    pub fn is_empty(&mut self) -> bool {
        self.task_pool.retain(|handle| !handle.is_finished());
        self.task_pool.is_empty()
    }
}
