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
    collections::HashMap,
    sync::{Arc, Mutex, OnceLock},
};

use asset_common::CallingInfo;
use asset_definition::{macros_lib, ErrCode, Result};
use asset_log::logw;

use crate::crypto::Crypto;

/// Per-application capacity of cryptos that require user authentication.
const CRYPTO_CAPACITY: usize = 16;

/// Total capacity of cryptos across all applications.
const CRYPTO_TOTAL_CAPACITY: usize = 32;

/// Manages the crypto that required user authentication.
/// Bucketed by `owner_info` (stable per app), so a single app cannot multiply its
/// quota by cycling target `user_id` or `group`.
pub struct CryptoManager {
    cryptos: HashMap<Vec<u8>, Vec<Crypto>>,
}

impl CryptoManager {
    fn new() -> Self {
        Self { cryptos: HashMap::new() }
    }

    /// Get the single instance of CryptoManager.
    pub fn get_instance() -> Arc<Mutex<CryptoManager>> {
        static INSTANCE: OnceLock<Arc<Mutex<CryptoManager>>> = OnceLock::new();
        INSTANCE.get_or_init(|| {
            logw!("Create instance for CryptoManager.");
            Arc::new(Mutex::new(CryptoManager::new()))
        }).clone()
    }

    /// Add the crypto to manager.
    pub fn add(&mut self, crypto: Crypto) -> Result<()> {
        self.remove_expired_crypto()?;
        let total: usize = self.cryptos.values().map(Vec::len).sum();
        if total >= CRYPTO_TOTAL_CAPACITY {
            macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
                ErrCode::LimitExceeded, "The total number of cryptos exceeds the upper limit.")
        } else {
            let owner_info = crypto.calling_info().owner_info().clone();
            let bucket_len = self.cryptos.get(&owner_info).map_or(0, Vec::len);
            if bucket_len >= CRYPTO_CAPACITY {
                macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
                    ErrCode::LimitExceeded, "The number of cryptos per application exceeds the upper limit.")
            } else {
                self.cryptos.entry(owner_info).or_default().push(crypto);
                Ok(())
            }
        }
    }

    /// Find the crypto with the specified alias and challenge slice from manager.
    pub fn find(&mut self, calling_info: &CallingInfo, challenge: &Vec<u8>) -> Result<&Crypto> {
        self.remove_expired_crypto()?;
        match self.cryptos.get(calling_info.owner_info()) {
            Some(bucket) => {
                for crypto in bucket.iter() {
                    if crypto.calling_info().eq(calling_info) && crypto.challenge().eq(challenge) {
                        return Ok(crypto);
                    }
                }
                macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
                    ErrCode::NotFound, "The crypto expires or does not exist. Call the preQuery first.")
            },
            None => macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
                ErrCode::NotFound, "The crypto expires or does not exist. Call the preQuery first."),
        }
    }

    /// Remove the crypto from manager.
    pub fn remove(&mut self, calling_info: &CallingInfo, challenge: &Vec<u8>) {
        let mut empty = false;
        if let Some(bucket) = self.cryptos.get_mut(calling_info.owner_info()) {
            bucket.retain(|crypto| !(crypto.calling_info().eq(calling_info) && crypto.challenge().eq(challenge)));
            empty = bucket.is_empty();
        }
        if empty {
            self.cryptos.remove(calling_info.owner_info());
        }
    }

    /// Remove the crypto by calling info.
    pub fn remove_by_calling_info(&mut self, calling_info: &CallingInfo) {
        let mut empty = false;
        if let Some(bucket) = self.cryptos.get_mut(calling_info.owner_info()) {
            bucket.retain(|crypto| crypto.calling_info() != calling_info);
            empty = bucket.is_empty();
        }
        if empty {
            self.cryptos.remove(calling_info.owner_info());
        }
    }

    /// Remove cryptos that required device to be unlocked.
    pub fn remove_need_device_unlocked(&mut self) {
        for bucket in self.cryptos.values_mut() {
            bucket.retain(|crypto| !crypto.key().need_device_unlock());
        }
        self.cryptos.retain(|_, bucket| !bucket.is_empty());
    }

    /// Get last crypto expire time.
    pub fn max_crypto_expire_duration(&mut self) -> u64 {
        self.remove_expired_crypto().unwrap();
        let mut max_time = 0;
        for bucket in self.cryptos.values() {
            for crypto in bucket {
                let remaining = (crypto.valid_time() as u64)
                    .saturating_sub(crypto.start_time().elapsed().as_secs());
                max_time = max(remaining, max_time)
            }
        }
        max_time
    }

    fn remove_expired_crypto(&mut self) -> Result<()> {
        for bucket in self.cryptos.values_mut() {
            bucket.retain(|crypto| crypto.start_time().elapsed().as_secs() <= crypto.valid_time() as u64);
        }
        self.cryptos.retain(|_, bucket| !bucket.is_empty());
        Ok(())
    }
}
