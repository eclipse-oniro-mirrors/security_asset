//!
//! Copyright (C) 2023 Huawei Device Co., Ltd.
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//! http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!

//! This crate implements the asset
use crate::huks_ffi::*;
use asset_common::{definition::ErrCode, loge};

/// KeyInfo struct
pub struct KeyInfo {
    /// User id
    pub user_id: i32,
    /// Uid
    pub uid: u64,
    /// Auth_type
    pub auth_type: u32,
    /// Access_type
    pub access_type: u32,
}
/// SecretKey struct
pub struct SecretKey {
    /// SecretKey alias
    pub alias: String,
}
impl SecretKey {
    /// New a secret key
    pub fn new(info: KeyInfo) -> Self {
        Self {
            alias: format!("{}_{}_{}_{}", info.user_id, info.uid, info.auth_type, info.access_type),
        }
    }

    /// Check whether the secret key exists
    pub fn exists(&self) -> HuksErrcode {
        unsafe { KeyExist(self.alias.len() as u32, self.alias.as_ptr()) }
    }

    /// Generate the hukkey
    pub fn generate(&self) -> HuksErrcode {
        unsafe { GenerateKey(self.alias.len() as u32, self.alias.as_ptr()) }
    }

    /// Delete the hukkey
    pub fn delete(&self) -> HuksErrcode {
        unsafe { DeleteKey(self.alias.len() as u32, self.alias.as_ptr()) }
    }

    /// Determine whether user auth is required.
    pub fn need_user_auth(&self) -> bool {
        for (i, item) in self.alias.split('_').enumerate() {
            if i == 2 {
                return item == 1.to_string();
            }
        }
        false
    }

    /// Determine whether device unlock is required.
    pub fn need_device_unlock(&self) -> bool {
        for (i, item) in self.alias.split('_').enumerate() {
            if i == 3 {
                return item == 3.to_string();
            }
        }
        false
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
        let mut cipher: Vec<u8> = vec![0; msg.len() + AEAD_SIZE as usize + NONCE_SIZE as usize]; // ciper 为出参密文长度，后面需要增加16(aead), 之后需要增加12(nonce)字节长度,可考虑增加预留长度
        let ret = unsafe {
            EncryptWrapper(
                self.key.alias.len() as u32,
                self.key.alias.as_ptr(),
                aad.len() as u32,
                aad.as_ptr(),
                msg.len() as u32,
                msg.as_ptr(),
                cipher.len() as u32,
                cipher.as_mut_ptr(),
            )
        };

        if ret != HKS_SUCCESS {
            loge!("Encrypt Failed.");
            return Err(ErrCode::Failed); //CRYPTO_FAIL
        }

        Ok(cipher)
    }

    /// Decrypt
    pub fn decrypt(&self, cipher: &Vec<u8>, aad: &Vec<u8>) -> Result<Vec<u8>, ErrCode> {
        if cipher.len() <= (AEAD_SIZE + NONCE_SIZE) as usize {
            loge!("invalid cipher\n");
            return Err(ErrCode::Failed);
        }

        let mut plain: Vec<u8> = vec![0; cipher.len() - AEAD_SIZE as usize - NONCE_SIZE as usize];
        let ret = unsafe {
            DecryptWrapper(
                self.key.alias.len() as u32,
                self.key.alias.as_ptr(),
                aad.len() as u32,
                aad.as_ptr(),
                cipher.len() as u32,
                cipher.as_ptr(),
                plain.len() as u32,
                plain.as_mut_ptr(),
            )
        };

        if ret != HKS_SUCCESS {
            loge!("Decrypt Failed.");
            return Err(ErrCode::Failed); //CRYPTO_FAIL
        }

        Ok(plain)
    }
}
