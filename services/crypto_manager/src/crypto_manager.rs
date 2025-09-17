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

//! This module is used to manage crypto in cache.

use std::{
    cmp::max,
    sync::{Arc, Mutex, OnceLock},
};

use asset_common::CallingInfo;
use asset_definition::{log_throw_error, ErrCode, Result};
use asset_log::logw;

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
        static INSTANCE: OnceLock<Arc<Mutex<CryptoManager>>> = OnceLock::new();
        INSTANCE.get_or_init(|| {
            logw!("create instance for CryptoManager.");
            Arc::new(Mutex::new(CryptoManager::new()))
        }).clone()
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
    pub fn find(&mut self, calling_info: &CallingInfo, challenge: &Vec<u8>) -> Result<&Crypto> {
        self.remove_expired_crypto()?;
        for crypto in self.cryptos.iter() {
            if crypto.challenge().eq(challenge) && crypto.calling_info().eq(calling_info) {
                return Ok(crypto);
            }
        }
        log_throw_error!(ErrCode::NotFound, "The crypto expires or does not exist. Call the preQuery first.")
    }

    /// Remove the crypto from manager.
    pub fn remove(&mut self, calling_info: &CallingInfo, challenge: &Vec<u8>) {
        self.cryptos.retain(|crypto| crypto.calling_info() != calling_info || !crypto.challenge().eq(challenge));
    }

    /// Remove the crypto by calling info.
    pub fn remove_by_calling_info(&mut self, calling_info: &CallingInfo) {
        self.cryptos.retain(|crypto| crypto.calling_info() != calling_info);
    }

    /// Remove cryptos that required device to be unlocked.
    pub fn remove_need_device_unlocked(&mut self) {
        self.cryptos.retain(|crypto| !crypto.key().need_device_unlock());
    }

    /// Get last crypto expire time.
    pub fn max_crypto_expire_duration(&mut self) -> u64 {
        self.remove_expired_crypto().unwrap();
        let mut max_time = 0;
        for crypto in &self.cryptos {
            max_time = max(crypto.valid_time() as u64 - crypto.start_time().elapsed().as_secs(), max_time)
        }
        max_time
    }

    fn remove_expired_crypto(&mut self) -> Result<()> {
        self.cryptos.retain(|crypto| crypto.start_time().elapsed().as_secs() <= crypto.valid_time() as u64);
        Ok(())
    }
}
