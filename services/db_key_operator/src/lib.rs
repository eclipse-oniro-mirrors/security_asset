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
use asset_definition::{Accessibility, AuthType, Result, ErrCode, log_throw_error};
use asset_file_operator::ce_operator::{is_db_key_cipher_file_exist, read_db_key_cipher, write_db_key_cipher};
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

static GEN_KEY_MUTEX: Mutex<()> = Mutex::new(());
static GET_DB_KEY_MUTEX: Mutex<()> = Mutex::new(());

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

/// db key obj
pub struct DbKey {
    /// db key
    pub db_key: Vec<u8>,
}

impl DbKey {
    /// Decrypt db key cipher.
    pub fn decrypt_db_key_cipher(calling_info: &CallingInfo, db_key_cipher: &Vec<u8>) -> Result<DbKey> {
        let secret_key = build_db_key_secret_key(calling_info)?;
        let aad: Vec<u8> = "trivial_aad_for_db_key".as_bytes().to_vec();
        let db_key = Crypto::decrypt(&secret_key, db_key_cipher, &aad)?;
        Ok(Self { db_key })
    }

    fn generate_db_key() -> Result<DbKey> {
        const KEY_LEN_IN_BYTES: usize = 32; // aes-256-gcm requires key length 256 bits = 32 bytes.
        let mut db_key = [0; KEY_LEN_IN_BYTES];
        rand_bytes(&mut db_key).unwrap();
        Ok(Self { db_key: db_key.to_vec() })
    }

    fn encrypt_db_key(&self, calling_info: &CallingInfo) -> Result<Vec<u8>> {
        let secret_key = build_db_key_secret_key(calling_info)?;
        generate_secret_key_if_needed(&secret_key)?;
        let aad: Vec<u8> = "trivial_aad_for_db_key".as_bytes().to_vec();
        let db_key_cipher = Crypto::encrypt(&secret_key, &self.db_key, &aad)?;

        Ok(db_key_cipher)
    }

    /// Read db key cipher and decrypt if the db key cipher file exists, generate db_key if not.
    pub fn get_db_key(calling_info: &CallingInfo) -> Result<DbKey> {
        match is_db_key_cipher_file_exist(calling_info.user_id()) {
            Ok(_) => {
                let _lock = GET_DB_KEY_MUTEX.lock().unwrap();
                match is_db_key_cipher_file_exist(calling_info.user_id()) {
                    Ok(true) => {
                        let db_key_cipher = read_db_key_cipher(calling_info.user_id())?;
                        Self::decrypt_db_key_cipher(calling_info, &db_key_cipher)
                    },
                    Ok(false) => {
                        let db_key = Self::generate_db_key()?;
                        let db_key_cipher = db_key.encrypt_db_key(calling_info)?;
                        write_db_key_cipher(calling_info.user_id(), &db_key_cipher)?;
                        Ok(db_key)
                    },
                    Err(e) => Err(e),
                }
            },
            Err(_) => log_throw_error!(ErrCode::FileOperationError, "[FATAL][SA]]Get database key failed!")
        }
    }
}

impl Drop for DbKey {
    fn drop(&mut self) {
        self.db_key.fill(0);
    }
}
