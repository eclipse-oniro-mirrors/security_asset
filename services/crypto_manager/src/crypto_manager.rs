/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This module is used to manage crypto in cache.

use std::sync::{Arc, Mutex};

use asset_definition::{log_throw_error, ErrCode, Result};
use asset_utils::time;

use crate::crypto::Crypto;

const CRYPTO_CAPACITY: usize = 16;

/// Manages the crypto that required user authentication.
pub struct CryptoManager {
    cryptos: Vec<Crypto>,
}

impl CryptoManager {
    fn new() -> Self {
        Self { cryptos: vec![] }
    }

    /// Get the single instance of CryptoManager.
    pub fn get_instance() -> Arc<Mutex<CryptoManager>> {
        static mut INSTANCE: Option<Arc<Mutex<CryptoManager>>> = None;
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(Mutex::new(CryptoManager::new()))).clone() }
    }

    /// Add the crypto to manager.
    pub fn add(&mut self, crypto: Crypto) -> Result<()> {
        self.remove_expired_crypto()?;
        if self.cryptos.len() >= CRYPTO_CAPACITY {
            log_throw_error!(ErrCode::LimitExceeded, "The number of cryptos exceeds the upper limit.")
        } else {
            self.cryptos.push(crypto);
            Ok(())
        }
    }

    /// Find the crypto with the specified alias and challenge slice from manager.
    pub fn find(&mut self, challenge: &Vec<u8>) -> Result<&Crypto> {
        self.remove_expired_crypto()?;
        for crypto in self.cryptos.iter() {
            if crypto.challenge().eq(challenge) {
                return Ok(crypto);
            }
        }
        log_throw_error!(ErrCode::NotFound, "The crypto expires or does not exist. Call the preQuery first.")
    }

    /// Remove the crypto from manager.
    pub fn remove(&mut self, challenge: &Vec<u8>) {
        self.cryptos.retain(|crypto| !crypto.challenge().eq(challenge));
    }

    /// Remove cryptos that required device to be unlocked.
    pub fn remove_need_device_unlocked(&mut self) {
        self.cryptos.retain(|crypto| !crypto.secret_key().need_device_unlock());
    }

    fn remove_expired_crypto(&mut self) -> Result<()> {
        let now = time::system_time_in_seconds()?;
        self.cryptos.retain(|crypto| crypto.expire_time() > now);
        Ok(())
    }
}
