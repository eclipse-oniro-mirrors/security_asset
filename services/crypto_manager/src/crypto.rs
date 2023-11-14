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

use std::sync::{Arc, Mutex};

use asset_definition::{log_throw_error, Accessibility, AssetError, AuthType, ErrCode, Result};
use asset_log::loge;
use asset_utils::hasher::sha256;
use asset_utils::time;

struct IdentityGuard {
    identity: String,
}

impl IdentityGuard {
    fn build() -> Result<Self> {
        let identity = ipc_rust::reset_calling_identity().map_err(|e| {
            AssetError::new(ErrCode::IpcError, format!("[FATAL][SA]Reset calling identity failed, error is [{}].", e))
        })?;
        Ok(Self { identity })
    }
}

impl Drop for IdentityGuard {
    fn drop(&mut self) {
        if !ipc_rust::set_calling_identity(self.identity.clone()) {
            loge!("[FATAL][SA]Set calling identity failed.");
        }
    }
}

/// Struct to store key attributes, excluding key materials.
#[derive(Clone)]
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    alias: Vec<u8>,
}

#[repr(C)]
struct HksBlob {
    size: u32,
    data: *const u8,
}

#[repr(C)]
struct OutBlob {
    size: u32,
    data: *mut u8,
}

extern "C" {
    fn GenerateKey(alias: *const HksBlob, need_auth: bool) -> i32;
    fn DeleteKey(alias: *const HksBlob) -> i32;
    fn IsKeyExist(alias: *const HksBlob) -> i32;
    fn EncryptData(alias: *const HksBlob, aad: *const HksBlob, in_data: *const HksBlob, out_data: *mut OutBlob) -> i32;
    fn DecryptData(alias: *const HksBlob, aad: *const HksBlob, in_data: *const HksBlob, out_data: *mut OutBlob) -> i32;
    fn InitKey(
        alias: *const HksBlob,
        valid_time: u32,
        challenge: *mut OutBlob,
        handle: *mut OutBlob,
    ) -> i32;
    fn ExecCrypt(
        handle: *const HksBlob,
        aad: *const HksBlob,
        auth_token: *const HksBlob,
        in_data: *const HksBlob,
        out_data: *mut OutBlob,
    ) -> i32;
    fn Drop(handle: *const HksBlob) -> i32;
}

const HKS_SUCCESS: i32 = 0;
const HKS_ERROR_NOT_EXIST: i32 = -13;
const NONCE_SIZE: usize = 12;
const TAG_SIZE: usize = 16;
const MAX_ALIAS_SIZE: usize = 64;
const HANDLE_LEN: usize = 8;
const CHALLENGE_LEN: usize = 32;

impl SecretKey {
    /// New a secret key.
    pub fn new(user_id: i32, owner: &Vec<u8>, auth_type: AuthType, access_type: Accessibility,
        require_password_set: bool) -> Self {
        let mut alias: Vec<u8> = Vec::with_capacity(MAX_ALIAS_SIZE);
        alias.extend_from_slice(&user_id.to_le_bytes());
        alias.push(b'_');
        alias.extend(owner);
        if auth_type != AuthType::None {
            alias.push(b'_');
            alias.extend_from_slice(&(auth_type as u32).to_le_bytes());
            loge!("alisa contain auth type")
        }
        if access_type != Accessibility::DeviceFirstUnlocked {
            alias.push(b'_');
            alias.extend_from_slice(&(access_type as u32).to_le_bytes());
            loge!("alisa contain accessibility")
        }
        if require_password_set {
            alias.push(b'_');
            alias.extend_from_slice(&(require_password_set as u32).to_le_bytes());
            loge!("alisa contain require_password_set")
        }
        alias = sha256(&alias);
        Self { auth_type, access_type, alias}
    }

    /// Check whether the secret key exists.
    pub fn exists(&self) -> Result<bool> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };

        let _identity = IdentityGuard::build()?;
        let ret = unsafe { IsKeyExist(&key_alias as *const HksBlob) };
        match ret {
            HKS_SUCCESS => Ok(true),
            HKS_ERROR_NOT_EXIST => Ok(false),
            _ => {
                log_throw_error!(ErrCode::CryptoError, "[FATAL]secret key exist check failed ret {}", ret)
            },
        }
    }

    /// Generate the secret key and store in HUKS.
    pub fn generate(&self) -> Result<()> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };
        let _identity = IdentityGuard::build()?;
        let ret = unsafe { GenerateKey(&key_alias as *const HksBlob, self.need_user_auth()) };
        match ret {
            HKS_SUCCESS => Ok(()),
            _ => {
                log_throw_error!(ErrCode::CryptoError, "[FATAL]secret key generate failed ret {}", ret)
            },
        }
    }

    /// Delete the secret key.
    pub fn delete(&self) -> Result<()> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };

        let _identity = IdentityGuard::build()?;
        let ret = unsafe { DeleteKey(&key_alias as *const HksBlob) };
        match ret {
            HKS_SUCCESS => Ok(()),
            _ => {
                log_throw_error!(ErrCode::CryptoError, "[FATAL]secret key delete failed ret {}", ret)
            },
        }
    }

    /// Determine whether user auth is required.
    pub fn need_user_auth(&self) -> bool {
        self.auth_type == AuthType::Any
    }

    /// Determine whether device unlock is required.
    pub fn need_device_unlock(&self) -> bool {
        self.access_type == Accessibility::DeviceUnlocked
    }
}

/// Crypto for storing key attributes that require user authentication.
pub struct Crypto {
    key: SecretKey,
    challenge: Vec<u8>,
    handle: Vec<u8>,
    valid_time: u32,
    exp_time: u64,
}

impl Crypto {
    /// Create a crypto instance.
    pub fn build(key: SecretKey, valid_time: u32) -> Result<Self> {
        let current_time = time::system_time_in_seconds()?;
        Ok(Self {
            key,
            challenge: vec![0; CHALLENGE_LEN],
            handle: vec![0; HANDLE_LEN],
            valid_time,
            exp_time: current_time + valid_time as u64,
        })
    }

    /// Init secret key and get challenge.
    pub fn init_key(&mut self) -> Result<&Vec<u8>> {
        let key_alias = HksBlob { size: self.key.alias.len() as u32, data: self.key.alias.as_ptr() };
        let mut challenge = OutBlob { size: self.challenge.len() as u32, data: self.challenge.as_mut_ptr() };
        let mut handle = OutBlob { size: self.handle.len() as u32, data: self.handle.as_mut_ptr() };

        let _identity = IdentityGuard::build()?;
        let ret = unsafe {
            InitKey(
                &key_alias as *const HksBlob,
                self.valid_time,
                &mut challenge as *mut OutBlob,
                &mut handle as *mut OutBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(&self.challenge),
            _ => log_throw_error!(ErrCode::CryptoError, "[FATAL]HUKS init key failed, ret: {}", ret),
        }
    }

    /// Decrypt data that requires user authentication.
    pub fn exec_crypt(&self, cipher: &Vec<u8>, aad: &Vec<u8>, auth_token: &Vec<u8>) -> Result<Vec<u8>> {
        if time::system_time_in_seconds()? >= self.exp_time {
            // todo: 超期要清理session
            return log_throw_error!(ErrCode::AuthTokenExpired, "[FATAL]The user authentication token has expired.");
        }

        if cipher.len() <= (TAG_SIZE + NONCE_SIZE) {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The cipher length is too short.");
        }

        let aad = HksBlob { size: aad.len() as u32, data: aad.as_ptr() };
        let auth_token = HksBlob { size: auth_token.len() as u32, data: auth_token.as_ptr() };
        let handle = HksBlob { size: self.handle.len() as u32, data: self.handle.as_ptr() };
        let in_data = HksBlob { size: cipher.len() as u32, data: cipher.as_ptr() };
        let mut msg: Vec<u8> = vec![0; cipher.len() - TAG_SIZE - NONCE_SIZE];
        let mut out_data = OutBlob { size: msg.len() as u32, data: msg.as_mut_ptr() };

        let _identity = IdentityGuard::build()?;
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
            HKS_SUCCESS => Ok(msg),
            _ => log_throw_error!(ErrCode::CryptoError, "[FATAL]HUKS execute crypt failed, ret: {}", ret),
        }
    }

    /// Encrypt data at one-time.
    pub fn encrypt(key: &SecretKey, msg: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>> {
        let mut cipher: Vec<u8> = vec![0; msg.len() + TAG_SIZE + NONCE_SIZE];
        let key_alias = HksBlob { size: key.alias.len() as u32, data: key.alias.as_ptr() };
        let aad_data = HksBlob { size: aad.len() as u32, data: aad.as_ptr() };
        let in_data = HksBlob { size: msg.len() as u32, data: msg.as_ptr() };
        let mut out_data = OutBlob { size: cipher.len() as u32, data: cipher.as_mut_ptr() };

        let _identity = IdentityGuard::build()?;
        let ret = unsafe {
            EncryptData(
                &key_alias as *const HksBlob,
                &aad_data as *const HksBlob,
                &in_data as *const HksBlob,
                &mut out_data as *mut OutBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(cipher),
            _ => log_throw_error!(ErrCode::CryptoError, "[FATAL]HUKS encrypt failed, ret: {}", ret),
        }
    }

    /// Encrypt data at one-time.
    pub fn decrypt(key: &SecretKey, cipher: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>> {
        if cipher.len() <= (TAG_SIZE + NONCE_SIZE) {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The cipher length is too short.");
        }

        let mut plain: Vec<u8> = vec![0; cipher.len() - TAG_SIZE - NONCE_SIZE];
        let key_alias = HksBlob { size: key.alias.len() as u32, data: key.alias.as_ptr() };
        let aad_data = HksBlob { size: aad.len() as u32, data: aad.as_ptr() };
        let in_data = HksBlob { size: cipher.len() as u32, data: cipher.as_ptr() };
        let mut out_data = OutBlob { size: plain.len() as u32, data: plain.as_mut_ptr() };

        let _identity = IdentityGuard::build()?;
        let ret = unsafe {
            DecryptData(
                &key_alias as *const HksBlob,
                &aad_data as *const HksBlob,
                &in_data as *const HksBlob,
                &mut out_data as *mut OutBlob,
            )
        };
        match ret {
            HKS_SUCCESS => Ok(plain),
            _ => log_throw_error!(ErrCode::CryptoError, "[FATAL]HUKS decrypt failed, ret: {}", ret),
        }
    }
}

impl Drop for Crypto {
    fn drop(&mut self) {
        let handle = HksBlob { size: self.handle.len() as u32, data: self.handle.as_ptr() };
        let identity = IdentityGuard::build();
        if identity.is_ok() {
            unsafe { Drop(&handle as *const HksBlob) };
        }
    }
}

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
        if self.cryptos.len() >= CRYPTO_CAPACITY {
            log_throw_error!(ErrCode::LimitExceeded, "The number of cryptos exceeds the upper limit.")
        } else {
            self.cryptos.push(crypto);
            Ok(())
        }
    }

    /// Remove the crypto from manager.
    pub fn remove(&mut self, challenge: &Vec<u8>) {
        self.cryptos.retain(|crypto| !crypto.challenge.eq(challenge));
    }

    /// Find the crypto with the specified alias and challenge slice from manager.
    pub fn find(&mut self, challenge: &Vec<u8>) -> Option<&Crypto> {
        for crypto in self.cryptos.iter() {
            if crypto.challenge.eq(challenge) {
                return Some(crypto);
            }
        }
        loge!("Can not found the crypto.");
        None
    }

    /// Remove cryptos that required device to be unlocked.
    pub fn remove_need_device_unlocked(&mut self) {
        self.cryptos.retain(|crypto| !crypto.key.need_device_unlock());
    }
}
