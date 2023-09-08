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
use crypto_manager::hukkey::*;
use crypto_manager::hukkey_ffi::*;

#[test]
fn test_hukkey_new(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let secret_key = SecretKey::new(info);
    assert_eq!(secret_key.alias,"1_2_3_4".to_string());
}

#[test]
fn test_hukkey_generate(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let mut secret_key = SecretKey::new(info);
    let mut gen_param_set = HksParamSet::new();
    let g_gen_params004:[HksParam;5] = [
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

    let ret = InitParamSet(&mut &mut gen_param_set, &g_gen_params004[0], 5);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(gen) failed.");
    }
    match secret_key.generate(gen_param_set){
        Ok((_a,_b,_c)) =>{
            println!("test_hukkey_generate pass");
        }
        Err(error) =>{
            panic!("test_hukkey_generate fail error = {}", error);
        }
    }
}

#[test]
fn test_hukkey_delete(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let mut secret_key = SecretKey::new(info);
    let mut gen_param_set = HksParamSet::new();
    let g_gen_params004:[HksParam;5] = [
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

    let ret = InitParamSet(&mut &mut gen_param_set, &g_gen_params004[0], 5);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(gen) failed.");
    }
    match secret_key.generate(gen_param_set){
        Ok((a,b,_c)) =>{
            assert_eq!(secret_key.delete(a,b),0);
        }
        Err(error) =>{
            panic!("test_hukkey_generate fail error = {}", error);
        }
    }
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_encrypt(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let mut secret_key = SecretKey::new(info);
    // Init gen_param_set
    let mut gen_param_set = HksParamSet::new();
    let g_gen_params004:[HksParam;5] = [
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
    let mut ret = InitParamSet(&mut &mut gen_param_set, &g_gen_params004[0], 5);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(gen) failed.");
    }
    // Init encrypt_param_set
    let mut encrypt_param_set = HksParamSet::new();
    let mut AAD_VEC: Vec<u8> = vec![0;AAD_SIZE as usize];
    let mut NONCE_VEC: Vec<u8> = vec![0;NONCE_SIZE as usize];
    let mut AEAD_VEC: Vec<u8> = vec![0;AEAD_SIZE as usize];
    let AAD = &mut AAD_VEC[0] as *mut _ as *mut u8;
    let NONCE = &mut NONCE_VEC[0] as *mut _ as *mut u8;
    let AEAD = &mut AEAD_VEC[0] as *mut _ as *mut u8;
    let g_encrypt_params004:[HksParam;9] = [
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
        },
        HksParam{
            tag: HKS_TAG_DIGEST,
            union_1: HksParam_union_1{
                uint32_param: HKS_DIGEST_NONE
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
    ret = InitParamSet(&mut &mut encrypt_param_set, &g_encrypt_params004[0], 9);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(encrypt) failed.");
    }

    match secret_key.generate(gen_param_set){
        Ok((a,b,_c)) =>{
            let mut msg = vec![1,2,3,4,5,6];
            match Crypto::encrypt(a.as_ref(), b.as_ref(), encrypt_param_set, &mut msg){
                Ok(cipher) =>{
                    // Check whether the values of msg and cipher are the same.
                    let mut flag = true;
                    for i in 0..=msg.len(){
                        if msg[i] == cipher[i]{
                            continue;
                        }else{
                            flag = false;
                            break;
                        }
                    }
                    if flag{
                        panic!("cipher_text equals indata.");
                    }
                    println!("test_hukkey_encrypt pass");
                }
                Err(error) =>{
                    panic!("test_hukkey_encrypt fail error = {}", error);
                }
            }
        }
        Err(error) =>{
            panic!("test_hukkey_generate fail error = {}", error);
        }
    }
    
    
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_decrypt(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let mut secret_key = SecretKey::new(info);
    // Init gen_param_set
    let mut gen_param_set = HksParamSet::new();
    let g_gen_params004:[HksParam;5] = [
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
    let mut ret = InitParamSet(&mut &mut gen_param_set, &g_gen_params004[0], 5);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(gen) failed.");
    }
    // Init encrypt_param_set
    let mut encrypt_param_set = HksParamSet::new();
    let mut AAD_VEC: Vec<u8> = vec![0;AAD_SIZE as usize];
    let mut NONCE_VEC: Vec<u8> = vec![0;NONCE_SIZE as usize];
    let mut AEAD_VEC: Vec<u8> = vec![0;AEAD_SIZE as usize];
    let AAD = &mut AAD_VEC[0] as *mut _ as *mut u8;
    let NONCE = &mut NONCE_VEC[0] as *mut _ as *mut u8;
    let AEAD = &mut AEAD_VEC[0] as *mut _ as *mut u8;
    let g_encrypt_params004:[HksParam;9] = [
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
        },
        HksParam{
            tag: HKS_TAG_DIGEST,
            union_1: HksParam_union_1{
                uint32_param: HKS_DIGEST_NONE
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
    ret = InitParamSet(&mut &mut encrypt_param_set, &g_encrypt_params004[0], 9);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(encrypt) failed.");
    }
    // Init decrypt_param_set
    let mut decrypt_param_set = HksParamSet::new();
    let g_decrypt_params004:[HksParam;9] = [
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
        },
        HksParam{
            tag: HKS_TAG_DIGEST,
            union_1: HksParam_union_1{
                uint32_param: HKS_DIGEST_NONE
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
    ret = InitParamSet(&mut &mut decrypt_param_set, &g_decrypt_params004[0], 9);
    if ret != HKS_SUCCESS{
        panic!("InitParamSet(decrypt) failed.");
    }

    match secret_key.generate(gen_param_set){
        Ok((a,b,_c)) =>{
            let mut msg = vec![1,2,3,4,5,6];
            match Crypto::encrypt(a.as_ref(), b.as_ref(), encrypt_param_set, &mut msg){
                Ok(mut cipher) =>{
                    // Check whether the values of msg and cipher are the same.
                    let mut flag = true;
                    for i in 0..=msg.len(){
                        if msg[i] == cipher[i]{
                            continue;
                        }else{
                            flag = false;
                            break;
                        }
                    }
                    if flag{
                        panic!("cipher_text equals indata.");
                    }
                    match Crypto::decrypt(a.as_ref(), b.as_ref(), decrypt_param_set, cipher.as_mut()){
                        Ok(plain) =>{
                            let mut flag = true;
                            for i in 0..=msg.len(){
                                if msg[i] != plain[i]{
                                    flag = false;
                                    break;
                                }else{
                                    continue;
                                }
                            }
                            if !flag{
                                panic!("plain_text not equals inData");
                            }
                            println!("test_hukkey_decrypt pass");
                        }
                        Err(error) =>{
                            panic!("test_hukkey_decrypt fail error = {}", error);
                        }
                    }
                }
                Err(error) =>{
                    panic!("test_hukkey_encrypt fail error = {}", error);
                }
            }
        }
        Err(error) =>{
            panic!("test_hukkey_generate fail error = {}", error);
        }
    }
    
}

