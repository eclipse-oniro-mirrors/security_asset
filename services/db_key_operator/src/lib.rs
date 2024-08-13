/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

//! This module implements functions related to Asset database key.

use asset_common::CallingInfo;
use asset_crypto_manager::{crypto::Crypto, secret_key::SecretKey};
use asset_db_operator::database::Database;
use asset_definition::{Accessibility, AssetMap, AuthType, Result, Tag, Value};
use asset_file_operator::{is_db_key_cipher_file_exist, read_db_key_cipher, write_db_key_cipher};
use asset_log::logi;
use openssl::rand::rand_bytes;
use std::sync::Mutex;

fn build_db_key_secret_key(calling_info: &CallingInfo) -> Result<SecretKey> {
    let auth_type = AuthType::None;
    let access_type = Accessibility::DeviceFirstUnlocked;
    let require_password_set = false;
    let alias = "db_key_secret_key".as_bytes().to_vec();

    Ok(SecretKey::new(calling_info, auth_type, access_type, require_password_set, Some(alias)))
}

/// Decrypt db key cipher.
pub fn decrypt_db_key_cipher(calling_info: &CallingInfo, db_key_cipher: &Vec<u8>) -> Result<Vec<u8>> {
    let secret_key = build_db_key_secret_key(calling_info)?;
    let aad: Vec<u8> = "trivial_aad_for_db_key".as_bytes().to_vec();
    let db_key = Crypto::decrypt(&secret_key, db_key_cipher, &aad)?;

    Ok(db_key)
}

fn generate_db_key() -> Result<Vec<u8>> {
    const KEY_LEN_IN_BYTES: usize = 32; // aes-256-gcm requires key length 256 bits = 32 bytes.
    let mut db_key = [0; KEY_LEN_IN_BYTES];
    rand_bytes(&mut db_key).unwrap();

    Ok(db_key.to_vec())
}

static GEN_KEY_MUTEX: Mutex<()> = Mutex::new(());

/// Generate secret key if it does not exist.
pub fn generate_secret_key_if_needed(secret_key: &SecretKey) -> Result<()> {
    match secret_key.exists() {
        Ok(true) => Ok(()),
        Ok(false) => {
            let _lock = GEN_KEY_MUTEX.lock().unwrap();
            match secret_key.exists() {
                Ok(true) => Ok(()),
                Ok(false) => {
                    logi!("[INFO]The key does not exist, generate it.");
                    secret_key.generate()
                },
                Err(ret) => Err(ret),
            }
        },
        Err(ret) => Err(ret),
    }
}

fn encrypt_db_key(calling_info: &CallingInfo, db_key: &Vec<u8>) -> Result<Vec<u8>> {
    let secret_key = build_db_key_secret_key(calling_info)?;
    generate_secret_key_if_needed(&secret_key)?;
    let aad: Vec<u8> = "trivial_aad_for_db_key".as_bytes().to_vec();
    let db_key_cipher = Crypto::encrypt(&secret_key, db_key, &aad)?;

    Ok(db_key_cipher)
}

/// Read db key cipher and decrypt if the db key cipher file exists, generate db_key if not.
pub fn get_db_key(calling_info: &CallingInfo) -> Result<Vec<u8>> {
    match is_db_key_cipher_file_exist(calling_info.user_id()) {
        Ok(true) => {
            let db_key_cipher = read_db_key_cipher(calling_info.user_id())?;
            let db_key = decrypt_db_key_cipher(calling_info, &db_key_cipher)?;
            Ok(db_key)
        },
        Ok(false) => {
            let db_key = generate_db_key()?;
            let db_key_cipher = encrypt_db_key(calling_info, &db_key)?;
            write_db_key_cipher(calling_info.user_id(), &db_key_cipher)?;
            Ok(db_key)
        },
        Err(e) => Err(e),
    }
}

/// Create de db instance if the value of tag "RequireAttrEncrypted" is set to false, Create ce db instance if true.
pub fn create_db_instance(attributes: &AssetMap, calling_info: &CallingInfo) -> Result<Database> {
    match attributes.get(&Tag::RequireAttrEncrypted) {
        Some(Value::Bool(true)) => {
            let db_key = get_db_key(calling_info)?;
            let db = Database::build(calling_info.user_id(), Some(&db_key))?;
            Ok(db)
        },
        _ => {
            let db = Database::build(calling_info.user_id(), None)?;
            Ok(db)
        },
    }
}
