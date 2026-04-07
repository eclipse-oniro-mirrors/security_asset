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

//! This module implements the capability of processing the identity information of the Asset caller.

use ipc::Skeleton;
use saf_log::loge;

/// The identity of calling process.
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct CallingInfo {
    calling_uid: u64,
    access_token_id: u32,
    foreground_user_id: i32,
}

const DEFAULT_FOREGROUND_USER_ID: i32 = 0;

extern "C" {
    fn GetForegroundOsAccountId(foreground_user_id: *mut i32) -> bool;
}

impl CallingInfo {
    /// Build identity of the specified owner.
    pub fn new() -> Self {
        let mut foreground_user_id = DEFAULT_FOREGROUND_USER_ID;
        let ret = unsafe { GetForegroundOsAccountId(&mut foreground_user_id) };
        if !ret {
            loge!("[FATAL]Get foreground userId failed! use default:[{}]", DEFAULT_FOREGROUND_USER_ID);
        }
        Self { calling_uid: Skeleton::calling_uid(), access_token_id: Skeleton::calling_token_id(), foreground_user_id }
    }

    /// Get owner type of calling.
    pub fn calling_uid(&self) -> u64 {
        self.calling_uid
    }

    /// Get owner type enum of calling.
    pub fn access_token_id(&self) -> u32 {
        self.access_token_id
    }

    /// Get foreground userid.
    pub fn foreground_user_id(&self) -> i32 {
        self.foreground_user_id
    }
}

impl Default for CallingInfo {
    fn default() -> Self {
        Self::new()
    }
}
