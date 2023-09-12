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

use std::fmt;

use crate::definition::{
    Result, ErrCode, Tag, DataType, Value,
    Accessibility, ReturnType, ConflictResolution, SyncType, AuthType
};

/// The mask used to obtain the data type of Asset attribute value.
const DATA_TYPE_MASK: u32 = 0xF << 28;

/// get type
pub trait GetType { // todo: getter方法的命名上不需要加上get
    /// get type
    fn get_type(&self) -> Result<DataType>;
    /// get real
    fn get_real(self) -> Value;
}

impl GetType for Tag {
    fn get_type(&self) -> Result<DataType> {
        let mask = (*self as u32) & DATA_TYPE_MASK;
        match mask {
            _ if DataType::Bool as u32 == mask => Ok(DataType::Bool),
            _ if DataType::Uint32 as u32 == mask => Ok(DataType::Uint32),
            _ if DataType::Bytes as u32 == mask => Ok(DataType::Bytes),
            _ => {
                loge!("get tag type failed!");
                Err(ErrCode::InvalidArgument)
            },
        }
    }

    fn get_real(self) -> Value {
        todo!()
    }
}

impl GetType for Accessibility {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for SyncType {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for ConflictResolution {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for ReturnType {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for bool {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Bool)
    }

    fn get_real(self) -> Value {
        Value::BOOL(self)
    }
}

impl GetType for AuthType {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Uint32)
    }

    fn get_real(self) -> Value {
        Value::NUMBER(self as u32)
    }
}

impl GetType for Vec<u8> {
    fn get_type(&self) -> Result<DataType> {
        Ok(DataType::Bytes)
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

impl fmt::Display for ErrCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // match *self {
        //     ErrCode::Ok => write!(f, "Ok"),
        //     ErrCode::Failed => write!(f, "Failed"),
        //     _ => {
        //         write!(f, "{}", *self as i32)
        //     }
        // }
        write!(f, "{}", *self as i32)
    }
}

/// Macro to implement TryFrom for enumeration types.
///
/// # Example
///
/// ```
/// impl_try_from! {
///     enum Color {
///         GREEN = 0,
///         YELLOW = 1,
///     }
/// }
/// ```
#[macro_export]
macro_rules! impl_try_from {
    ($(#[$meta:meta])* $vis:vis enum $name:ident {
        $($(#[$vmeta:meta])* $vname:ident $(= $val:expr)?,)*
    }) => {
        $(#[$meta])*
        $vis enum $name {
            $($(#[$vmeta])* $vname $(= $val)?,)*
        }

        impl std::convert::TryFrom<u32> for $name {
            type Error = $crate::definition::ErrCode;

            fn try_from(v: u32) -> std::result::Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as u32 => Ok($name::$vname),)*
                    _ => Err($crate::definition::ErrCode::InvalidArgument),
                }
            }
        }

        impl std::convert::TryFrom<i32> for $name {
            type Error = $crate::definition::ErrCode;

            fn try_from(v: i32) -> std::result::Result<Self, Self::Error> {
                match v {
                    $(x if x == $name::$vname as i32 => Ok($name::$vname),)*
                    _ => Err($crate::definition::ErrCode::InvalidArgument),
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