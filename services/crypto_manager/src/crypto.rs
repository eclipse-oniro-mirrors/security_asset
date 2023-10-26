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

//! This module is used to implement cryptographic algorithm operations, including key generation and usage.

use std::sync::Arc;
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};
use asset_definition::{Accessibility, AuthType, ErrCode};
use asset_log::{loge, logi};

use crate::huks_ffi::*;

/// SecretKey struct
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    alias: Vec<u8>,
}

const MAX_ALIAS_SIZE: u32 = 64;
const VALIAD_CHALLENGE_LEN: usize = 8;

/// construct alias
pub fn construct_alias(user_id: i32, owner: &Vec<u8>, auth_type: AuthType, access_type: Accessibility) -> Vec<u8> {
    let mut alias: Vec<u8> = Vec::with_capacity(MAX_ALIAS_SIZE as usize);
    alias.extend_from_slice(&user_id.to_le_bytes());
    alias.push(b'_');
    alias.extend(owner);
    alias.push(b'_');
    alias.extend_from_slice(&(auth_type as u32).to_le_bytes());
    alias.push(b'_');
    alias.extend_from_slice(&(access_type as u32).to_le_bytes());
    alias
}

impl SecretKey {
    /// New a secret key
    pub fn new(user_id: i32, owner: &Vec<u8>, auth_type: AuthType, access_type: Accessibility) -> Self {
        let mut alias: Vec<u8> = Vec::with_capacity(MAX_ALIAS_SIZE as usize);
        alias.extend_from_slice(&user_id.to_le_bytes());
        alias.push(b'_');
        alias.extend(owner);
        alias.push(b'_');
        alias.extend_from_slice(&(auth_type as u32).to_le_bytes());
        alias.push(b'_');
        alias.extend_from_slice(&(access_type as u32).to_le_bytes());
        Self { auth_type, access_type, alias }
    }

    /// Check whether the secret key exists.
    pub fn exists(&self) -> Result<bool, ErrCode> {
        let key_data = ConstCryptoBlob {
            size: self.alias.len() as u32,
            data: self.alias.as_ptr(),
        };

        let ret = unsafe { KeyExist(&key_data as *const ConstCryptoBlob) };
        match ret {
            HKS_SUCCESS => Ok(true),
            HKS_ERROR_NOT_EXIST => Ok(false),
            _ => {
                loge!("secret key exist check failed ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    /// Generate the secret key
    pub fn generate(&self) -> Result<(), ErrCode> {
        loge!("start to generate key!!!!");
        let key_data = ConstCryptoBlob {
            size: self.alias.len() as u32,
            data: self.alias.as_ptr(),
        };

        let ret = unsafe { GenerateKey(&key_data as *const ConstCryptoBlob) };
        match ret {
            HKS_SUCCESS => Ok(()),
            _ => {
                loge!("secret key generate failed ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    /// Delete the secret key.
    pub fn delete(&self) -> Result<(), ErrCode> {
        let key_data = ConstCryptoBlob {
            size: self.alias.len() as u32,
            data: self.alias.as_ptr(),
        };

        let ret = unsafe { DeleteKey(&key_data as *const ConstCryptoBlob) };
        match ret {
            HKS_SUCCESS => Ok(()),
            _ => {
                loge!("secret key delete failed ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    /// Determine whether user auth is required.
    pub fn need_user_auth(&self) -> bool {
        self.auth_type == AuthType::Any
    }

    /// Determine whether device unlock is required.
    pub fn need_device_unlock(&self) -> bool {
        self.access_type == Accessibility::DeviceUnlock
    }
}

/// Crypto struct
pub struct Crypto {
    /// Crypto secretkey
    key: SecretKey,
    /// crypto mode for crypto
    mode: HksKeyPurpose,
    /// chanllenge from HksInit
    challenge: Vec<u8>,
    /// handle from HksInit
    handle: Vec<u8>,
    /// challege position for huks
    challenge_pos: u32,
    /// timeout time, reserved, 600s max
    exp_time: u64,
}

impl Drop for Crypto {
    fn drop(&mut self) {
        // in param
        let param = CryptParam {
            crypto_mode: self.mode,
            challenge_pos: self.challenge_pos,
            exp_time: 0, // no use
        };

        let mut handle_data = CryptoBlob { size: self.handle.len() as u32, data: self.handle.as_mut_ptr() };

        let ret = unsafe { DropCrypto(&param as *const CryptParam, &mut handle_data as *mut CryptoBlob) };
        match ret {
            HKS_SUCCESS => logi!("crypto drop finish success\n"),
            _ => loge!("crypto drop finish failed ret {}", ret),
        }
    }
}

impl Crypto {
    /// New a crypto struct
    pub fn new(mode: HksKeyPurpose, key: SecretKey, challenge_pos: u32, exp_time: u32) -> Self {
        let now = SystemTime::now();
        let since_the_epoch = now.duration_since(UNIX_EPOCH).expect("Time went backwards");
        let now_second = since_the_epoch.as_secs();
        logi!("now time is {}", now_second);

        Self {
            key,
            mode,
            challenge: vec![0; CHALLENGE_LEN as usize],
            handle: vec![0; HANDLE_LEN as usize],
            challenge_pos,
            exp_time: now_second + exp_time as u64,
        }
    }

    /// Start HuksInit
    pub fn init_crypto(&mut self) -> Result<Vec<u8>, ErrCode> {
        let now = SystemTime::now();
        let since_the_epoch = now.duration_since(UNIX_EPOCH).expect("Time went backwards");
        let now_second = since_the_epoch.as_secs();
        logi!("now time is {}", now_second);
        if now_second >= self.exp_time {
            loge!("init crypto time expired {}", now_second);
            return Err(ErrCode::AuthTokenExpired);
        }

        // in param
        let param = CryptParam {
            crypto_mode: self.mode,
            challenge_pos: self.challenge_pos,
            exp_time: (self.exp_time - now_second) as u32,
        };

        let key_data = ConstCryptoBlob { size: self.key.alias.len() as u32, data: self.key.alias.as_ptr() };

        // out param
        let mut challenge_data = CryptoBlob {
            size: self.challenge.len() as u32,
            data: self.challenge.as_mut_ptr(),
        };

        let mut handle_data = CryptoBlob { size: self.handle.len() as u32, data: self.handle.as_mut_ptr() };

        let ret = unsafe {
            InitCryptoWrapper(
                &param as *const CryptParam,
                &key_data as *const ConstCryptoBlob,
                &mut challenge_data as *mut CryptoBlob,
                &mut handle_data as *mut CryptoBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(self.challenge.clone()),
            _ => {
                loge!("crypto init failed ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    //todo：需要增加authtoken入参
    /// Exec encrypt or decrypt
    pub fn exec_crypto(&self, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        let now = SystemTime::now();
        let since_the_epoch = now.duration_since(UNIX_EPOCH).expect("Time went backwards");
        let now_second = since_the_epoch.as_secs();
        logi!("now time is {}", now_second);
        if now_second >= self.exp_time {
            loge!("exec crypto time expired {}", now_second);
            return Err(ErrCode::AuthTokenExpired);
        }

        // out param
        let mut cipher: Vec<u8> = vec![0; msg.len() + AEAD_SIZE as usize + NONCE_SIZE as usize];
        // in param
        let param = CryptParam {
            crypto_mode: self.mode,
            challenge_pos: self.challenge_pos,
            exp_time: 0, // no use
        };

        let aad_data = ConstCryptoBlob { size: aad.len() as u32, data: aad.as_ptr() };

        let handle_data = ConstCryptoBlob { size: self.handle.len() as u32, data: self.handle.as_ptr() };

        let in_data = ConstCryptoBlob { size: msg.len() as u32, data: msg.as_ptr() };

        let mut out_data = CryptoBlob { size: cipher.len() as u32, data: cipher.as_mut_ptr() };

        let ret = unsafe {
            ExecCryptoWrapper(
                &param as *const CryptParam,
                &aad_data as *const ConstCryptoBlob,
                &handle_data as *const ConstCryptoBlob,
                &in_data as *const ConstCryptoBlob,
                &mut out_data as *mut CryptoBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(cipher),
            _ => {
                loge!("execute crypto error ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    /// Signle function call for encrypt
    pub fn encrypt(key: &SecretKey, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        // out param
        let mut cipher: Vec<u8> = vec![0; msg.len() + AEAD_SIZE as usize + NONCE_SIZE as usize];
        let key_alias = ConstCryptoBlob { size: key.alias.len() as u32, data: key.alias.as_ptr() };

        let aad_data = ConstCryptoBlob { size: aad.len() as u32, data: aad.as_ptr() };

        let in_data = ConstCryptoBlob { size: msg.len() as u32, data: msg.as_ptr() };

        let mut out_data = CryptoBlob { size: cipher.len() as u32, data: cipher.as_mut_ptr() };

        let ret = unsafe {
            EncryptWrapper(
                &key_alias as *const ConstCryptoBlob,
                &aad_data as *const ConstCryptoBlob,
                &in_data as *const ConstCryptoBlob,
                &mut out_data as *mut CryptoBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(cipher),
            _ => {
                loge!("encrypto error ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    /// Signle function call for decrypt
    pub fn decrypt(key: &SecretKey, cipher: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        if cipher.len() <= (AEAD_SIZE + NONCE_SIZE) as usize {
            loge!("invalid cipher\n");
            return Err(ErrCode::InvalidArgument);
        }
        // out param
        let mut plain: Vec<u8> = vec![0; cipher.len() - AEAD_SIZE as usize - NONCE_SIZE as usize];
        let key_alias = ConstCryptoBlob { size: key.alias.len() as u32, data: key.alias.as_ptr() };

        let aad_data = ConstCryptoBlob { size: aad.len() as u32, data: aad.as_ptr() };

        let in_data = ConstCryptoBlob { size: cipher.len() as u32, data: cipher.as_ptr() };

        let mut out_data = CryptoBlob { size: plain.len() as u32, data: plain.as_mut_ptr() };

        let ret = unsafe {
            DecryptWrapper(
                &key_alias as *const ConstCryptoBlob,
                &aad_data as *const ConstCryptoBlob,
                &in_data as *const ConstCryptoBlob,
                &mut out_data as *mut CryptoBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(plain),
            _ => {
                loge!("decrypto error ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }
}

/// Crypto Manager struct
pub struct CryptoManager {
    crypto_vec: Vec<Crypto>,
    mutex: Mutex<u32>,
}

/// default for crypto manager
impl Default for CryptoManager {
    fn default() -> Self {
        Self::new()
    }
}

/// concurrency is not handlled in these impl, plese handle it
impl CryptoManager {
    /// new crypto manager
    fn new() -> Self {
        //
        Self { crypto_vec: vec![], mutex: Mutex::new(0) }
    }

    /// get single instance for cryptomgr
    pub fn get_instance() -> Arc<Mutex<CryptoManager>> {
        static mut INSTANCE: Option<Arc<Mutex<CryptoManager>>> = None;
        unsafe { INSTANCE.get_or_insert_with(|| Arc::new(Mutex::new(CryptoManager::new()))).clone() }
    }

    fn challenge_cmp(challenge: &Vec<u8>, crypto: &Crypto) -> Result<(), ErrCode> {
        if challenge.len() != CHALLENGE_LEN as usize {
            loge!("invalid challenge len {}", challenge.len());
            return Err(ErrCode::CryptoError);
        }

        let index = crypto.challenge_pos as usize;
        if get_valiad_challenge(challenge, index) == get_valiad_challenge(&crypto.challenge, index) {
            return Ok(());
        }

        Err(ErrCode::CryptoError)
    }

    /// add a crypto in manager, not allow insert crypto with same challenge
    pub fn add(&mut self, crypto: Crypto) -> Result<(), ErrCode> {
        let _lock = self.mutex.lock().unwrap();
        for temp_crypto in self.crypto_vec.iter() {
            if crypto.challenge.as_slice() == temp_crypto.challenge.as_slice() {
                loge!("crypto manager not allow insert crypto with same challenge");
                return Err(ErrCode::CryptoError);
            }
        }
        self.crypto_vec.push(crypto);
        Ok(())
    }

    /// remove a crypto in manager
    pub fn remove(&mut self, challenge: &Vec<u8>) {
        let _lock = self.mutex.lock().unwrap();
        let mut delete_index: Vec<usize> = vec![];
        for (index, crypto) in self.crypto_vec.iter().enumerate() {
            match Self::challenge_cmp(challenge, crypto) {
                Ok(()) => continue,
                _ => delete_index.push(index),
            }
        }

        let delete_num = delete_index.len();
        delete_index.sort();
        for x in 0..delete_num {
            self.crypto_vec.remove(delete_index[delete_num - x - 1]);
        }
    }

    /// find a crypto in manager, donnot use this function return value with add&remove
    pub fn find(&self, secret_key: &SecretKey, challenge: &Vec<u8>) -> Option<&Crypto> {
        let _lock = self.mutex.lock().unwrap();
        for crypto in self.crypto_vec.iter() {
            if secret_key.alias.as_slice() != crypto.key.alias.as_slice() {
                continue;
            }

            match Self::challenge_cmp(challenge, crypto) {
                Ok(()) => {
                    return Some(crypto);
                },
                _ => continue,
            }
        }
        loge!("crypto not found\n");
        None
    }

    /// remove device_unlock crypto in crypto mgr
    pub fn remove_device_unlock(&mut self) {
        let _lock = self.mutex.lock().unwrap();
    }
}

/// get valiad challenge
pub fn get_valiad_challenge(challenge: &[u8], index: usize) -> &[u8] {
    let start = index * VALIAD_CHALLENGE_LEN;
    let end = start + VALIAD_CHALLENGE_LEN;
    &challenge[start..end]
}

/// set valiad challenge
pub fn set_valiad_challenge(valiad_challenge: &[u8], index: usize, challenge: &mut [u8]) {
    let start = index * VALIAD_CHALLENGE_LEN;
    let end = start + VALIAD_CHALLENGE_LEN;
    challenge[start..end].copy_from_slice(valiad_challenge);
}
