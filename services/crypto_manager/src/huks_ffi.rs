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

/// HuksErrcode type
pub type HuksErrcode = i32;
/// HuksErrcode Success
pub const HKS_SUCCESS: HuksErrcode = 0;
/// HuksErrcode Failure
pub const HKS_FAILURE: HuksErrcode = -1;

/// Nonce size, keep same with huks_wrapper
pub const NONCE_SIZE: u32 = 12;
/// Aead size, keep same with huks_wrapper
pub const AEAD_SIZE: u32 = 16;

extern {
    /// c generate key
    pub fn GenerateKey(
        keyLen: u32,
        keyData: *const u8
    ) -> HuksErrcode;

    /// c delete key
    pub fn DeleteKey(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// c key exist
    pub fn KeyExist(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// hks encrypt c func
    pub fn EncryptWrapper(
        keyLen: u32, keyData: *const u8,
        aadLen: u32, aad: *const u8,
        msgLen: u32, msg: *const u8,
        cipherLen: u32, cipher: *mut u8
    ) -> HuksErrcode;

    /// hks decrypt c func
    pub fn DecryptWrapper(
        keyLen: u32, keyData: *const u8,
        aadLen: u32, aad: *const u8,
        cipherLen: u32, cipher: *const u8,
        plainLen: u32, plain: *mut u8
    ) -> HuksErrcode;
}
