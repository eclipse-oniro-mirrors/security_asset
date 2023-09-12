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

// use std::mem::size_of;
use std::ptr::null_mut;

/// HuksErrcode type
pub type HuksErrcode = i32;
/// HuksErrcode Success
pub const HKS_SUCCESS: HuksErrcode = 0;
/// HuksErrcode Failure
pub const HKS_FAILURE: HuksErrcode = -1;

/// Huks tag type uint
pub const HKS_TAG_TYPE_UINT: u32 = 2 << 28;
/// Huks tag type bytes
pub const HKS_TAG_TYPE_BYTES: u32 = 5 << 28;
/// Huks tag algorithm
pub const HKS_TAG_ALGORITHM: u32 = HKS_TAG_TYPE_UINT | 1;
/// Huks tag purpose
pub const HKS_TAG_PURPOSE: u32 = HKS_TAG_TYPE_UINT | 2;
/// Huks tag key size
pub const HKS_TAG_KEY_SIZE: u32 = HKS_TAG_TYPE_UINT | 3;
/// Huks tag digest
pub const HKS_TAG_DIGEST: u32 = HKS_TAG_TYPE_UINT | 4;
/// Huks tag padding
pub const HKS_TAG_PADDING: u32 = HKS_TAG_TYPE_UINT | 5;
/// Huks tag block mode
pub const HKS_TAG_BLOCK_MODE: u32 = HKS_TAG_TYPE_UINT | 6;
/// Huks tag associated data
pub const HKS_TAG_ASSOCIATED_DATA:u32 = HKS_TAG_TYPE_BYTES | 8;
/// Huks tag nonce
pub const HKS_TAG_NONCE: u32 = HKS_TAG_TYPE_BYTES | 9;
/// Huks tag ae tag
pub const HKS_TAG_AE_TAG: u32 = HKS_TAG_TYPE_BYTES | 10009;

/// Huks key algorithm aes
pub const HKS_ALG_AES: u32 = 20;

/// Huks key purpose encrypt
pub const HKS_KEY_PURPOSE_ENCRYPT: u32 = 1;
/// Huks key purpose decrypt
pub const HKS_KEY_PURPOSE_DECRYPT: u32 = 2;

/// Huks key size 256
pub const HKS_AES_KEY_SIZE_256: u32 = 256;

/// Huks key padding none
pub const HKS_PADDING_NONE: u32 = 0;

/// Huks key digest none
pub const HKS_DIGEST_NONE:u32 = 0;

/// Huks cipher mode gcm
pub const HKS_MODE_GCM: u32 = 32;

/// Some const variables huks encrypt used
/// Aes common size
pub const AES_COMMON_SIZE: u32 = 1024;
/// Aad size
pub const AAD_SIZE: u32 = 16;
/// Nonce size
pub const NONCE_SIZE: u32 = 12;
/// Aead size
pub const AEAD_SIZE: u32 = 16;
/// Times
pub const TIMES: u32 = 4;
/// Max update size
pub const MAX_UPDATE_SIZE: u32 = 64;
/// Max outdata size
pub const MAX_OUTDATA_SIZE: u32 = MAX_UPDATE_SIZE * TIMES;


/// HksBlob struct
#[repr(C)]
#[derive(Copy, Clone)]
pub struct HksBlob{
    /// HksBlob size
    pub size: u32,
    /// HksBlob data pointer
    pub data: *const u8,
}
/// HksParamSet struct
#[repr(C)]
pub struct HksParamSet{
    /// HksParamSet size
    pub param_set_size: u32,
    /// HksParamSet count
    pub params_cnt: u32,
    /// HksParamSet params pointer
    pub params: *mut HksParam,
}
impl HksParamSet{
    /// New a HksParamSet object
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

/// HksParam struct
#[repr(C)]
#[derive(Copy, Clone)]
pub struct HksParam{
    /// HksParam tag
    pub tag: u32,
    /// HksParam union types
    pub union_1: HksParam_union_1,
}

/// HksParam_union_1 union
#[repr(C)]
#[derive(Copy, Clone)]
pub union HksParam_union_1{
    /// HksParam bool param
    pub bool_param: bool,
    /// HksParam int32 param
    pub int32_param: i32,
    /// HksParam uint32 param
    pub uint32_param: u32,
    /// HksParam uint64 param
    pub uint64_param: u64,
    /// HksParam blob param
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

    /// c key exist
    pub fn HksKeyExist(key_alias: *const HksBlob, param_set: *const HksParamSet) -> HuksErrcode;

    /// c init paramset
    pub fn HksInitParamSet(param_set: *mut *mut HksParamSet) -> HuksErrcode;

    /// c addparams
    pub fn HksAddParams(param_set: *mut HksParamSet,params: *const HksParam, param_cnt: u32) -> HuksErrcode;

    /// c free paramset
    pub fn HksFreeParamSet(param_set: *mut *mut HksParamSet);

    /// c build paramset
    pub fn HksBuildParamSet(param_set: *mut *mut HksParamSet) -> HuksErrcode;

    /// c hksinit
    pub fn HksInit(keyAlias: *const HksBlob, paramSet: *const HksParamSet,handle: *mut HksBlob, token: *mut HksBlob) -> HuksErrcode;

    /// c hksupdate
    pub fn HksUpdate(handle: *const HksBlob, paramSet: *const HksParamSet, inData: *const HksBlob, outData: *mut HksBlob) -> HuksErrcode;

    /// c hksfinish
    pub fn HksFinish(handle: *const HksBlob, paramSet: *const HksParamSet, inData: *const HksBlob, outData: *mut HksBlob) -> HuksErrcode;
}