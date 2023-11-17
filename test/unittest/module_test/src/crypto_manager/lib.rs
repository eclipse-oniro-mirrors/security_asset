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

use asset_crypto_manager::{crypto::*, crypto_manager::*, secret_key::*};
use asset_definition::{Accessibility, AuthType};

pub const AAD_SIZE: u32 = 8;

#[test]
fn generate_and_delete() {
    let secret_key = SecretKey::new(1, &vec![b'2'], AuthType::None, Accessibility::DeviceUnlocked, false);
    secret_key.generate().unwrap();
    assert!(secret_key.delete().is_ok())
}

#[test]
fn encrypt_and_decrypt() {
    // generate key
    let secret_key = SecretKey::new(4, &vec![b'2'], AuthType::None, Accessibility::DeviceFirstUnlocked, false);
    secret_key.generate().unwrap();

    // encrypt data
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let cipher = Crypto::encrypt(&secret_key, &msg, &aad).unwrap();
    assert!(!cipher.eq(&msg));

    // decrypt data
    let plaintext = Crypto::decrypt(&secret_key, &cipher, &aad).unwrap();
    assert!(plaintext.eq(&msg));

    // delete key
    let _ = secret_key.delete();
}

#[test]
fn crypto_init() {
    let secret_key = SecretKey::new(6, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlocked, false);
    secret_key.generate().unwrap();

    let mut crypto = Crypto::build(secret_key.clone(), 600).unwrap();
    crypto.init_key().unwrap();
    let _ = secret_key.delete();
}

#[test]
fn crypto_exec() {
    let secret_key = SecretKey::new(7, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlocked, false);
    secret_key.generate().unwrap();

    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let cipher = Crypto::encrypt(&secret_key, &msg, &aad).unwrap();
    let mut crypto = Crypto::build(secret_key.clone(), 600).unwrap();
    crypto.init_key().unwrap();

    let authtoken = vec![0; 148];
    assert!(crypto.exec_crypt(&cipher, &aad, &authtoken).is_err());
    let _ = secret_key.delete();
}

#[test]
fn crypto_manager() {
    let secret_key1 = SecretKey::new(8, &vec![b'2'], AuthType::Any, Accessibility::DeviceFirstUnlocked, false);
    secret_key1.generate().unwrap();
    let mut crypto1 = Crypto::build(secret_key1.clone(), 600).unwrap();
    let challenge1 = crypto1.init_key().unwrap().clone();

    let secret_key2 = SecretKey::new(8, &vec![b'2'], AuthType::Any, Accessibility::DeviceUnlocked, false);
    secret_key2.generate().unwrap();
    let mut crypto2 = Crypto::build(secret_key2.clone(), 600).unwrap();
    let challenge2 = crypto2.init_key().unwrap().clone();

    let arc_crypto_manager = CryptoManager::get_instance();
    let mut crypto_manager = arc_crypto_manager.lock().unwrap();
    crypto_manager.add(crypto1).unwrap();
    crypto_manager.add(crypto2).unwrap();

    crypto_manager.find(&challenge1).unwrap();
    crypto_manager.find(&challenge2).unwrap();

    crypto_manager.remove(&challenge1);
    crypto_manager.remove(&challenge2);

    let _ = secret_key1.delete();
    let _ = secret_key2.delete();
}
