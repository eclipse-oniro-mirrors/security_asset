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

//! This module is used to implement cryptographic algorithm operations, including key usage.

use std::time::Instant;

use asset_constants::{transfer_error_code, SUCCESS};
use asset_definition::{log_throw_error, ErrCode, Result};

use crate::{secret_key::SecretKey, HksBlob, KeyId, OutBlob};

extern "C" {
    fn EncryptData(keyId: *const KeyId, aad: *const HksBlob, in_data: *const HksBlob, out_data: *mut OutBlob) -> i32;
    fn DecryptData(keyId: *const KeyId, aad: *const HksBlob, in_data: *const HksBlob, out_data: *mut OutBlob) -> i32;
    fn InitKey(keyId: *const KeyId, valid_time: u32, challenge: *mut OutBlob, handle: *mut OutBlob) -> i32;
    fn ExecCrypt(
        handle: *const HksBlob,
        aad: *const HksBlob,
        auth_token: *const HksBlob,
        in_data: *const HksBlob,
        out_data: *mut OutBlob,
    ) -> i32;
    fn Drop(handle: *const HksBlob) -> i32;
}

const NONCE_SIZE: usize = 12;
const TAG_SIZE: usize = 16;
const HANDLE_LEN: usize = 8;
const CHALLENGE_LEN: usize = 32;

/// Crypto for storing key attributes that require user authentication.
pub struct Crypto {
    key: SecretKey,
    challenge: Vec<u8>,
    handle: Vec<u8>,
    valid_time: u32,
    start_time: Instant,
}

impl Crypto {
    /// Create a crypto instance.
    pub fn build(key: SecretKey, valid_time: u32) -> Result<Self> {
        Ok(Self {
            key,
            challenge: vec![0; CHALLENGE_LEN],
            handle: vec![0; HANDLE_LEN],
            valid_time,
            start_time: Instant::now(),
        })
    }

    /// Init secret key and get challenge.
    pub fn init_key(&mut self) -> Result<&Vec<u8>> {
        let key_alias = HksBlob { size: self.key.alias().len() as u32, data: self.key.alias().as_ptr() };
        let mut challenge = OutBlob { size: self.challenge.len() as u32, data: self.challenge.as_mut_ptr() };
        let mut handle = OutBlob { size: self.handle.len() as u32, data: self.handle.as_mut_ptr() };
        let key_id = KeyId::new(self.key.calling_info().stored_user_id(), key_alias, self.key.access_type());

        let ret = unsafe {
            InitKey(
                &key_id as *const KeyId,
                self.valid_time,
                &mut challenge as *mut OutBlob,
                &mut handle as *mut OutBlob,
            )
        };
        match ret {
            SUCCESS => Ok(&self.challenge),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Decrypt data that requires user authentication.
    pub fn exec_crypt(&self, cipher: &Vec<u8>, aad: &Vec<u8>, auth_token: &Vec<u8>) -> Result<Vec<u8>> {
        if cipher.len() <= (TAG_SIZE + NONCE_SIZE) {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The cipher length is too short.");
        }

        let aad = HksBlob { size: aad.len() as u32, data: aad.as_ptr() };
        let auth_token = HksBlob { size: auth_token.len() as u32, data: auth_token.as_ptr() };
        let handle = HksBlob { size: self.handle.len() as u32, data: self.handle.as_ptr() };
        let in_data = HksBlob { size: cipher.len() as u32, data: cipher.as_ptr() };
        let mut msg: Vec<u8> = vec![0; cipher.len() - TAG_SIZE - NONCE_SIZE];
        let mut out_data = OutBlob { size: msg.len() as u32, data: msg.as_mut_ptr() };

        let ret = unsafe {
            ExecCrypt(
                &handle as *const HksBlob,
                &aad as *const HksBlob,
                &auth_token as *const HksBlob,
                &in_data as *const HksBlob,
                &mut out_data as *mut OutBlob,
            )
        };
        match ret {
            SUCCESS => Ok(msg),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Encrypt data at one-time.
    pub fn encrypt(key: &SecretKey, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>> {
        let mut cipher: Vec<u8> = vec![0; msg.len() + TAG_SIZE + NONCE_SIZE];
        let key_alias = HksBlob { size: key.alias().len() as u32, data: key.alias().as_ptr() };
        let aad_data = HksBlob { size: aad.len() as u32, data: aad.as_ptr() };
        let in_data = HksBlob { size: msg.len() as u32, data: msg.as_ptr() };
        let mut out_data = OutBlob { size: cipher.len() as u32, data: cipher.as_mut_ptr() };
        let key_id = KeyId::new(key.calling_info().stored_user_id(), key_alias, key.access_type());

        let ret = unsafe {
            EncryptData(
                &key_id as *const KeyId,
                &aad_data as *const HksBlob,
                &in_data as *const HksBlob,
                &mut out_data as *mut OutBlob,
            )
        };
        match ret {
            SUCCESS => Ok(cipher),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Encrypt data at one-time.
    pub fn decrypt(key: &SecretKey, cipher: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>> {
        if cipher.len() <= (TAG_SIZE + NONCE_SIZE) {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The cipher length is too short.");
        }

        let mut plain: Vec<u8> = vec![0; cipher.len() - TAG_SIZE - NONCE_SIZE];
        let key_alias = HksBlob { size: key.alias().len() as u32, data: key.alias().as_ptr() };
        let aad_data = HksBlob { size: aad.len() as u32, data: aad.as_ptr() };
        let in_data = HksBlob { size: cipher.len() as u32, data: cipher.as_ptr() };
        let mut out_data = OutBlob { size: plain.len() as u32, data: plain.as_mut_ptr() };
        let key_id = KeyId::new(key.calling_info().stored_user_id(), key_alias, key.access_type());

        let ret = unsafe {
            DecryptData(
                &key_id as *const KeyId,
                &aad_data as *const HksBlob,
                &in_data as *const HksBlob,
                &mut out_data as *mut OutBlob,
            )
        };
        match ret {
            SUCCESS => Ok(plain),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    pub(crate) fn key(&self) -> &SecretKey {
        &self.key
    }

    pub(crate) fn challenge(&self) -> &Vec<u8> {
        &self.challenge
    }

    pub(crate) fn start_time(&self) -> &Instant {
        &self.start_time
    }

    pub(crate) fn valid_time(&self) -> u32 {
        self.valid_time
    }
}

impl Drop for Crypto {
    fn drop(&mut self) {
        let handle = HksBlob { size: self.handle.len() as u32, data: self.handle.as_ptr() };
        unsafe { Drop(&handle as *const HksBlob) };
    }
}
