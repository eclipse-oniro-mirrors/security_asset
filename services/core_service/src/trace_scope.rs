/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

//! This module is used to Asset service hitrace.

use hitrace_meter_rust::{finish_trace, start_trace};

/// Hitrace adapter which provides timing capability.
///
/// The timing will end automatically when the structure drops. Users should
/// take care that the lifetime of this structure.
pub(crate) struct TraceScope {
    label: u64,
}

impl TraceScope {
    const HITRACE_TAG_SECURITY: u64 = 1u64 << 7;

    /// Starts tracing.
    pub(crate) fn trace(value: &str) -> Self {
        let trace = Self { label: Self::HITRACE_TAG_SECURITY };
        start_trace(trace.label, value);
        trace
    }
}

impl Drop for TraceScope {
    // Finish tracing. The timing will end automatically when the structure drops.
    fn drop(&mut self) {
        finish_trace(self.label);
    }
}
