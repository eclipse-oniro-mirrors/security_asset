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
use crate::hukkey_ffi::*;
use asset_common_lib::{asset_type::{AssetStatusCode,AssetResult},asset_log_error};
use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};
use std::ptr::null_mut;
// use std::mem::{size_of,align_of};
// use std::alloc::{alloc,dealloc,Layout};

/// KeyInfo
pub struct KeyInfo {
    /// user id
    pub user_id: u32,
    /// uid
    pub uid: u32,
    /// auth_type
    pub auth_type: u32,
    /// access_type
    pub access_type: u32,
}
/// SecretKey
pub struct SecretKey{
    /// secret key alias
    pub alias: String,
}
impl SecretKey{
    /// new a secret key
    pub fn new(info: KeyInfo) -> Self{
        Self { alias: format!("{}_{}_{}_{}",info.user_id,info.uid,info.auth_type,info.access_type) }
    }

    /// generate the hukkey
    pub fn generate(&mut self) -> AssetResult<(Box<HksBlob>,Box<HksParamSet>)>{
        let hks_blob = Box::new(
            HksBlob{
            size: self.alias.len() as u32,
            data: &mut self.alias as *mut _ as *mut u8,
        });
        let key_alias: *const HksBlob = Box::into_raw(hks_blob);
        
        let mut genParamSet = HksParamSet::new();
        let g_genParams004:[HksParam;5] = [
            HksParam{
                tag: HKS_TAG_ALGORITHM,
                union_1: HksParam_union_1{
                    uint32_param: HKS_ALG_AES
                }
            },
            HksParam{
                tag: HKS_TAG_PURPOSE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT
                }
            },
            HksParam{
                tag: HKS_TAG_KEY_SIZE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_AES_KEY_SIZE_128
                }
            },
            HksParam{
                tag: HKS_TAG_PADDING,
                union_1: HksParam_union_1{
                    uint32_param: HKS_PADDING_NONE
                }
            },
            HksParam{
                tag: HKS_TAG_BLOCK_MODE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_MODE_GCM
                }
            }
        ];
        // let g_genParams004_ptr = g_genParams004.as_ptr();

        let ret = InitParamSet(&mut &mut genParamSet, &g_genParams004[0], 5);
        if ret != HKS_SUCCESS{
            asset_log_error!("InitParamSet(gen) failed.");
            return Err(AssetStatusCode::Failed);
        }
        unsafe{
            HksGenerateKey(key_alias, &genParamSet as *const HksParamSet, null_mut());
        }
        unsafe{Ok((Box::from_raw(key_alias as *mut HksBlob),Box::from_raw(&mut genParamSet as *mut HksParamSet)))}
    }

    /// delete the hukkey
    pub fn delete(&self,hks_blob: Box<HksBlob>, hks_param_set: Box<HksParamSet>) -> HuksErrcode{
        let key_alias: *const HksBlob = Box::into_raw(hks_blob);
        let param_set1: *const HksParamSet = Box::into_raw(hks_param_set);
        unsafe{HksDeleteKey(key_alias, param_set1)}
    }

    // pub fn HksEncrypt_func(key: *const HksBlob, paramSet: *const HksParamSet,
    //     plainText: *const HksBlob, cipherText: *mut HksBlob) -> HuksErrcode{
    //     unsafe{HksEncrypt(key, paramSet, plainText, cipherText)}
    // }
    // pub fn HksDecrypt_func(key: *const HksBlob, paramSet: *const HksParamSet,
    //     cipherText: *const HksBlob, plainText: *mut HksBlob) -> HuksErrcode{
    //     unsafe{HksDecrypt(key, paramSet, cipherText, plainText)}
    // }
}

/// init param set
pub fn InitParamSet(param_set:&mut &mut HksParamSet, params:&HksParam, paramcount:u32) -> HuksErrcode{
    let mut ret: HuksErrcode = unsafe{HksInitParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet)};
    if ret != HKS_SUCCESS {
        asset_log_error!("HksInitParamSet failed");
        return ret;
    }
    ret = unsafe{HksAddParams((*param_set) as *mut HksParamSet, params as *const HksParam, paramcount)};
    if ret != HKS_SUCCESS {
        asset_log_error!("HksAddParams failed");
        unsafe{HksFreeParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet)};
        return ret;
    }

    ret = unsafe{HksBuildParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet)};
    if ret != HKS_SUCCESS {
        asset_log_error!("HksBuildParamSet failed!");
        unsafe{HksFreeParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet)};
        return ret;
    }

    HKS_SUCCESS
}

// fn HksInitParamSet(mut param_set:*mut HksParamSet) -> HuksErrcode{
//     if param_set.is_null(){
//         asset_log_error!("invalid init params!");
//     }

//     let hks_param_set = Box::new(
//         HksParamSet{
//             params_cnt: 0,
//             param_set_size: size_of::<HksParamSet>() as u32,
//             params: null_mut::<HksParam>(),
//         }
//     );
//     param_set = Box::into_raw(hks_param_set);
//     if param_set.is_null(){
//         asset_log_error!("malloc init param set failed!");
//     }
//     HKS_SUCCESS
// }

// fn HksAddParams(param_set: *mut HksParamSet,params: *const HksParam, param_cnt: u32) -> HuksErrcode{
//     let ret: HuksErrcode = CheckBeforeAddParams(param_set, params, param_cnt);
//     if ret != HKS_SUCCESS {
//         asset_log_error!("CheckBeforeAddParams failed");
//         return ret;
//     }
    
//     unsafe{
//         if param_cnt > 0{
//             let vec_hksparam = Box::new(
//                 [HksParam{
//                     tag: 0,
//                     union_1: HksParam_union_1{
//                         uint32_param: 0
//                     }
//                 },HksParam{
//                     tag: 0,
//                     union_1: HksParam_union_1{
//                         uint32_param: 0
//                     }
//                 },HksParam{
//                     tag: 0,
//                     union_1: HksParam_union_1{
//                         uint32_param: 0
//                     }
//                 },HksParam{
//                     tag: 0,
//                     union_1: HksParam_union_1{
//                         uint32_param: 0
//                     }
//                 },HksParam{
//                     tag: 0,
//                     union_1: HksParam_union_1{
//                         uint32_param: 0
//                     }
//                 }]
//             );
//             let vec_hksparam_ptr = Box::into_raw(vec_hksparam);
//             (*param_set).params = &mut (*vec_hksparam_ptr)[0] as *mut HksParam;
//         }
//         for i in 0..=param_cnt{
//             (*param_set).param_set_size += size_of::<HksParam>() as u32;
//             if GetTagType((*params.add(i as usize)).tag) == HKS_TAG_TYPE_BYTES {
//                 if IsAdditionOverflow((*param_set).param_set_size, (*params.add(i as usize)).union_1.blob.size) {
//                     asset_log_error!("params size overflow!");
//                     (*param_set).param_set_size -= size_of::<HksParam>() as u32;
//                     return HKS_ERROR_INVALID_ARGUMENT;
//                 }
//                 (*param_set).param_set_size += (*params.add(i as usize)).union_1.blob.size;
//             }
//             *((*param_set).params.add(i as usize)) =  *params.add(i as usize);
//         }
//     }
//     HKS_SUCCESS
// }

// fn CheckBeforeAddParams(param_set: *mut HksParamSet,params: *const HksParam, param_cnt: u32) -> HuksErrcode{
//     unsafe{
//         if params.is_null() || param_set.is_null() || (*param_set).param_set_size > HKS_PARAM_SET_MAX_SIZE ||
//         param_cnt > HKS_DEFAULT_PARAM_CNT || (*param_set).params_cnt > (HKS_DEFAULT_PARAM_CNT - param_cnt) {
//             asset_log_error!("invalid params or paramset!");
//             return HKS_ERROR_INVALID_ARGUMENT;
//         }
//     }
    
//     unsafe{    
//         for i in 0..=param_cnt{
//             if GetTagType((*params.add(i as usize)).tag) == HKS_TAG_TYPE_BYTES &&
//                 (*params.add(i as usize)).union_1.blob.data.is_null(){
//                 asset_log_error!("invalid blob param!");
//                 return HKS_ERROR_INVALID_ARGUMENT;
//             }
//         }
//     }
//     HKS_SUCCESS
// }

// fn GetTagType(tag: u32) -> u32{
//     tag & HKS_TAG_TYPE_MASK
// }

// fn IsAdditionOverflow(a: u32, b: u32) -> bool{
//     (0xffffffff - a) < b
// }

// fn HksFreeParamSet(param_set:*mut HksParamSet){
//     if param_set.is_null() {
//         asset_log_error!("invalid free paramset!");
//         return;
//     }
//     unsafe{
//         drop_in_place(param_set);
//     }
// }

// fn HksBuildParamSet(param_set:*mut HksParamSet) -> HuksErrcode{
//     if param_set.is_null(){
//         return HKS_ERROR_NULL_POINTER;
//     }
    
//     let ret = unsafe{HksCheckParamSet(param_set as * const HksParamSet, (*param_set).param_set_size)};
//     if ret != HKS_SUCCESS{
//         asset_log_error!("invalid build params!");
//         return ret;
//     }

//     BuildParamSet(param_set)
// }

// fn HksCheckParamSet(param_set: *const HksParamSet, size: u32) -> HuksErrcode{
//     if param_set.is_null() {
//         return HKS_ERROR_NULL_POINTER;
//     }

//     unsafe{
//         if size < size_of::<HksParamSet>() as u32 || size > HKS_PARAM_SET_MAX_SIZE ||
//         (*param_set).param_set_size != size ||
//         (*param_set).params_cnt > ((size - size_of::<HksParamSet>() as u32) / size_of::<HksParam>() as u32) {
//             asset_log_error!("invalid param set!");
//             return HKS_ERROR_INVALID_ARGUMENT;
//         }
//     }
    
//     HKS_SUCCESS
// }

// fn BuildParamSet(param_set:*mut HksParamSet) -> HuksErrcode{
//     let mut fresh_param_set = param_set;
//     let size: u32 = unsafe{(*fresh_param_set).param_set_size};
//     let offset: u32 = unsafe{size_of::<HksParamSet>() as u32 + size_of::<HksParam>() as u32 * (*fresh_param_set).params_cnt};

//     if size > HKS_DEFAULT_PARAM_SET_SIZE {
//         let mut layout = Layout::from_size_align(size as usize,align_of::<HksParamSet>()).unwrap();
//         fresh_param_set = unsafe{alloc(layout) as *mut HksParamSet};
//         if fresh_param_set.is_null(){
//             asset_log_error!("malloc params failed!");
//             return HKS_ERROR_MALLOC_FAIL;
//         }

//         unsafe{
//             copy_nonoverlapping(param_set as *const HksParamSet,fresh_param_set, size as usize);
//             layout = Layout::from_size_align(offset as usize,align_of::<HksParamSet>()).unwrap();
//             dealloc(param_set as *mut u8,layout);
//             // param_set = fresh_param_set;
//         }
//     }

//     HksFreshParamSet(fresh_param_set)
// }

// fn HksFreshParamSet(param_set: *mut HksParamSet) -> HuksErrcode{
//     if param_set.is_null(){
//         asset_log_error!("invalid NULL paramSet");
//         return HKS_ERROR_NULL_POINTER;
//     }
//     let ret = unsafe{HksCheckParamSet(param_set as *const HksParamSet, (*param_set).param_set_size)};
//     if ret != HKS_SUCCESS{
//         asset_log_error!("invalid fresh paramSet");
//         return ret;
//     }

//     FreshParamSet(param_set)
// }

// fn FreshParamSet(param_set: *mut HksParamSet) -> HuksErrcode{
//     let size: u32 = unsafe{(*param_set).param_set_size};
//     let mut offset: u32 = unsafe{size_of::<HksParamSet>() as u32 + size_of::<HksParam>() as u32 * (*param_set).params_cnt};
//     unsafe{
//         let param_ptr = (*param_set).params;
//         for i in 0..=(*param_set).params_cnt as usize{
//             if offset > size {
//                 asset_log_error!("invalid param set offset!");
//                 return HKS_ERROR_INVALID_ARGUMENT;
//             }
//             if GetTagType((*param_ptr.add(i)).tag) == HKS_TAG_TYPE_BYTES{
//                 if IsAdditionOverflow(offset, (*param_ptr.add(i)).union_1.blob.size) {
//                     asset_log_error!("blob size overflow!");
//                     return HKS_ERROR_INVALID_ARGUMENT;
//                 }
//                 copy_nonoverlapping((*param_ptr.add(i)).union_1.blob.data,param_set.add(offset as usize) as *mut u8, (*param_ptr.add(i)).union_1.blob.size as usize);
//                 (*param_ptr.add(i)).union_1.blob.data = param_set.add(offset as usize) as *mut u8;
//                 offset += (*param_ptr.add(i)).union_1.blob.size;
//             }
//         }
    
//         if (*param_set).param_set_size != offset {
//             asset_log_error!("invalid param set size!");
//             return HKS_ERROR_INVALID_ARGUMENT;
//         }
//     }
//     HKS_SUCCESS
// }

/// Crypto
pub struct Crypto {
    // key: SecretKey,
    // mode: CryptoMode,
    // challenge: Vec<u8>,
    // handle: Vec<u8>,
    // pos: ChallengePos,
    // exp_time: u32,
}

// enum CryptoMode {
//     Encrypt,
//     Decrypt
// }

// enum ChallengePos {
//     Position0 = 0,
//     Position1 = 1,
//     Position2 = 2,
//     Position3 = 3,
// }

impl Crypto {
    /// encrypt
    pub fn encrypt(msg: &Vec<u8>) -> AssetResult<Vec<u8>>{
        let ptr = msg.as_ptr() as *mut u8;
        let len = msg.len();
        let cap = msg.capacity();
        let res = unsafe{Vec::from_raw_parts(ptr, len, cap)};
        Ok(res)
    }
    /// decrypt
    pub fn decrypt(cipher: &Vec<u8>) -> AssetResult<Vec<u8>>{
        let ptr = cipher.as_ptr() as *mut u8;
        let len = cipher.len();
        let cap = cipher.capacity();
        let res = unsafe{Vec::from_raw_parts(ptr, len, cap)};
        Ok(res)
    }
}