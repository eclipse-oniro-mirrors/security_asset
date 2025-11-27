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

//! This module defines the tool macro of the enumeration type.

/// Macro to implement TryFrom and Display for enumeration types.
///
/// # Examples
///
/// ```
/// impl_tag_trait! {
///     enum Color {
///         GREEN = 0,
///         YELLOW = 1,
///     }
/// }
/// ```
#[macro_export]
macro_rules! impl_tag_trait {
    ($(#[$meta:meta])* $vis:vis enum $name:ident {
        $($(#[$vmeta:meta])* $vname:ident $(= $val:expr)?,)*
    }) => {
        $(#[$meta])*
        $vis enum $name {
            $($(#[$vmeta])* $vname $(= $val)?,)*
        }

        impl std::convert::TryFrom<u32> for $name {
            type Error = macros_lib::AssetError;

            fn try_from(v: u32) -> std::result::Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as u32 => Ok($name::$vname),)*
                    _ => {
                        macros_lib::log_throw_error!(macros_lib::ErrCode::InvalidArgument,
                            "[FATAL]Type[{}] try from u32[{}] failed.", stringify!($name), v)
                    }
                }
            }
        }

        impl std::fmt::Display for $name {
            fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
                match self {
                    $($name::$vname => {
                        write!(f, "{}", stringify!($name::$vname))
                    },)*
                }
            }
        }
    }
}

/// Macro to implement TryFrom and Display for enumeration types.
///
/// # Examples
///
/// ```
/// impl_enum_trait! {
///     enum Color {
///         GREEN = 0,
///         YELLOW = 1,
///     }
/// }
/// ```
#[macro_export]
macro_rules! impl_enum_trait {
    ($(#[$meta:meta])* $vis:vis enum $name:ident {
        $($(#[$vmeta:meta])* $vname:ident $(= $val:expr)?,)*
    }) => {
        $(#[$meta])*
        $vis enum $name {
            $($(#[$vmeta])* $vname $(= $val)?,)*
        }

        impl std::convert::TryFrom<u32> for $name {
            type Error = macros_lib::AssetError;

            fn try_from(v: u32) -> std::result::Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as u32 => Ok($name::$vname),)*
                    _ => {
                        macros_lib::log_throw_error!(macros_lib::ErrCode::InvalidArgument,
                            "[FATAL]Type[{}] try from u32[{}] failed.", stringify!($name), v)
                    }
                }
            }
        }

        impl std::fmt::Display for $name {
            fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
                match self {
                    $($name::$vname => {
                        write!(f, "{}", stringify!($name::$vname))
                    },)*
                }
            }
        }

        impl macros_lib::Conversion for $name {
            fn data_type(&self) -> macros_lib::DataType {
                macros_lib::DataType::Number
            }

            fn into_value(self) -> macros_lib::Value {
                macros_lib::Value::Number(self as u32)
            }
        }
    }
}

/// Print log and throw AssetError.
///
/// # Examples
///
/// ```
/// log_throw_error!(ErrCode::InvalidArgument, "hello, {}", "world");
/// ```
#[macro_export]
macro_rules! log_throw_error {
    ($code:expr, $($arg:tt)*) => {{
        let str = format!($($arg)*);
        macros_lib::loge!("{}", str);
        Err(macros_lib::AssetError {
            code: $code,
            msg: str
        })
    }};
}

/// Print log and return AssetError.
///
/// # Examples
///
/// ```
/// log_and_into_asset_error!(ErrCode::InvalidArgument, "hello, {}", "world");
/// ```
#[macro_export]
macro_rules! log_and_into_asset_error {
    ($code:expr, $($arg:tt)*) => {{
        let str = format!($($arg)*);
        macros_lib::loge!("{}", str);
        macros_lib::AssetError {
            code: $code,
            msg: str
        }
    }};
}

/// Throw AssetError.
///
/// # Examples
///
/// ```
/// throw_error!(ErrCode::InvalidArgument, "hello, {}", "world");
/// ```
#[macro_export]
macro_rules! throw_error {
    ($code:expr, $($arg:tt)*) => {{
        let str = format!($($arg)*);
        Err(macros_lib::AssetError {
            code: $code,
            msg: str
        })
    }};
}

/// Impl from trait for u32.
///
/// # Examples
///
/// ```
/// impl_from_for_u32!(Accessibility);
/// ```
#[macro_export]
macro_rules! impl_from_for_u32 {
    ($t:ty) => {
        impl From<$t> for u32 {
            #[inline(always)]
            fn from(value: $t) -> Self {
                value as u32
            }
        }
    };
}
