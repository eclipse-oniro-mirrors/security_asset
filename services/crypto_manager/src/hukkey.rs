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
use asset_common::{definition::{ErrCode,Result},asset_log_error};
use hilog_rust::{hilog, HiLogLabel, LogType};
use std::ffi::{c_char, CString};
use std::ptr::{null_mut,copy_nonoverlapping};
use std::mem::align_of;
use std::alloc::{alloc,dealloc,Layout};


/// KeyInfo struct
pub struct KeyInfo {
    /// User id
    pub user_id: u32,
    /// Uid
    pub uid: u32,
    /// Auth_type
    pub auth_type: u32,
    /// Access_type
    pub access_type: u32,
}
/// SecretKey struct
pub struct SecretKey{
    /// SecretKey alias
    pub alias: String,
}
impl SecretKey{
    /// New a secret key
    pub fn new(info: KeyInfo) -> Self{
        Self { alias: format!("{}_{}_{}_{}",info.user_id,info.uid,info.auth_type,info.access_type) }
    }

    /// Generate the hukkey
    pub fn generate(&mut self, mut gen_param_set: HksParamSet) -> Result<(Box<HksBlob>,Box<HksParamSet>,HuksErrcode)>{
        let hks_blob = Box::new(
            HksBlob{
            size: self.alias.len() as u32,
            data: &mut self.alias as *mut _ as *mut u8,
        });
        let key_alias: *const HksBlob = Box::into_raw(hks_blob);
        let ret = unsafe{HksGenerateKey(key_alias, &gen_param_set as *const HksParamSet, null_mut())};
        unsafe{Ok((Box::from_raw(key_alias as *mut HksBlob),Box::from_raw(&mut gen_param_set as *mut HksParamSet),ret))}
    }

    /// Delete the hukkey
    pub fn delete(&self,hks_blob: Box<HksBlob>, hks_param_set: Box<HksParamSet>) -> HuksErrcode{
        let key_alias: *const HksBlob = Box::into_raw(hks_blob);
        let param_set: *const HksParamSet = Box::into_raw(hks_param_set);
        unsafe{HksDeleteKey(key_alias, param_set)}
    }

}

/// Init param set
pub fn InitParamSet(param_set:&mut &mut HksParamSet, params:&HksParam, paramcount:u32) -> HuksErrcode{
    let mut ret: HuksErrcode = unsafe{HksInitParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet)};
    if ret != HKS_SUCCESS {
        asset_log_error!("HksInitParamSet failed");
        return ret;
    }
    ret = unsafe{HksAddParams((*param_set) as *mut HksParamSet, params as *const HksParam, paramcount)};
    if ret != HKS_SUCCESS {
        asset_log_error!("HksAddParams failed");
        unsafe{
            HksFreeParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet);
        }
        return ret;
    }

    ret = unsafe{HksBuildParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet)};
    if ret != HKS_SUCCESS {
        asset_log_error!("HksBuildParamSet failed!");
        unsafe{
            HksFreeParamSet(param_set as *mut &mut HksParamSet as *mut *mut HksParamSet);
        }
        return ret;
    }

    HKS_SUCCESS
}

/// Test update loop finish
pub fn TestUpdateLoopFinish(handle:&HksBlob, param_set:&HksParamSet, indata:&mut Box<HksBlob>, outdata:&mut Box<HksBlob>) -> HuksErrcode{
    let last_ptr = unsafe{indata.data.add(indata.size as usize - 1)};
    let param_set_ptr = param_set as *const HksParamSet;
    let mut out_data_seg = HksBlob{
        size: MAX_OUTDATA_SIZE,
        data: null_mut()
    };
    let mut cur = outdata.data;
    outdata.size = 0;

    indata.size = MAX_UPDATE_SIZE;

    unsafe{
        while indata.data <= last_ptr{
            if indata.data.add(MAX_UPDATE_SIZE as usize) <= last_ptr{
                out_data_seg.size = MAX_OUTDATA_SIZE;
            } else {
                indata.size = last_ptr.offset_from(indata.data) as u32 + 1;
                break;
            }
            if MallocAndCheckBlobData(&mut out_data_seg) != HKS_SUCCESS{
                return HKS_FAILURE;
            }

            if HksUpdate(handle as *const HksBlob, param_set_ptr, Box::as_mut(indata) as *mut HksBlob as *const HksBlob, &mut out_data_seg as *mut HksBlob) != HKS_SUCCESS{
                asset_log_error!("HksUpdate Failed.");
                let layout = Layout::from_size_align(out_data_seg.size as usize,align_of::<u32>()).unwrap();
                dealloc(out_data_seg.data,layout);
                return HKS_FAILURE;
            }
            copy_nonoverlapping(out_data_seg.data as *const u8, cur, out_data_seg.size as usize);
            cur = cur.add(out_data_seg.size as usize);
            outdata.size += out_data_seg.size;
            let layout = Layout::from_size_align(out_data_seg.size as usize,align_of::<u32>()).unwrap();
            dealloc(out_data_seg.data,layout);
            if indata.data.add(MAX_UPDATE_SIZE as usize) > last_ptr {
                return HKS_FAILURE;
            }
            indata.data = indata.data.add(MAX_UPDATE_SIZE as usize);
        }
    }

    let mut out_data_finish = HksBlob{
        size: indata.size * TIMES,
        data: null_mut()
    };
    if MallocAndCheckBlobData(&mut out_data_finish) != HKS_SUCCESS{
        return HKS_FAILURE;
    }

    unsafe{
        if HksFinish(handle as *const HksBlob, param_set_ptr, Box::as_mut(indata) as *mut HksBlob as *const HksBlob, &mut out_data_finish as *mut HksBlob) != HKS_SUCCESS{
            let layout = Layout::from_size_align(out_data_finish.size as usize,align_of::<u32>()).unwrap();
            dealloc(out_data_finish.data,layout);
            return HKS_FAILURE;
        }
    }

    unsafe{
        copy_nonoverlapping(out_data_finish.data as *const u8, cur, out_data_finish.size as usize);
    }
    outdata.size += out_data_finish.size;
    let layout = Layout::from_size_align(out_data_finish.size as usize,align_of::<u32>()).unwrap();
    unsafe{
        dealloc(out_data_finish.data,layout);
    }

    HKS_SUCCESS
}

fn MallocAndCheckBlobData(blob: &mut HksBlob) -> HuksErrcode{
    unsafe{
        let layout = Layout::from_size_align(blob.size as usize,align_of::<u32>()).unwrap();
        blob.data = alloc(layout);
        if blob.data.is_null(){
            asset_log_error!("could not alloc memory");
            return HKS_FAILURE;
        }
    }
    HKS_SUCCESS
}

/// Crypto struct
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
    /// Encrypt
    pub fn encrypt(key_alias: &HksBlob,gen_param_set: &HksParamSet, encrypt_param_set: HksParamSet, msg: &mut Vec<u8>) -> Result<Box<Vec<u8>>>{
        let mut handle_e: Box<Vec<u8>> = Box::new(vec![0,0,0,0,0,0,0,0]);
        let mut handle_encrypt = Box::new(
            HksBlob{
                size: 8,
                data: &mut handle_e[0] as *mut _ as *mut u8,
            }
        );

        let mut ret = unsafe{HksInit(key_alias as *const HksBlob, &encrypt_param_set as *const HksParamSet, Box::as_mut(&mut handle_encrypt) as *mut HksBlob, null_mut())};
        if ret != HKS_SUCCESS{
            asset_log_error!("Init failed.");
            return Err(ErrCode::Failed);
        }
        let mut indata = Box::new(
            HksBlob {
                size: msg.len() as u32,
                data: &mut (*msg)[0] as *mut _ as *mut u8
            }
        );
        let mut cipher: Box<Vec<u8>> = Box::new(vec![0;AES_COMMON_SIZE as usize]);
        let mut cipher_text = Box::new(
            HksBlob{
                size: AES_COMMON_SIZE,
                data: &mut cipher[0] as *mut _ as *mut u8,
            }
        );
        ret = TestUpdateLoopFinish(Box::as_ref(&handle_encrypt), &encrypt_param_set, &mut indata, &mut cipher_text);
        if ret != HKS_SUCCESS{
            asset_log_error!("TestUpdateLoopFinish failed.");
            return Err(ErrCode::Failed);
        }

        if ret != HKS_SUCCESS{
            unsafe{HksDeleteKey(key_alias as *const HksBlob, gen_param_set as *const HksParamSet)};
            return Err(ErrCode::Failed);
        }
        Ok(cipher)
    }

    /// Decrypt
    pub fn decrypt(key_alias: &HksBlob, gen_param_set: &HksParamSet, decrypt_param_set: HksParamSet, cipher: &mut Vec<u8>) -> Result<Box<Vec<u8>>>{
        let mut handle_d: Box<Vec<u8>> = Box::new(vec![0,0,0,0,0,0,0,0]);
        let mut handle_decrypt = Box::new(
            HksBlob{
                size: 8,
                data: &mut handle_d[0] as *mut _ as *mut u8,
            }
        );

        let mut ret = unsafe{HksInit(key_alias as *const HksBlob, &decrypt_param_set as *const HksParamSet, Box::as_mut(&mut handle_decrypt) as *mut HksBlob, null_mut())};
        if ret != HKS_SUCCESS{
            asset_log_error!("Init failed.");
            return Err(ErrCode::Failed);
        }
        let mut cipher_text = Box::new(
            HksBlob {
                size: cipher.len() as u32,
                data: &mut (*cipher)[0] as *mut _ as *mut u8
            }
        );
        let mut plain: Box<Vec<u8>> = Box::new(vec![0;AES_COMMON_SIZE as usize]);
        let mut plain_text = Box::new(
            HksBlob{
                size: AES_COMMON_SIZE,
                data: &mut plain[0] as *mut _ as *mut u8,
            }
        );
        ret = TestUpdateLoopFinish(Box::as_ref(&handle_decrypt), &decrypt_param_set, &mut cipher_text, &mut plain_text);
        if ret != HKS_SUCCESS{
            asset_log_error!("TestUpdateLoopFinish failed.");
            return Err(ErrCode::Failed);
        }

        if ret != HKS_SUCCESS{
            unsafe{HksDeleteKey(key_alias as *const HksBlob, gen_param_set as *const HksParamSet)};
            return Err(ErrCode::Failed);
        }
        Ok(plain)
    }
}