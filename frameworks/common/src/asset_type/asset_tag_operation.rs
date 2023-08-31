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

//! 各种类型的拓展方法定义在此处

use super::{AssetResult, AssetStatusCode, AssetTag, AssetType, AssetValue};
use hilog_rust::{hilog, HiLogLabel, LogType};
use ipc_rust::IpcStatusCode;

use std::ffi::{c_char, CString};
use std::fmt;

impl AssetTag {
    /// sss
    pub fn get_type(&self) -> AssetResult<AssetType> {
        match self {
            _ if ((*self as u32) & (AssetType::Bool as u32)) != 0 => Ok(AssetType::Bool),
            _ if ((*self as u32) & (AssetType::U32 as u32)) != 0 => Ok(AssetType::U32),
            _ if ((*self as u32) & (AssetType::Uint8Array as u32)) != 0 => {
                Ok(AssetType::Uint8Array)
            },
            _ => {
                asset_log_error!("get tag type failed!");
                Err(AssetStatusCode::Failed)
            },
        }
    }
}

impl fmt::Display for AssetValue {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            AssetValue::BOOL(b) => {
                write!(f, "bool is {}", b)
            },
            AssetValue::NUMBER(number) => {
                write!(f, "number is {}", number)
            },
            AssetValue::UINT8ARRAY(array) => {
                write!(f, "array len is {}", array.len())
            },
        }
    }
}

// impl Into<IpcStatusCode> for AssetStatusCode {
//     fn into(self) -> IpcStatusCode {
//         asset_log_error!("get asset result [{}] for ipc", self);
//         IpcStatusCode::Failed
//     }
// }

impl From<AssetStatusCode> for IpcStatusCode {
    fn from(value: AssetStatusCode) -> Self {
        asset_log_error!("get asset result [{}] for ipc", value);
        IpcStatusCode::Failed
    }
}

impl From<IpcStatusCode> for AssetStatusCode {
    fn from(value: IpcStatusCode) -> Self {
        asset_log_error!("get ipc result [{}]", value);
        AssetStatusCode::IpcFailed
    }
}

impl fmt::Display for AssetStatusCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // match *self {
        //     AssetStatusCode::Ok => write!(f, "Ok"),
        //     AssetStatusCode::Failed => write!(f, "Failed"),
        //     _ => {
        //         write!(f, "{}", *self as i32)
        //     }
        // }
        write!(f, "{}", *self as i32)
    }
}

/// xxx
#[macro_export]
macro_rules! back_to_enum {
    ($(#[$meta:meta])* $vis:vis enum $name:ident {
        $($(#[$vmeta:meta])* $vname:ident $(= $val:expr)?,)*
    }) => {
        $(#[$meta])*
        $vis enum $name {
            $($(#[$vmeta])* $vname $(= $val)?,)*
        }

        impl std::convert::TryFrom<u32> for $name {
            type Error = ();

            fn try_from(v: u32) -> Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as u32 => Ok($name::$vname),)*
                    _ => Err(()),
                }
            }
        }

        impl std::convert::TryFrom<i32> for $name {
            type Error = ();

            fn try_from(v: i32) -> Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as i32 => Ok($name::$vname),)*
                    _ => Err(()),
                }
            }
        }
    }
}
