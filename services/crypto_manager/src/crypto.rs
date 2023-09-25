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

//! This crate implements the asset
use crate::huks_ffi::*;
// use std::ffi::c_char;
use asset_common::{definition::ErrCode, loge};
use std::alloc::{alloc, dealloc, Layout};
use std::mem::{align_of, size_of};
use std::ptr::{copy_nonoverlapping, null_mut};

/// KeyInfo struct
pub struct KeyInfo {
    /// User id
    pub user_id: u32,
    /// Uid
    pub uid: u64,
    /// Auth_type
    pub auth_type: u32,
    /// Access_type
    pub access_type: u32,
}
/// SecretKey struct
pub struct SecretKey {
    /// SecretKey alias
    pub alias: String,
}
impl SecretKey {
    /// New a secret key
    pub fn new(info: KeyInfo) -> Self {
        Self {
            alias: format!("{}_{}_{}_{}", info.user_id, info.uid, info.auth_type, info.access_type),
        }
    }

    /// Check whether the secret key exists
    pub fn exists(&self) -> HuksErrcode {
        let hks_blob = HksBlob {
            size: self.alias.len() as u32,
            data: self.alias.as_str() as *const _ as *const u8,
        };
        let key_alias = &hks_blob as *const HksBlob;
        unsafe { HksKeyExist(key_alias, null_mut()) }
    }

    /// Generate the hukkey
    pub fn generate(&self) -> HuksErrcode {
        let hks_blob = HksBlob {
            size: self.alias.len() as u32,
            data: self.alias.as_str() as *const _ as *const u8,
        };
        let key_alias = &hks_blob as *const HksBlob;

        // init gen_param_set
        let g_gen_params004: [HksParam; 5] = [
            HksParam {
                tag: HKS_TAG_ALGORITHM,
                union_1: HksParam_union_1 { uint32_param: HKS_ALG_AES },
            },
            HksParam {
                tag: HKS_TAG_PURPOSE,
                union_1: HksParam_union_1 {
                    uint32_param: HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT,
                },
            },
            HksParam {
                tag: HKS_TAG_KEY_SIZE,
                union_1: HksParam_union_1 { uint32_param: HKS_AES_KEY_SIZE_256 },
            },
            HksParam {
                tag: HKS_TAG_PADDING,
                union_1: HksParam_union_1 { uint32_param: HKS_PADDING_NONE },
            },
            HksParam {
                tag: HKS_TAG_BLOCK_MODE,
                union_1: HksParam_union_1 { uint32_param: HKS_MODE_GCM },
            },
        ];
        let mut buffer = Box::new([0u8; 8 + 5 * size_of::<HksParam>()]);
        unsafe {
            let gen_param_set = buffer.as_mut_ptr() as *mut HksParamSet;
            (*gen_param_set).param_set_size = 8 + 5 * size_of::<HksParam>() as u32;
            (*gen_param_set).params_cnt = 5;
            copy_nonoverlapping(
                g_gen_params004.as_ptr() as *const u8,
                buffer.as_mut_ptr().add(8),
                5 * size_of::<HksParam>(),
            );
        }

        unsafe { HksGenerateKey(key_alias, buffer.as_ptr() as *const HksParamSet, null_mut()) }
    }

    /// Delete the hukkey
    pub fn delete(&self) -> HuksErrcode {
        let hks_blob = HksBlob {
            size: self.alias.len() as u32,
            data: self.alias.as_str() as *const _ as *const u8,
        };
        let key_alias = &hks_blob as *const HksBlob;
        unsafe { HksDeleteKey(key_alias, null_mut()) }
    }

    /// Determine whether user auth is required.
    pub fn need_user_auth(&self) -> bool {
        for (i, item) in self.alias.split('_').enumerate() {
            if i == 2 {
                return item == 1.to_string();
            }
        }
        false
    }

    /// Determine whether device unlock is required.
    pub fn need_device_unlock(&self) -> bool {
        for (i, item) in self.alias.split('_').enumerate() {
            if i == 3 {
                return item == 3.to_string();
            }
        }
        false
    }
}

/// update and finish 目前相当于只有finish
pub fn update_and_finish(
    handle: &HksBlob,
    param_set: &HksParamSet,
    indata: &mut HksBlob,
    outdata: &mut HksBlob,
) -> HuksErrcode {
    let param_set_ptr = param_set as *const HksParamSet;
    let cur = outdata.data;
    outdata.size = 0;
    indata.size = MAX_UPDATE_SIZE;

    let mut out_data_finish = HksBlob { size: indata.size * TIMES, data: null_mut() };
    if malloc_and_check_blob_data(&mut out_data_finish) != HKS_SUCCESS {
        return HKS_FAILURE;
    }

    unsafe {
        if HksFinish(
            handle as *const HksBlob,
            param_set_ptr,
            indata as *mut HksBlob as *const HksBlob,
            &mut out_data_finish as *mut HksBlob,
        ) != HKS_SUCCESS
        {
            let layout =
                Layout::from_size_align(out_data_finish.size as usize, align_of::<u32>()).unwrap();
            dealloc(out_data_finish.data as *mut u8, layout);
            return HKS_FAILURE;
        }
    }

    unsafe {
        copy_nonoverlapping(out_data_finish.data, cur as *mut u8, out_data_finish.size as usize);
    }
    outdata.size += out_data_finish.size;
    let layout = Layout::from_size_align(out_data_finish.size as usize, align_of::<u32>()).unwrap();
    unsafe {
        dealloc(out_data_finish.data as *mut u8, layout);
    }

    HKS_SUCCESS
}

fn malloc_and_check_blob_data(blob: &mut HksBlob) -> HuksErrcode {
    unsafe {
        let layout = Layout::from_size_align(blob.size as usize, align_of::<u32>()).unwrap();
        blob.data = alloc(layout);
        if blob.data.is_null() {
            loge!("could not alloc memory");
            return HKS_FAILURE;
        }
    }
    HKS_SUCCESS
}

/// Crypto struct
pub struct Crypto {
    /// Crypto secretkey
    pub key: SecretKey,
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
    pub fn encrypt(&mut self, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        let hks_blob = HksBlob {
            size: self.key.alias.len() as u32,
            data: self.key.alias.as_str() as *const _ as *const u8,
        };
        let key_alias = &hks_blob as *const HksBlob;

        // init handle_encrypt
        let handle_e: Vec<u8> = vec![0, 0, 0, 0, 0, 0, 0, 0];
        let mut handle_encrypt = HksBlob { size: 8, data: handle_e.as_ptr() };

        // 此处对NONCE是硬编码
        let NONCE_VEC: Vec<u8> = vec![0; NONCE_SIZE as usize];
        let AAD = aad.as_ptr();
        let NONCE = NONCE_VEC.as_ptr();

        // Init encrypt_param_set
        let g_encrypt_params004: [HksParam; 7] = [
            HksParam {
                tag: HKS_TAG_ALGORITHM,
                union_1: HksParam_union_1 { uint32_param: HKS_ALG_AES },
            },
            HksParam {
                tag: HKS_TAG_PURPOSE,
                union_1: HksParam_union_1 { uint32_param: HKS_KEY_PURPOSE_ENCRYPT },
            },
            HksParam {
                tag: HKS_TAG_KEY_SIZE,
                union_1: HksParam_union_1 { uint32_param: HKS_AES_KEY_SIZE_256 },
            },
            HksParam {
                tag: HKS_TAG_PADDING,
                union_1: HksParam_union_1 { uint32_param: HKS_PADDING_NONE },
            },
            HksParam {
                tag: HKS_TAG_BLOCK_MODE,
                union_1: HksParam_union_1 { uint32_param: HKS_MODE_GCM },
            },
            HksParam {
                tag: HKS_TAG_ASSOCIATED_DATA,
                union_1: HksParam_union_1 { blob: HksBlob { size: AAD_SIZE, data: AAD } },
            },
            HksParam {
                tag: HKS_TAG_NONCE,
                union_1: HksParam_union_1 { blob: HksBlob { size: NONCE_SIZE, data: NONCE } },
            },
        ];
        let mut buffer = Box::new([0u8; 8 + 7 * size_of::<HksParam>()]);
        unsafe {
            let encrypt_param_set = buffer.as_mut_ptr() as *mut HksParamSet;
            (*encrypt_param_set).param_set_size =
                8 + 7 * size_of::<HksParam>() as u32 + AAD_SIZE + NONCE_SIZE;
            (*encrypt_param_set).params_cnt = 7;
            copy_nonoverlapping(
                g_encrypt_params004.as_ptr() as *const u8,
                buffer.as_mut_ptr().add(8),
                7 * size_of::<HksParam>(),
            );
        }

        let mut ret = unsafe {
            HksInit(
                key_alias,
                buffer.as_ptr() as *const HksParamSet,
                &mut handle_encrypt as *mut HksBlob,
                null_mut(),
            )
        };
        if ret != HKS_SUCCESS {
            loge!("Encrypt HksInit Failed.");
            return Err(ErrCode::Failed); //CRYPTO_FAIL
        }
        let mut indata = HksBlob { size: msg.len() as u32, data: msg.as_ptr() };
        let cipher: Vec<u8> = vec![0; AES_COMMON_SIZE as usize];
        let mut cipher_text = HksBlob { size: AES_COMMON_SIZE, data: cipher.as_ptr() };
        ret = unsafe {
            update_and_finish(
                &handle_encrypt,
                &*(buffer.as_ptr() as *const HksParamSet),
                &mut indata,
                &mut cipher_text,
            )
        };
        if ret != HKS_SUCCESS {
            loge!("Encrypt update_and_finish Failed.");
            return Err(ErrCode::Failed); //CRYPTO_FAIL
        }

        let mut cipher_final: Vec<u8> = vec![0; cipher_text.size as usize];
        cipher_final[0..cipher_text.size as usize]
            .copy_from_slice(&cipher[0..cipher_text.size as usize]);
        Ok(cipher_final)
    }

    /// Decrypt
    pub fn decrypt(&mut self, cipher: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        let hks_blob = HksBlob {
            size: self.key.alias.len() as u32,
            data: self.key.alias.as_str() as *const _ as *const u8,
        };
        let key_alias = &hks_blob as *const HksBlob;

        // init handle_decrypt
        let handle_d: Vec<u8> = vec![0, 0, 0, 0, 0, 0, 0, 0];
        let mut handle_decrypt = HksBlob { size: 8, data: handle_d.as_ptr() };

        // take the AEAD from cipher.
        if cipher.len() < AEAD_SIZE as usize {
            return Err(ErrCode::InvalidArgument);
        }
        let cipher_without_aead_size = cipher.len() - AEAD_SIZE as usize;
        let mut AEAD_VEC: Vec<u8> = vec![0; AEAD_SIZE as usize];
        AEAD_VEC[0..AEAD_SIZE as usize].copy_from_slice(
            &cipher[cipher_without_aead_size..(cipher_without_aead_size + AEAD_SIZE as usize)],
        );
        let mut cipher_text =
            HksBlob { size: cipher_without_aead_size as u32, data: cipher.as_ptr() };

        // 此处对NONCE是硬编码
        let NONCE_VEC: Vec<u8> = vec![0; NONCE_SIZE as usize];
        let AAD = aad.as_ptr();
        let NONCE = NONCE_VEC.as_ptr();
        let AEAD = AEAD_VEC.as_ptr();
        let g_decrypt_params004: [HksParam; 8] = [
            HksParam {
                tag: HKS_TAG_ALGORITHM,
                union_1: HksParam_union_1 { uint32_param: HKS_ALG_AES },
            },
            HksParam {
                tag: HKS_TAG_PURPOSE,
                union_1: HksParam_union_1 { uint32_param: HKS_KEY_PURPOSE_DECRYPT },
            },
            HksParam {
                tag: HKS_TAG_KEY_SIZE,
                union_1: HksParam_union_1 { uint32_param: HKS_AES_KEY_SIZE_256 },
            },
            HksParam {
                tag: HKS_TAG_PADDING,
                union_1: HksParam_union_1 { uint32_param: HKS_PADDING_NONE },
            },
            HksParam {
                tag: HKS_TAG_BLOCK_MODE,
                union_1: HksParam_union_1 { uint32_param: HKS_MODE_GCM },
            },
            HksParam {
                tag: HKS_TAG_ASSOCIATED_DATA,
                union_1: HksParam_union_1 { blob: HksBlob { size: AAD_SIZE, data: AAD } },
            },
            HksParam {
                tag: HKS_TAG_NONCE,
                union_1: HksParam_union_1 { blob: HksBlob { size: NONCE_SIZE, data: NONCE } },
            },
            HksParam {
                tag: HKS_TAG_AE_TAG,
                union_1: HksParam_union_1 { blob: HksBlob { size: AEAD_SIZE, data: AEAD } },
            },
        ];
        let mut buffer = Box::new([0u8; 8 + 8 * size_of::<HksParam>()]);
        unsafe {
            let decrypt_param_set = buffer.as_mut_ptr() as *mut HksParamSet;
            (*decrypt_param_set).param_set_size =
                8 + 8 * size_of::<HksParam>() as u32 + AAD_SIZE + NONCE_SIZE + AEAD_SIZE;
            (*decrypt_param_set).params_cnt = 8;
            copy_nonoverlapping(
                g_decrypt_params004.as_ptr() as *const u8,
                buffer.as_mut_ptr().add(8),
                8 * size_of::<HksParam>(),
            );
        }

        let mut ret = unsafe {
            HksInit(
                key_alias,
                buffer.as_ptr() as *const HksParamSet,
                &mut handle_decrypt as *mut HksBlob,
                null_mut(),
            )
        };
        if ret != HKS_SUCCESS {
            loge!("Decrypt Init Failed.");
            return Err(ErrCode::Failed); //CRYPTO_FAIL
        }

        let plain: Vec<u8> = vec![0; cipher.len()];
        let mut plain_text = HksBlob { size: cipher.len() as u32, data: plain.as_ptr() };
        ret = unsafe {
            update_and_finish(
                &handle_decrypt,
                &*(buffer.as_ptr() as *const HksParamSet),
                &mut cipher_text,
                &mut plain_text,
            )
        };
        if ret != HKS_SUCCESS {
            loge!("Decrypt update_and_finish Failed.");
            return Err(ErrCode::Failed); //CRYPTO_FAIL
        }
        Ok(plain)
    }
}
