/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use asset_crypto_manager::crypto::*;
use asset_crypto_manager::huks_ffi::*;
use asset_definition::{Accessibility, AuthType};

pub const AAD_SIZE: u32 = 8;

#[test]
fn test_hukkey_key_new() {
    let _secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
}

#[test]
fn test_hukkey_key_generate_and_delete() {
    let secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    match secret_key.generate() {
        Ok(()) => println!("test_hukkey_generate: generate success"),
        Err(res) => panic!("test_hukkey_delete fail because generate error = {}", res),
    };

    match secret_key.delete() {
        Ok(()) => println!("test_hukkey_delete pass"),
        Err(res) => panic!("test_hukkey_delete fail error = {}", res),
    }
}

#[test]
fn test_hukkey_need_user_auth() {
    let secret_key_need = SecretKey::new(2, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlock);
    let secret_key_dont_need = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    assert!(secret_key_need.need_user_auth());
    assert!(!secret_key_dont_need.need_user_auth());
}

#[test]
fn test_hukkey_need_device_unlock() {
    let secret_key_need = SecretKey::new(3, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlock);
    let secret_key_dont_need = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlock);
    assert!(secret_key_need.need_device_unlock());
    assert!(!secret_key_dont_need.need_device_unlock());
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_encrypt() {
    let secret_key = SecretKey::new(4, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlock);
    match secret_key.generate() {
        Ok(()) => println!("test_hukkey_generate: generate success"),
        Err(res) => panic!("test_hukkey_encrypt fail because generate error = {}", res),
    };

    println!("test_hukkey_encrypt: generate success");
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let encrypt_res = Crypto::encrypt(&secret_key, &msg, &aad);
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
                let _ = secret_key.delete();
                panic!("test_hukkey_encrypt fail because cipher_text equals indata.");
            }

            println!("test_hukkey_encrypt pass");
        },
        Err(e) => {
            let _ = secret_key.delete();
            panic!("test_hukkey_encrypt fail because encrypt error = {}", e);
        },
    }
    let _ = secret_key.delete();
}

#[test]
#[allow(non_snake_case)]
fn test_hukkey_decrypt() {
    let secret_key = SecretKey::new(5, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlock);
    match secret_key.generate() {
        Ok(()) => println!("test_hukkey_generate: generate success"),
        Err(res) => panic!("test_hukkey_encrypt fail because generate error = {}", res),
    };

    println!("test_hukkey_decrypt: generate key success");
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let encrypt_res = Crypto::encrypt(&secret_key, &msg, &aad);
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
                let _ = secret_key.delete();
                panic!("test_hukkey_decrypt fail because cipher_text equals indata.");
            }

            println!("encrypt pass, now decrypt..., cipher is {:?}", cipher);
            let decrypt_res = Crypto::decrypt(&secret_key, &cipher, &aad);
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
                        let _ = secret_key.delete();
                        panic!("test_hukkey_decrypt fail because plain_text not equals inData");
                    }

                    println!("test_hukkey_decrypt pass");
                },
                Err(e) => {
                    let _ = secret_key.delete();
                    panic!("test_hukkey_decrypt fail because decrypt error = {}", e);
                },
            }
        },
        Err(e) => {
            let _ = secret_key.delete();
            panic!("test_hukkey_decrypt fail because encrypt error = {}", e);
        },
    }
    let _ = secret_key.delete();
}

#[test]
fn test_crypto_init() {
    let secret_key = SecretKey::new(6, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlock);
    match secret_key.exists() {
        Ok(true) => (),
        Ok(false) => {
            print!("huks key start create");
            match secret_key.generate() {
                Ok(()) => println!("test_hukkey_generate: generate success"),
                Err(res) => panic!("test_hukkey_delete fail because generate error = {}", res),
            };
        },
        _ => panic!("hukskey exist failed"),
    };

    let secret_key2 = SecretKey::new(6, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlock);

    let mut crypto = Crypto::new(HKS_KEY_PURPOSE_DECRYPT, secret_key, 0, 600);
    let challenge = crypto.init_crypto();
    match challenge {
        Ok(_) => print!("crypto challenge init success"),
        Err(_) => {
            let _ = secret_key2.delete();
            panic!("crypto init fail")
        },
    };
    let _ = secret_key2.delete();
}

#[test]
fn test_crypto_exec_crypto() {
    let secret_key = SecretKey::new(7, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlock);
    match secret_key.exists() {
        Ok(true) => println!("test_hukkey_decrypt: key exist"),
        Ok(false) => {
            print!("test_crypto_exec_crypto: huks key start create");
            match secret_key.generate() {
                Ok(()) => println!("test_hukkey_generate: generate success"),
                Err(res) => panic!("test_hukkey_delete fail because generate error = {}", res),
            };
        },
        _ => panic!("hukskey exist failed"),
    };

    let secret_key2 = SecretKey::new(7, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlock);
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let encrypt_res = Crypto::encrypt(&secret_key, &msg, &aad);
    match encrypt_res {
        Ok(cipher) => {
            println!("encrypt pass, now decrypt..., cipher is {:?}", cipher);
            let mut crypto = Crypto::new(HKS_KEY_PURPOSE_DECRYPT, secret_key, 0, 600);
            let challenge = crypto.init_crypto();
            match challenge {
                Ok(_) => print!("test_crypto_exec_crypto: crypto challenge init success"),
                Err(_) => {
                    let _ = secret_key2.delete();
                    panic!("crypto init fail")
                },
            };

            let authtoken = vec![0; 148]; // todo, need authtoken
            let encrypt_res = crypto.exec_crypto(&cipher, &aad, &authtoken);
            match encrypt_res {
                Ok(plain_text) => {
                    println!("decrypt pass, plain is {:?}, now check decrypt", plain_text);
                    for i in 0..msg.len() {
                        if msg[i] != plain_text[i] {
                            let _ = secret_key2.delete();
                            panic!("test_hukkey_decrypt fail because plain_text not equals inData");
                        }
                    }
                },
                Err(e) => println!("test_crypto_exec_crypto fail because encrypt error = {}", e),
            }
        },
        Err(e) => {
            let _ = secret_key2.delete();
            panic!("test_crypto_exec_crypto fail because encrypt error = {}", e);
        },
    }
    let _ = secret_key2.delete();
}
