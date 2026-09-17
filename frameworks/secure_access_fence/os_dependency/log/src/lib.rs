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

//! This module defines the macros required for log printing.

use std::ffi::{c_char, CString};

use hilog_rust::hilog;

const LOG_LABEL: hilog_rust::HiLogLabel = hilog_rust::HiLogLabel { 
    log_type: hilog_rust::LogType::LogCore, domain: 0xD002F21, tag: "SAF" 
};

/// the function to print log, and may not be used instead of logi
pub fn log_func_i(log: &str) {
    hilog_rust::info!(LOG_LABEL, "{}", @public(log));
}

/// the function to print log, and may not be used instead of logw
pub fn log_func_w(log: &str) {
    hilog_rust::warn!(LOG_LABEL, "{}", @public(log));
}

/// the function to print log, and may not be used instead of loge
pub fn log_func_e(log: &str) {
    hilog_rust::error!(LOG_LABEL, "{}", @public(log));
}

/// the function to print log, and may not be used instead of logd
pub fn log_func_d(log: &str) {
    hilog_rust::debug!(LOG_LABEL, "{}", @public(log));
}

/// Print logs at the info level.
///
/// # Examples
///
/// ```
/// logi!("hello, {}", "world");
/// ```
#[macro_export]
macro_rules! logi {
    ($($arg:tt)*) => (
        $crate::log_func_i(&format!($($arg)*));
    );
}

/// Print logs at the info level.
///
/// # Examples
///
/// ```
/// logw!("hello, {}", "world");
/// ```
#[macro_export]
macro_rules! logw {
    ($($arg:tt)*) => (
        $crate::log_func_w(&format!($($arg)*));
    );
}

/// Print logs at the error level.
///
/// # Examples
///
/// ```
/// loge!("Error message: {}", "read file failed");
/// ```
#[macro_export]
macro_rules! loge {
    ($($arg:tt)*) => (
        $crate::log_func_e(&format!($($arg)*));
    );
}

/// Print logs at the debug level.
///
/// # Examples
///
/// ```
/// logd!("Debug message: {}", "read file failed");
/// ```
#[macro_export]
macro_rules! logd {
    ($($arg:tt)*) => (
        $crate::log_func_d(&format!($($arg)*));
    );
}
