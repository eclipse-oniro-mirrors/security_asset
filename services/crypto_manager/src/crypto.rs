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

use crate::huks_ffi::*;
use asset_common::{
    definition::{Accessibility, AuthType, ErrCode},
    loge,
};

/// SecretKey struct
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    alias: Vec<u8>,
}

const MAX_ALIAS_SIZE: u32 = 64;

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
    pub fn exists(&self) -> Result<bool, HuksErrcode> {
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
    pub key: SecretKey,
    // mode: CryptoMode,
    // challenge: Vec<u8>,
    // handle: Vec<u8>,
    // pos: ChallengePos,
    // exp_time: u32,
}

impl Crypto {
    /// Encrypt
    pub fn encrypt(&self, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        // out param
        let mut cipher: Vec<u8> = vec![0; msg.len() + AEAD_SIZE as usize + NONCE_SIZE as usize];
        // in param
        let data = CryptParam {
            key_len: self.key.alias.len() as u32,
            key_data: self.key.alias.as_ptr(),
            aad_len: aad.len() as u32,
            aad: aad.as_ptr(),
            data_in_len: msg.len() as u32,
            data_in: msg.as_ptr(),
            data_out_len: cipher.len() as u32,
            data_out: cipher.as_mut_ptr(),
        };

        let ret = unsafe { EncryptWrapper(&data as *const CryptParam) };
        match ret {
            HKS_SUCCESS => Ok(cipher),
            _ => {
                loge!("crypto error ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }

    /// Decrypt
    pub fn decrypt(&self, cipher: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        if cipher.len() <= (AEAD_SIZE + NONCE_SIZE) as usize {
            loge!("invalid cipher\n");
            return Err(ErrCode::InvalidArgument);
        }
        // out param
        let mut plain: Vec<u8> = vec![0; cipher.len() - AEAD_SIZE as usize - NONCE_SIZE as usize];
        // in param
        let data = CryptParam {
            key_len: self.key.alias.len() as u32,
            key_data: self.key.alias.as_ptr(),
            aad_len: aad.len() as u32,
            aad: aad.as_ptr(),
            data_in_len: cipher.len() as u32,
            data_in: cipher.as_ptr(),
            data_out_len: plain.len() as u32,
            data_out: plain.as_mut_ptr(),
        };

        let ret = unsafe { DecryptWrapper(&data as *const CryptParam) };
        match ret {
            HKS_SUCCESS => Ok(plain),
            _ => {
                loge!("crypto error ret {}", ret);
                Err(ErrCode::CryptoError)
            },
        }
    }
}
