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

use crate::asset_type::{AssetResult, AssetStatusCode, Tag, AssetType, Value, Accessibility,
    AssetReturnType, AssetConflictPolicy, AssetSyncType, AssetAuthType};
use hilog_rust::{hilog, HiLogLabel, LogType};
use ipc_rust::IpcStatusCode;

use std::ffi::{c_char, CString};
use std::fmt;

/// get type
pub trait GetType { // todo: getter方法的命名上不需要加上get
    /// get type
    fn get_type(&self) -> AssetResult<AssetType>;
    /// get real
    fn get_real(self) -> Value;
}

impl GetType for Tag {
    fn get_type(&self) -> AssetResult<AssetType> {
        let mask = (*self as u32) & (0xF << 28) as u32;
        match mask {
            _ if AssetType::Bool as u32 == mask => Ok(AssetType::Bool),
            _ if AssetType::Uint32 as u32 == mask => Ok(AssetType::Uint32),
            _ if AssetType::Bytes as u32 == mask => Ok(AssetType::Bytes),
            _ => {
                asset_log_error!("get tag type failed!");
                Err(AssetStatusCode::Failed)
            },
        }
    }

    fn get_real(self) -> Value {
        todo!()
    }
}

impl GetType for Accessibility {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for AssetSyncType {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for AssetConflictPolicy {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for AssetReturnType {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for bool {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Bool)
    }

    fn get_real(self) -> Value {
        Value::BOOL(self)
    }
}

impl GetType for AssetAuthType {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for Vec<u8> {
    fn get_type(&self) -> AssetResult<AssetType> {
        Ok(AssetType::Bytes)
    }

    fn get_real(self) -> Value {
        Value::Bytes(self)
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Value::BOOL(b) => {
                write!(f, "bool is {}", b)
            },
            Value::NUMBER(number) => {
                write!(f, "number is {}", number)
            },
            Value::Bytes(array) => {
                write!(f, "array len is {}", array.len())
            },
        }
    }
}

impl From<AssetStatusCode> for IpcStatusCode {
    fn from(value: AssetStatusCode) -> Self {
        asset_log_error!("get asset result [{}] for ipc", @public(value));
        IpcStatusCode::Failed
    }
}

impl From<IpcStatusCode> for AssetStatusCode {
    fn from(value: IpcStatusCode) -> Self {
        asset_log_error!("get ipc result [{}]", @public(value));
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
macro_rules! enum_auto_impl_try_from {
    ($(#[$meta:meta])* $vis:vis enum $name:ident {
        $($(#[$vmeta:meta])* $vname:ident $(= $val:expr)?,)*
    }) => {
        $(#[$meta])*
        $vis enum $name {
            $($(#[$vmeta])* $vname $(= $val)?,)*
        }

        impl std::convert::TryFrom<u32> for $name {
            type Error = $crate::asset_type::AssetStatusCode;

            fn try_from(v: u32) -> Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as u32 => Ok($name::$vname),)*
                    _ => Err($crate::asset_type::AssetStatusCode::Failed),
                }
            }
        }

        impl std::convert::TryFrom<i32> for $name {
            type Error = $crate::asset_type::AssetStatusCode;

            fn try_from(v: i32) -> Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as i32 => Ok($name::$vname),)*
                    _ => Err($crate::asset_type::AssetStatusCode::Failed),
                }
            }
        }
    }
}

// todo: SDK、SA共用的类型放在common，封装成静态库；只在SDK和SA层单独使用的函数需要抽出来，比如GetType只有服务层使用


// 过程宏生成display显示 枚举名 + 枚举值（i32)
// use proc_macro::TokenStream;
// use quote::quote;
// use syn::{parse_macro_input, Data, DeriveInput, Fields};

// #[proc_macro_derive(Display)]
// pub fn display_macro(input: TokenStream) -> TokenStream {
//     let ast = parse_macro_input!(input as DeriveInput);
//     let name = &ast.ident;

//     let fields = match ast.data {
//         Data::Enum(ref data) => &data.variants,
//         _ => panic!("Display macro only works with enums"),
//     };

//     let match_arms = fields.iter().map(|field| {
//         let ident = &field.ident;
//         let name = ident.as_ref().unwrap().to_string();
//         quote! {
//             #name => write!(f, #name),
//         }
//     });

//     let output = quote! {
//         impl std::fmt::Display for #name {
//             fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
//                 match self {
//                     #(#match_arms)*
//                 }
//             }
//         }
//     };

//     output.into()
// }