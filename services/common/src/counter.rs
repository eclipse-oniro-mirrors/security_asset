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

//! This module is used to Asset service counter.

/// Manages the count.
use std::sync::{Arc, Mutex};

/// Count asset service use times
pub struct Counter {
    count: u32,
}

impl Counter {
    fn new() -> Self {
        Self { count: 0 }
    }

    /// Get the single instance of Counter.
    pub fn get_instance() -> Arc<Mutex<Counter>> {
        static mut INSTANCE: Option<Arc<Mutex<Counter>>> = None;
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(Mutex::new(Counter::new()))).clone() }
    }

    fn increase_count(&mut self) {
        self.count += 1;
    }

    fn decrease_count(&mut self) {
        if self.count > 0 {
            self.count -= 1;
        }
    }

    /// get count.
    pub fn count(&mut self) -> u32 {
        self.count
    }
}

/// Auto count asset service use times
#[derive(Default)]
pub struct AutoCounter;

impl AutoCounter {
    /// New auto counter instance and add count
    pub fn new() -> Self {
        let counter = Counter::get_instance();
        counter.lock().unwrap().increase_count();
        Self {}
    }
}

impl Drop for AutoCounter {
    // Finish use counter.
    fn drop(&mut self) {
        let counter = Counter::get_instance();
        counter.lock().unwrap().decrease_count();
    }
}
