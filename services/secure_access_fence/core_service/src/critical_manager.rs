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

//! Synchronizes the SAF process critical state with the memory manager.

use std::sync::{Mutex, OnceLock};

use system_ability_fwk::ability::Handler;

const MEMORY_MANAGER_SA_ID: i32 = 1909;
const START_STATUS: i32 = 1;
const DEAD_STATUS: i32 = 0;

extern "C" {
    fn CheckMemoryMgr() -> bool;
    fn NotifyStatus(status: i32) -> i32;
    fn SetCritical(critical: bool) -> i32;
}

#[derive(Default)]
struct CriticalState {
    notified: bool,
    // None means the state has not been successfully reported yet.
    reported: Option<bool>,
    requested: bool,
    stopped: bool,
}

impl CriticalState {
    fn sync(&mut self) {
        if self.stopped {
            return;
        }
        if !self.notified {
            if unsafe { !CheckMemoryMgr() } || unsafe { NotifyStatus(START_STATUS) } != 0 {
                return;
            }
            self.notified = true;
        }
        if self.reported == Some(self.requested) {
            return;
        }
        if self.reported.is_none() && !self.requested {
            if unsafe { SetCritical(true) } != 0 {
                return;
            }
            self.reported = Some(true);
        }
        if unsafe { SetCritical(self.requested) } == 0 {
            self.reported = Some(self.requested);
        }
    }
}

/// Serializes critical updates and restores them when the memory manager restarts.
pub(crate) struct CriticalManager {
    state: Mutex<CriticalState>,
}

impl CriticalManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<CriticalManager> = OnceLock::new();
        INSTANCE.get_or_init(|| Self { state: Mutex::new(CriticalState::default()) })
    }

    pub(crate) fn set_critical(&self, critical: bool) {
        let mut state = self.state.lock().unwrap();
        if state.stopped {
            return;
        }
        state.requested = critical;
        state.sync();
    }

    pub(crate) fn stop(&self) {
        let mut state = self.state.lock().unwrap();
        state.stopped = true;
        state.requested = false;
        if state.reported == Some(true) && unsafe { SetCritical(false) } == 0 {
            state.reported = Some(false);
        }
        if state.notified && unsafe { NotifyStatus(DEAD_STATUS) } == 0 {
            state.notified = false;
        }
    }

    pub(crate) fn subscribe(&self, handler: &Handler) {
        handler.add_system_ability_listen(MEMORY_MANAGER_SA_ID);
    }

    pub(crate) fn on_system_ability_load(&self, said: i32) {
        if said == MEMORY_MANAGER_SA_ID {
            self.state.lock().unwrap().sync();
        }
    }

    pub(crate) fn on_system_ability_remove(&self, said: i32) {
        if said == MEMORY_MANAGER_SA_ID {
            let mut state = self.state.lock().unwrap();
            state.notified = false;
            state.reported = None;
        }
    }
}
