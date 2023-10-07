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
use crypto_manager::crypto::*;
use crypto_manager::huks_ffi::*;

pub const AAD_SIZE: u32 = 8;

#[test]
fn test_hukkey_new() {
    let info = KeyInfo { user_id: 1, owner: "2".to_owned(), auth_type: 3, access_type: 4 };
    let secret_key = SecretKey::new(info);
    assert_eq!(secret_key.alias, "1_2_3_4".to_string());
}

#[test]
fn test_hukkey_generate_and_delete() {
    let info = KeyInfo { user_id: 1, owner: "2".to_owned(), auth_type: 3, access_type: 4 };
    let secret_key = SecretKey::new(info);
    let generate_res = secret_key.generate();
    if generate_res != HKS_SUCCESS {
        panic!("test_hukkey_delete fail because generate error = {}", generate_res);
    }

    println!("test_hukkey_generate: generate success");
    let delete_res = secret_key.delete();
    if delete_res != HKS_SUCCESS {
        panic!("test_hukkey_delete fail error = {}", delete_res);
    }

    println!("test_hukkey_delete pass");
}

#[test]
fn test_hukkey_need_user_auth() {
    let info_need = KeyInfo { user_id: 0, owner: "0".to_owned(), auth_type: 1, access_type: 0 };
    let secret_key_need = SecretKey::new(info_need);
    let info_dont_need = KeyInfo { user_id: 0, owner: "0".to_owned(), auth_type: 0, access_type: 0 };
    let secret_key_dont_need = SecretKey::new(info_dont_need);
    assert!(secret_key_need.need_user_auth());
    assert!(!secret_key_dont_need.need_user_auth());
}

#[test]
fn test_hukkey_need_device_unlock() {
    let info_need = KeyInfo { user_id: 0, owner: "0".to_owned(), auth_type: 0, access_type: 3 };
    let secret_key_need = SecretKey::new(info_need);
    let info_dont_need = KeyInfo { user_id: 0, owner: "0".to_owned(), auth_type: 0, access_type: 0 };
    let secret_key_dont_need = SecretKey::new(info_dont_need);
    assert!(secret_key_need.need_device_unlock());
    assert!(!secret_key_dont_need.need_device_unlock());
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_encrypt() {
    let info = KeyInfo { user_id: 1, owner: "0".to_owned(), auth_type: 0, access_type: 0 };
    let secret_key = SecretKey::new(info);
    let generate_res = secret_key.generate();
    let crypto = Crypto { key: secret_key };

    if generate_res != HKS_SUCCESS {
        panic!("test_hukkey_encrypt fail because generate error = {}", generate_res);
    }

    println!("test_hukkey_encrypt: generate success");
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let encrypt_res = crypto.encrypt(&msg, &aad);
    match encrypt_res {
        Ok(cipher) => {
            println!("encrypt success, cipher is {:?}, now check cipher", cipher);
            let mut flag = true;
            for i in 0..msg.len() {
                if msg[i] != cipher[i] {
                    flag = false;
                    break;
                }
            }
            if flag {
                crypto.key.delete();
                panic!("test_hukkey_encrypt fail because cipher_text equals indata.");
            }

            println!("test_hukkey_encrypt pass");
        },
        Err(e) => {
            crypto.key.delete();
            panic!("test_hukkey_encrypt fail because encrypt error = {}", e);
        },
    }
    crypto.key.delete();
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_decrypt() {
    let info = KeyInfo { user_id: 1, owner: "3".to_owned(), auth_type: 3, access_type: 4 };
    let secret_key = SecretKey::new(info);
    let generate_res = secret_key.generate();
    let crypto = Crypto { key: secret_key };
    if generate_res != HKS_SUCCESS {
        panic!("test_hukkey_decrypt fail because generate key error = {}", generate_res);
    }

    println!("test_hukkey_decrypt: generate key success");
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let encrypt_res = crypto.encrypt(&msg, &aad);
    match encrypt_res {
        Ok(cipher) => {
            println!("encrypt success, cipher is {:?}, now check cipher", cipher);
            let mut flag = true;
            for i in 0..msg.len() {
                if msg[i] != cipher[i] {
                    flag = false;
                    break;
                }
            }
            if flag {
                crypto.key.delete();
                panic!("test_hukkey_decrypt fail because cipher_text equals indata.");
            }

            println!("encrypt pass, now decrypt..., cipher is {:?}", cipher);
            let decrypt_res = crypto.decrypt(&cipher, &aad);
            match decrypt_res {
                Ok(plain) => {
                    println!("decrypt pass, plain is {:?}, now check decrypt", plain);
                    flag = true;
                    for i in 0..msg.len() {
                        if msg[i] != plain[i] {
                            flag = false;
                            break;
                        }
                    }
                    if !flag {
                        crypto.key.delete();
                        panic!("test_hukkey_decrypt fail because plain_text not equals inData");
                    }

                    println!("test_hukkey_decrypt pass");
                },
                Err(e) => {
                    crypto.key.delete();
                    panic!("test_hukkey_decrypt fail because decrypt error = {}", e);
                },
            }
        },
        Err(e) => {
            crypto.key.delete();
            panic!("test_hukkey_decrypt fail because encrypt error = {}", e);
        },
    }
    crypto.key.delete();
}
