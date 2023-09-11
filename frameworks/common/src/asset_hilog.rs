/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

/// Print logs at the info level.
///
/// # Examples
///
/// ```
/// use std::ffi::{c_char, CString};
/// use hilog_rust::hilog;
///
/// logi!("hello, {}", "world");
/// ```
#[macro_export]
macro_rules! logi{
    ($($arg:tt)*) => (
        let log_label = hilog_rust::HiLogLabel {
            log_type: hilog_rust::LogType::LogCore,
            domain: 0xD002F70, // asset domain id
            tag: "Asset"
        };
        hilog_rust::info!(log_label, $($arg)*)
    );
}

/// Print logs at the error level.
///
/// # Examples
///
/// ```
/// use std::ffi::{c_char, CString};
/// use hilog_rust::hilog;
///
/// loge!("Error message: {}", "read file failed");
/// ```
#[macro_export]
macro_rules! loge{
    ($($arg:tt)*) => (
        let log_label = hilog_rust::HiLogLabel {
            log_type: hilog_rust::LogType::LogCore,
            domain: 0xD002F70, // security domain
            tag: "Asset"
        };
        hilog_rust::error!(log_label, $($arg)*)
    );
}
