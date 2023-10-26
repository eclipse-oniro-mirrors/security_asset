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

use asset_definition::{Accessibility, AuthType, ErrCode};
use asset_log::{loge, logi};
use std::sync::Arc;
use std::sync::Mutex;

use crate::huks_ffi::*;

/// SecretKey struct
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    alias: Vec<u8>,
}

const MAX_ALIAS_SIZE: u32 = 64;

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
        let alias = construct_alias(user_id, owner, auth_type, access_type);
        Self { auth_type, access_type, alias }
    }

    /// Check whether the secret key exists.
    pub fn exists(&self) -> Result<bool, HuksErrcode> { // todo: zdy exists去掉最后的s
        let ret = unsafe { KeyExist(self.alias.len() as u32, self.alias.as_ptr()) };
        match ret {
            HKS_SUCCESS => Ok(true),
            HKS_ERROR_NOT_EXIST => Ok(false),
            _ => Err(ret),
        }
    }

    /// Generate the secret key
    pub fn generate(&self) -> Result<(), HuksErrcode> {
        loge!("start to generate key!!!!");
        let ret = unsafe { GenerateKey(self.alias.len() as u32, self.alias.as_ptr()) };
        match ret {
            HKS_SUCCESS => Ok(()),
            _ => Err(ret),
        }
    }

    /// Delete the secret key.
    pub fn delete(&self) -> Result<bool, HuksErrcode> {
        // todo: zdy 不需要bool的返回值
        let ret = unsafe { DeleteKey(self.alias.len() as u32, self.alias.as_ptr()) };
        match ret {
            HKS_SUCCESS => Ok(true),
            _ => Err(ret),
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
    /// timeout time, reserved
    _exp_time: u32, // 最大10min
}

impl Drop for Crypto {
    fn drop(&mut self) {
        // in param
        let param = CryptParam {
            crypto_mode: self.mode,
            challenge_pos: self.challenge_pos,
        };

        let mut handle_data = CryptoBlob {
            size: self.handle.len() as u32,
            data: self.handle.as_mut_ptr(),
        };

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
        Self {
            key,
            mode,
            challenge: vec![0; CHALLENGE_LEN as usize],
            handle: vec![0; HANDLE_LEN as usize],
            challenge_pos,
            _exp_time: exp_time,
        }
    }

    /// Start HuksInit
    pub fn init_crypto(&mut self) -> Result<Vec<u8>, ErrCode> {
        // in param
        let param = CryptParam {
            crypto_mode: self.mode,
            challenge_pos: self.challenge_pos,
        };

        let key_data = ConstCryptoBlob {
            size: self.key.alias.len() as u32,
            data: self.key.alias.as_ptr(),
        };

        let mut challenge_data = CryptoBlob {
            size: self.challenge.len() as u32,
            data: self.challenge.as_mut_ptr(),
        };

        let mut handle_data = CryptoBlob {
            size: self.handle.len() as u32,
            data: self.handle.as_mut_ptr(),
        };

        let ret = unsafe { InitCryptoWrapper(&param as *const CryptParam, &key_data as *const ConstCryptoBlob,
            &mut challenge_data as *mut CryptoBlob, &mut handle_data as *mut CryptoBlob) };
        match ret {
            HKS_SUCCESS => Ok(self.challenge.clone()),
            _ => {
                loge!("crypto init failed ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    //todo：需要判断一下超时时间，返回超时错误码 AuthTokenExpired，需要增加authtoken入参
    /// Exec encrypt or decrypt
    pub fn exec_crypto(&self, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        // out param
        let mut cipher: Vec<u8> = vec![0; msg.len() + AEAD_SIZE as usize + NONCE_SIZE as usize];
        // in param
        let param = CryptParam {
            crypto_mode: self.mode,
            challenge_pos: self.challenge_pos,
        };

        let aad_data = ConstCryptoBlob {
            size: aad.len() as u32,
            data: aad.as_ptr(),
        };

        let handle_data = ConstCryptoBlob {
            size: self.handle.len() as u32,
            data: self.handle.as_ptr(),
        };

        let in_data = ConstCryptoBlob {
            size: msg.len() as u32,
            data: msg.as_ptr(),
        };

        let mut out_data = CryptoBlob {
            size: cipher.len() as u32,
            data: cipher.as_mut_ptr(),
        };

        let ret = unsafe { ExecCryptoWrapper(&param as *const CryptParam, &aad_data as *const ConstCryptoBlob,
            &handle_data as *const ConstCryptoBlob, &in_data as *const ConstCryptoBlob, &mut out_data as *mut CryptoBlob) };
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
        let key_alias = ConstCryptoBlob {
            size: key.alias.len() as u32,
            data: key.alias.as_ptr(),
        };

        let aad_data = ConstCryptoBlob {
            size: aad.len() as u32,
            data: aad.as_ptr(),
        };

        let in_data = ConstCryptoBlob {
            size: msg.len() as u32,
            data: msg.as_ptr(),
        };

        let mut out_data = CryptoBlob {
            size: cipher.len() as u32,
            data: cipher.as_mut_ptr(),
        };

        let ret = unsafe { EncryptWrapper(&key_alias as *const ConstCryptoBlob, &aad_data as *const ConstCryptoBlob,
            &in_data as *const ConstCryptoBlob, &mut out_data as *mut CryptoBlob) };
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
        let key_alias = ConstCryptoBlob {
            size: key.alias.len() as u32,
            data: key.alias.as_ptr(),
        };

        let aad_data = ConstCryptoBlob {
            size: aad.len() as u32,
            data: aad.as_ptr(),
        };

        let in_data = ConstCryptoBlob {
            size: cipher.len() as u32,
            data: cipher.as_ptr(),
        };

        let mut out_data = CryptoBlob {
            size: plain.len() as u32,
            data: plain.as_mut_ptr(),
        };

        let ret = unsafe { DecryptWrapper(&key_alias as *const ConstCryptoBlob, &aad_data as *const ConstCryptoBlob,
            &in_data as *const ConstCryptoBlob, &mut out_data as *mut CryptoBlob) };
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
    fn new() -> Self { //
        Self { crypto_vec: vec![], mutex: Mutex::new(0) }
    }

    /// get single instance for cryptomgr
    pub fn get_instance() -> Arc<Mutex<CryptoManager>> {
        static mut INSTANCE: Option<Arc<Mutex<CryptoManager>>> = None;
        unsafe {
            INSTANCE.get_or_insert_with(|| Arc::new(Mutex::new(CryptoManager::new()))).clone()
        }
    }

    fn challenge_cmp(challenge: &Vec<u8>, crypto: &Crypto) -> Result<(), ErrCode> {
        if challenge.len() != CHALLENGE_LEN as usize {
            loge!("invalid challenge len {}", challenge.len());
            return Err(ErrCode::CryptoError);
        }

        let index = (crypto.challenge_pos * 4) as usize;
        if challenge[index..(index + 8)] == crypto.challenge[index..(index + 8)] {
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
    pub fn find(&self, alias: &Vec<u8>, challenge: &Vec<u8>) -> Option<&Crypto> {
        let _lock = self.mutex.lock().unwrap();
        for crypto in self.crypto_vec.iter() {
            if alias.as_slice() != crypto.key.alias.as_slice() {
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
