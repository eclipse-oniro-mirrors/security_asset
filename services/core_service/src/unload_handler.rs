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
use std::{future::Future, mem::MaybeUninit, sync::Once, time::Duration};

use asset_common::Counter;
use asset_ipc::SA_ID;
use asset_log::{loge, logi};
use samgr::manage::SystemAbilityManager;
use ylong_runtime::{sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender}, task::JoinHandle};

pub(crate) static DELAYED_UNLOAD_TIME_IN_SEC: i32 = 20;
pub(crate) static SEC_TO_MILLISEC: i32 = 1000;

pub(crate) struct TaskManager {
    rx: TaskManagerRx,
}

pub(crate) struct TaskManagerRx {
    rx: UnboundedReceiver<i32>,
}

#[derive(Clone)]
pub struct TaskManagerTx {
    pub(crate) tx: UnboundedSender<i32>,
}

impl TaskManagerTx {
    pub(crate) fn send_event(&self, event: i32) -> bool {
        if self.tx.send(event).is_err() {
            return false;
        }
        true
    }
}

static mut APP_STATE_LISTENER: MaybeUninit<TaskManager> = MaybeUninit::uninit();
static ONCE: Once = Once::new();

impl TaskManager {
    pub(crate) fn init() {
        unsafe {
            ONCE.call_once(|| {
                APP_STATE_LISTENER.write(Self::process());
            });
        }
    }

    fn process() -> TaskManagerTx {
        let (tx, rx) = unbounded_channel();
        let tx = TaskManagerTx{ tx };
        let rx = TaskManagerRx{ rx };
        runtime_spawn(send_unload_sa_req(tx.clone()));
        rx.run();
        tx
    }
}

impl TaskManagerRx {
    async fn run(mut self) {
        loop {
            let event = match self.rx.recv().await {
                Ok(event) => event,
                Err(e) => {
                    loge!("TaskManager receives error {:?}", e);
                    continue;
                }
            };
            let counter = Counter::get_instance();
            if counter.lock().unwrap().count() > 0 {
                continue;
            }
            self.unload_sa();
        }
    }

    fn unload_sa(&self) {
        logi!("[INFO]Start unload asset service");
        SystemAbilityManager::unload_system_ability(SA_ID);
    }
}

pub(crate) fn runtime_spawn<F: Future<Output = ()> + Send + Sync + 'static>(
    fut: F,
) -> JoinHandle<()> {
    ylong_runtime::spawn(Box::into_pin(
        Box::new(fut) as Box<dyn Future<Output = ()> + Send + Sync>
    ))
}

async fn send_unload_sa_req(tx: TaskManagerTx) {
    loop {
        ylong_runtime::time::sleep(Duration::from_secs(DELAYED_UNLOAD_TIME_IN_SEC as u64)).await;
        let _ = tx.send_event(1);
    }
}

