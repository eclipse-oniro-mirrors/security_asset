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
use crate::huks_ffi::*;
use asset_common::{loge, definition::ErrCode};
use hilog_rust::hilog;
use std::ffi::{c_char, CString};
use std::ptr::{null_mut,copy_nonoverlapping};
use std::mem::{size_of,align_of};
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
        Self { alias: format!("{}_{}_{}_{}", info.user_id,info.uid,info.auth_type,info.access_type) }
    }

    /// Check whether the secret key exists
    pub fn exists(&self) -> HuksErrcode{
        let hks_blob = HksBlob{
            size: self.alias.len() as u32,
            data: &mut self.alias.as_str() as *mut _ as *mut u8,
        };
        let key_alias = &hks_blob as *const HksBlob;
        unsafe{HksKeyExist(key_alias,null_mut())}
    }

    /// Generate the hukkey
    pub fn generate(&self) -> HuksErrcode{
        let hks_blob = HksBlob{
            size: self.alias.len() as u32,
            data: &mut self.alias.as_str() as *mut _ as *mut u8,
        };
        let key_alias = &hks_blob as *const HksBlob;

        // init gen_param_set
        let mut g_gen_params004:[HksParam;5] = [
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
                    uint32_param: HKS_AES_KEY_SIZE_256
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
        let gen_param_set = HksParamSet { 
            param_set_size: 8 + 5 * size_of::<HksParam>() as u32, 
            params_cnt: 5, 
            params: &mut g_gen_params004[0] as *mut _ as *mut HksParam,
        };
        unsafe{HksGenerateKey(key_alias, &gen_param_set as *const HksParamSet, null_mut())}
    }

    /// Delete the hukkey
    pub fn delete(&self) -> HuksErrcode{
        let hks_blob = HksBlob{
            size: self.alias.len() as u32,
            data: &mut self.alias.as_str() as *mut _ as *mut u8,
        };
        let key_alias = &hks_blob as *const HksBlob;
        unsafe{HksDeleteKey(key_alias,null_mut())}
    }

    /// Determine whether user auth is required.
    pub fn need_user_auth(&self) -> bool{
        for (i,item) in self.alias.split('_').enumerate(){
            if i == 2{
                return item == 1.to_string();
            }
        }
        false
    }

    /// Determine whether device unlock is required.
    pub fn need_device_unlock(&self) -> bool{
        for (i,item) in self.alias.split('_').enumerate(){
            if i == 3{
                return item == 3.to_string();
            }
        }
        false
    }
}

/// update and finish 目前相当于只有finish
pub fn update_and_finish(handle:&HksBlob, param_set:&HksParamSet, indata:&mut HksBlob, outdata:&mut HksBlob) -> HuksErrcode{
    let param_set_ptr = param_set as *const HksParamSet;
    let cur = outdata.data;
    outdata.size = 0;
    indata.size = MAX_UPDATE_SIZE;

    let mut out_data_finish = HksBlob{
        size: indata.size * TIMES,
        data: null_mut()
    };
    if malloc_and_check_blob_data(&mut out_data_finish) != HKS_SUCCESS{
        return HKS_FAILURE;
    }

    unsafe{
        if HksFinish(handle as *const HksBlob, param_set_ptr, 
            indata as *mut HksBlob as *const HksBlob, &mut out_data_finish as *mut HksBlob) != HKS_SUCCESS{
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

fn malloc_and_check_blob_data(blob: &mut HksBlob) -> HuksErrcode{
    unsafe{
        let layout = Layout::from_size_align(blob.size as usize,align_of::<u32>()).unwrap();
        blob.data = alloc(layout);
        if blob.data.is_null(){
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
    pub fn encrypt(&mut self, msg: &mut Vec<u8>,aad:&mut Vec<u8>) -> Result<Vec<u8>,ErrCode>{
        let hks_blob = HksBlob{
            size: self.key.alias.len() as u32,
            data: &mut self.key.alias.as_str() as *mut _ as *mut u8,
        };
        let key_alias = &hks_blob as *const HksBlob;

        // init handle_encrypt
        let mut handle_e: Vec<u8> = vec![0,0,0,0,0,0,0,0];
        let mut handle_encrypt = HksBlob{
            size: 8,
            data: &mut handle_e[0] as *mut _ as *mut u8,
        };

        // 此处对NONCE是硬编码
        let mut NONCE_VEC: Vec<u8> = vec![0;NONCE_SIZE as usize];
        let AAD = &mut (*aad)[0] as *mut _ as *mut u8;
        let NONCE = &mut NONCE_VEC[0] as *mut _ as *mut u8;

        // Init encrypt_param_set
        let mut g_encrypt_params004:[HksParam;7] = [
            HksParam{
                tag: HKS_TAG_ALGORITHM,
                union_1: HksParam_union_1{
                    uint32_param: HKS_ALG_AES
                }
            },
            HksParam{
                tag: HKS_TAG_PURPOSE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_KEY_PURPOSE_ENCRYPT
                }
            },
            HksParam{
                tag: HKS_TAG_KEY_SIZE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_AES_KEY_SIZE_256
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
            },
            HksParam{
                tag: HKS_TAG_ASSOCIATED_DATA,
                union_1: HksParam_union_1{
                    blob: HksBlob{
                        size: AAD_SIZE,
                        data: AAD,
                    }
                }
            },
            HksParam{
                tag: HKS_TAG_NONCE,
                union_1: HksParam_union_1{
                    blob: HksBlob{
                        size: NONCE_SIZE,
                        data: NONCE,
                    }
                }
            },
        ];
        let encrypt_param_set = HksParamSet { 
            param_set_size: 8 + 7 * size_of::<HksParam>() as u32, 
            params_cnt: 7, 
            params: &mut g_encrypt_params004[0] as *mut _ as *mut HksParam,
        };


        let mut ret = unsafe{
            HksInit(key_alias, &encrypt_param_set as *const HksParamSet, 
                &mut handle_encrypt as *mut HksBlob, null_mut())
        };
        if ret != HKS_SUCCESS{
            loge!("Encrypt HksInit Failed.");
            return Err(ErrCode::Failed);
        }
        let mut indata = HksBlob{
            size: msg.len() as u32,
            data: &mut (*msg)[0] as *mut _ as *mut u8
        };
        let mut cipher: Vec<u8> = vec![0;AES_COMMON_SIZE as usize];
        let mut cipher_text =HksBlob{
            size: AES_COMMON_SIZE,
            data: &mut cipher[0] as *mut _ as *mut u8,
        };
        ret = update_and_finish(&handle_encrypt, &encrypt_param_set, &mut indata, &mut cipher_text);
        if ret != HKS_SUCCESS{
            loge!("Encrypt update_and_finish Failed.");
            return Err(ErrCode::Failed);
        }
        Ok(cipher)
    }

    /// Decrypt
    pub fn decrypt(&mut self, cipher: &mut Vec<u8>, aad:&mut Vec<u8>) -> Result<Vec<u8>,ErrCode>{
        let hks_blob = HksBlob{
            size: self.key.alias.len() as u32,
            data: &mut self.key.alias.as_str() as *mut _ as *mut u8,
        };
        let key_alias = &hks_blob as *const HksBlob;
        
        // init handle_decrypt
        let mut handle_d: Vec<u8> = vec![0,0,0,0,0,0,0,0];
        let mut handle_decrypt = HksBlob{
            size: 8,
            data: &mut handle_d[0] as *mut _ as *mut u8,
        };

        // take the AEAD from cipher.
        let cipher_without_aead_size = cipher.len() - AEAD_SIZE as usize;
        let mut AEAD_VEC: Vec<u8> = vec![0;AEAD_SIZE as usize];
        // for i in cipher_without_aead_size..cipher_without_aead_size + AEAD_SIZE as usize{
        //     AEAD_VEC[i] = cipher[i];
        // }
        AEAD_VEC[cipher_without_aead_size..(cipher_without_aead_size + AEAD_SIZE as usize)].copy_from_slice(
            &cipher[cipher_without_aead_size..(cipher_without_aead_size + AEAD_SIZE as usize)]);
        let mut cipher_text = HksBlob{
            size: cipher_without_aead_size as u32,
            data: &mut (*cipher)[0] as *mut _ as *mut u8
        };

        // 此处对NONCE是硬编码
        let mut NONCE_VEC: Vec<u8> = vec![0;NONCE_SIZE as usize];
        let AAD = &mut (*aad)[0] as *mut _ as *mut u8;
        let NONCE = &mut NONCE_VEC[0] as *mut _ as *mut u8;
        let AEAD = &mut AEAD_VEC[0] as *mut _ as *mut u8;
        let mut g_decrypt_params004:[HksParam;8] = [
            HksParam{
                tag: HKS_TAG_ALGORITHM,
                union_1: HksParam_union_1{
                    uint32_param: HKS_ALG_AES
                }
            },
            HksParam{
                tag: HKS_TAG_PURPOSE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_KEY_PURPOSE_DECRYPT
                }
            },
            HksParam{
                tag: HKS_TAG_KEY_SIZE,
                union_1: HksParam_union_1{
                    uint32_param: HKS_AES_KEY_SIZE_256
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
            },
            HksParam{
                tag: HKS_TAG_ASSOCIATED_DATA,
                union_1: HksParam_union_1{
                    blob: HksBlob{
                        size: AAD_SIZE,
                        data: AAD,
                    }
                }
            },
            HksParam{
                tag: HKS_TAG_NONCE,
                union_1: HksParam_union_1{
                    blob: HksBlob{
                        size: NONCE_SIZE,
                        data: NONCE,
                    }
                }
            },
            HksParam{
                tag: HKS_TAG_AE_TAG,
                union_1: HksParam_union_1{
                    blob: HksBlob{
                        size: AEAD_SIZE,
                        data: AEAD,
                    }
                }
            },
        ];
        let decrypt_param_set = HksParamSet { 
            param_set_size: 8 + 8 * size_of::<HksParam>() as u32, 
            params_cnt: 8, 
            params: &mut g_decrypt_params004[0] as *mut _ as *mut HksParam,
        };

        let mut ret = unsafe{
            HksInit(key_alias, &decrypt_param_set as *const HksParamSet, 
                &mut handle_decrypt as *mut HksBlob, null_mut())
        };
        if ret != HKS_SUCCESS{
            loge!("Decrypt Init Failed.");
            return Err(ErrCode::Failed);
        }

        
        let mut plain: Vec<u8> = vec![0;AES_COMMON_SIZE as usize];
        let mut plain_text = HksBlob{
            size: AES_COMMON_SIZE,
            data: &mut plain[0] as *mut _ as *mut u8,
        };
        ret = update_and_finish(&handle_decrypt, &decrypt_param_set, &mut cipher_text, &mut plain_text);
        if ret != HKS_SUCCESS{
            loge!("Decrypt update_and_finish Failed.");
            return Err(ErrCode::Failed);
        }
        Ok(plain)
    }
}
