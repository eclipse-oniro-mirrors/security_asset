//!
//! Copyright (C) 2023 Huawei Device Co., Ltd.
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//! http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!

//! This create implement the asset

// use std::marker::PhantomData;
use std::mem::size_of;
use std::ptr::null_mut;

/// HuksErrcode
pub type HuksErrcode = i32;
pub const HKS_SUCCESS: HuksErrcode = 0;
pub const HKS_ERROR_INVALID_ARGUMENT: HuksErrcode = -3;
pub const HKS_ERROR_NULL_POINTER: HuksErrcode = -14;
pub const HKS_ERROR_MALLOC_FAIL: HuksErrcode = -21;

/// Huks Param 
pub const HKS_PARAM_SET_MAX_SIZE: u32 = 4 * 1024 * 1024;
pub const HKS_DEFAULT_PARAM_SET_SIZE: u32 = 512;
pub const HKS_DEFAULT_PARAM_CNT: u32 = (HKS_DEFAULT_PARAM_SET_SIZE - size_of::<HksParamSet>() as u32) / size_of::<HksParam>() as u32;

pub const HKS_TAG_TYPE_MASK: u32 = 0xF << 28;
pub const HKS_TAG_TYPE_BYTES: u32 = 5 << 28;
pub const HKS_TAG_TYPE_UINT: u32 = 2 << 28;
pub const HKS_TAG_ALGORITHM: u32 = HKS_TAG_TYPE_UINT | 1;
pub const HKS_TAG_PURPOSE: u32 = HKS_TAG_TYPE_UINT | 2;
pub const HKS_TAG_KEY_SIZE: u32 = HKS_TAG_TYPE_UINT | 3;
pub const HKS_TAG_PADDING: u32 = HKS_TAG_TYPE_UINT | 5;
pub const HKS_TAG_BLOCK_MODE: u32 = HKS_TAG_TYPE_UINT | 6;

/// Huks key algorithm
pub const HKS_ALG_AES: u32 = 20;


/// Huks key purpose
pub const HKS_KEY_PURPOSE_ENCRYPT: u32 = 1;
pub const HKS_KEY_PURPOSE_DECRYPT: u32 = 2;

/// Huks key size
pub const HKS_AES_KEY_SIZE_128: u32 = 128;

/// Huks key padding
pub const HKS_PADDING_NONE: u32 = 0;

/// Huks cipher mode
pub const HKS_MODE_GCM: u32 = 32;


// #[repr(C)]
// pub struct __IncompleteArrayField<T>(PhantomData<T>, [T; 0]);
// impl<T> __IncompleteArrayField<T> {
//     #[inline]
//     pub const fn new() -> Self {
//         __IncompleteArrayField(::std::marker::PhantomData, [])
//     }
// }

/// HksBlob
#[repr(C)]
#[derive(Copy, Clone)]
pub struct HksBlob{
    pub size: u32,
    pub data: *mut u8,
}
/// HksParamSet
#[repr(C)]
pub struct HksParamSet{
    pub param_set_size: u32,
    pub params_cnt: u32,
    pub params: *mut HksParam,
}
impl HksParamSet{
    pub fn new() -> Self{
        Self{
            param_set_size:0,
            params_cnt:0,
            params: null_mut::<HksParam>(),
        }
    }
}
impl Default for HksParamSet{
    fn default() -> Self {
        Self::new()
    }
}
impl Drop for HksParamSet{
    fn drop(&mut self){
        println!("HksParamSet dropping");
    }
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct HksParam{
    pub tag: u32,
    pub union_1: HksParam_union_1,
}

// impl Copy for HksParam{}
// impl Clone for HksParam{
//     fn clone(&self) -> HksParam{
//         *self
//     }
// }
// impl Copy for Vec<HksParam>{}
// impl Clone for Vec<HksParam>{
//     fn clone(&self) -> Vec<HksParam>{
//         self.clone()
//     }
// }

#[repr(C)]
#[derive(Copy, Clone)]
pub union HksParam_union_1{
    pub bool_param: bool,
    pub int32_param: i32,
    pub uint32_param: u32,
    pub uint64_param: u64,
    pub blob: HksBlob,
}

extern "C"{
    /// c generate key
    pub fn HksGenerateKey(key_alias: *const HksBlob,
        param_set_in: *const HksParamSet, param_set_out: *mut HksParamSet
    ) -> HuksErrcode;

    /// c delete key
    pub fn HksDeleteKey(key_alias: *const HksBlob, param_set: *const HksParamSet
    ) -> HuksErrcode;

    // pub fn HksEncrypt(key: *const HksBlob, paramSet: *const HksParamSet,
    //     plainText: *const HksBlob, cipherText: *mut HksBlob
    // ) -> HuksErrcode;

    // pub fn HksDecrypt(key: *const HksBlob, paramSet: *const HksParamSet,
    //     cipherText: *const HksBlob, plainText: *mut HksBlob
    // ) -> HuksErrcode;

    // pub fn HksInit(keyAlias: *const HksBlob, paramSet: *const HksParamSet,
    //     handle: *mut HksBlob, token: *mut HksBlob
    // ) -> HuksErrcode;
}