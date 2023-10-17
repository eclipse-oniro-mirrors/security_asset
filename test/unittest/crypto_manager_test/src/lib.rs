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
use asset_common::definition::{Accessibility, AuthType};
use crypto_manager::crypto::*;

pub const AAD_SIZE: u32 = 8;

#[test]
fn test_hukkey_key_new() { // todo: zdy 不要出现hukkey, 不存在huk, 可以使用secret key替换
    // let secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    // assert_eq!(secret_key.alias, vec![1, 0, 0, 0, 95, 50, 95, 3, 0, 0, 0, 95, 4, 0, 0, 0]);
    // todo zdy 不要为了测试用例暴露不该是public的字段
}

#[test]
fn test_hukkey_key_generate_and_delete() {
    let secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    match secret_key.generate() {
        Ok(()) => println!("test_hukkey_generate: generate success"),
        Err(res) => panic!("test_hukkey_delete fail because generate error = {}", res),
    };

    match secret_key.delete() {
        Ok(true) => println!("test_hukkey_delete pass"),
        Ok(false) => println!("never reached"),
        Err(res) => panic!("test_hukkey_delete fail error = {}", res),
    }
}

#[test]
fn test_hukkey_need_user_auth() {
    let secret_key_need = SecretKey::new(1, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlock);
    let secret_key_dont_need = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    assert!(secret_key_need.need_user_auth());
    assert!(!secret_key_dont_need.need_user_auth());
}

#[test]
fn test_hukkey_need_device_unlock() {
    let secret_key_need = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    let secret_key_dont_need = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlock);
    assert!(secret_key_need.need_device_unlock());
    assert!(!secret_key_dont_need.need_device_unlock());
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_encrypt() {
    let secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlock);
    match secret_key.generate() {
        Ok(()) => println!("test_hukkey_generate: generate success"),
        Err(res) => panic!("test_hukkey_encrypt fail because generate error = {}", res),
    };

    let crypto = Crypto { key: secret_key };

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
                let _ = crypto.key.delete();
                panic!("test_hukkey_encrypt fail because cipher_text equals indata.");
            }

            println!("test_hukkey_encrypt pass");
        },
        Err(e) => {
            let _ = crypto.key.delete();
            panic!("test_hukkey_encrypt fail because encrypt error = {}", e);
        },
    }
    let _ = crypto.key.delete();
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_decrypt() {
    let secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlock);
    match secret_key.generate() {
        Ok(()) => println!("test_hukkey_generate: generate success"),
        Err(res) => panic!("test_hukkey_encrypt fail because generate error = {}", res),
    };

    let crypto = Crypto { key: secret_key };

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
                let _ = crypto.key.delete();
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
                        let _ = crypto.key.delete();
                        panic!("test_hukkey_decrypt fail because plain_text not equals inData");
                    }

                    println!("test_hukkey_decrypt pass");
                },
                Err(e) => {
                    let _ = crypto.key.delete();
                    panic!("test_hukkey_decrypt fail because decrypt error = {}", e);
                },
            }
        },
        Err(e) => {
            let _ = crypto.key.delete();
            panic!("test_hukkey_decrypt fail because encrypt error = {}", e);
        },
    }
    let _ = crypto.key.delete();
}
