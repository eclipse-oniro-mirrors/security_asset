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

use std::sync::Mutex;

use asset_common::SUCCESS;
use asset_definition::{log_throw_error, Accessibility, AuthType, ErrCode, Result};
use asset_file_operator::{ce_operator::*, common::is_ce_db_exist};
use asset_log::{logi, loge};

use crate::{crypto::Crypto, secret_key::SecretKey};

const TRIVIAL_AAD_FOR_DB_KEY: &str = "trivial_aad_for_db_key";
static GEN_KEY_MUTEX: Mutex<()> = Mutex::new(());
static GET_DB_KEY_MUTEX: Mutex<()> = Mutex::new(());

fn build_db_key_secret_key(user_id: i32) -> Result<SecretKey> {
    let auth_type = AuthType::None;
    let access_type = Accessibility::DeviceFirstUnlocked;
    let require_password_set = false;
    let alias = "db_key_secret_key".as_bytes().to_vec();

    SecretKey::new_with_alias(user_id, auth_type, access_type, require_password_set, alias)
}

fn check_validity_of_db_key(user_id: i32) -> Result<()> {
    if is_ce_db_exist(user_id)? && !DbKey::check_existance(user_id)? {
        loge!("[FATAL]There is database but no database key. Now all data should be cleared and restart over.");
        remove_ce_files(user_id)?;
        return log_throw_error!(ErrCode::DataCorrupted, "[FATAL]All data is cleared in {}.", user_id);
    }
    Ok(())
}

/// try to get db_key
pub fn get_db_key(user_id: i32, is_ce: bool) -> Result<Option<Vec<u8>>> {
    if !is_ce {
        return Ok(None);
    }
    check_validity_of_db_key(user_id)?;
    match DbKey::get_db_key(user_id) {
        Ok(key) => Ok(Some(key.db_key.clone())),
        Err(e) if e.code == ErrCode::NotFound || e.code == ErrCode::DataCorrupted => {
            loge!(
                "[FATAL]The key is corrupted. Now all data should be cleared and restart over, err is {}.",
                e.code
            );
            remove_ce_files(user_id)?;
            log_throw_error!(ErrCode::DataCorrupted, "[FATAL]All data is cleared in {}.", user_id)
        },
        Err(e) => Err(e),
    }
}

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

extern "C" {
    fn GenerateRandom(random: *mut u8, random_len: u32) -> i32;
}

/// db key obj
pub struct DbKey {
    /// db key
    pub db_key: Vec<u8>,
}

impl DbKey {
    fn decrypt_db_key_cipher(user_id: i32, db_key_cipher: &Vec<u8>) -> Result<DbKey> {
        let secret_key = build_db_key_secret_key(user_id)?;
        let aad: Vec<u8> = TRIVIAL_AAD_FOR_DB_KEY.as_bytes().to_vec();
        let db_key = Crypto::decrypt(&secret_key, db_key_cipher, &aad)?;
        Ok(Self { db_key })
    }

    fn generate_db_key() -> Result<DbKey> {
        const KEY_LEN_IN_BYTES: usize = 32; // aes-256-gcm requires key length 256 bits = 32 bytes.
        let mut db_key = [0; KEY_LEN_IN_BYTES];

        if unsafe { GenerateRandom(db_key.as_mut_ptr(), db_key.len() as u32) } != SUCCESS {
            return log_throw_error!(ErrCode::CryptoError, "[FATAL]Generate random failed!");
        }
        Ok(Self { db_key: db_key.to_vec() })
    }

    fn encrypt_db_key(&self, user_id: i32) -> Result<Vec<u8>> {
        let secret_key = build_db_key_secret_key(user_id)?;
        generate_secret_key_if_needed(&secret_key)?;
        let aad: Vec<u8> = TRIVIAL_AAD_FOR_DB_KEY.as_bytes().to_vec();
        let db_key_cipher = Crypto::encrypt(&secret_key, &self.db_key, &aad)?;

        Ok(db_key_cipher)
    }

    /// Check whether the database key exists.
    pub fn check_existance(user_id: i32) -> Result<bool> {
        is_db_key_cipher_file_exist(user_id)
    }

    /// Read db key cipher and decrypt if the db key cipher file exists, generate db_key if not.
    pub fn get_db_key(user_id: i32) -> Result<DbKey> {
        match is_db_key_cipher_file_exist(user_id) {
            Ok(true) => {
                let db_key_cipher = read_db_key_cipher(user_id)?;
                Self::decrypt_db_key_cipher(user_id, &db_key_cipher)
            },
            Ok(false) => {
                let _lock = GET_DB_KEY_MUTEX.lock().unwrap();
                match is_db_key_cipher_file_exist(user_id) {
                    Ok(true) => {
                        let db_key_cipher = read_db_key_cipher(user_id)?;
                        Self::decrypt_db_key_cipher(user_id, &db_key_cipher)
                    },
                    Ok(false) => {
                        let db_key = Self::generate_db_key()?;
                        let db_key_cipher = db_key.encrypt_db_key(user_id)?;
                        write_db_key_cipher(user_id, &db_key_cipher)?;
                        Ok(db_key)
                    },
                    Err(e) => Err(e),
                }
            },
            Err(e) => Err(e),
        }
    }
}

impl Drop for DbKey {
    fn drop(&mut self) {
        self.db_key.fill(0);
    }
}
