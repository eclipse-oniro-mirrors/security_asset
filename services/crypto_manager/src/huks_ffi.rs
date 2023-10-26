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

//! This module is used to adapt to the capabilities provided by the HUKS.

/// HuksErrcode type
pub type HuksErrcode = i32;
/// HuksErrcode Success
pub const HKS_SUCCESS: HuksErrcode = 0;
/// HuksErrcode Failure
pub const HKS_FAILURE: HuksErrcode = -1;
/// HuksErrcode NOt EXIST
pub const HKS_ERROR_NOT_EXIST: HuksErrcode = -13;

/// crypto type
pub type HksKeyPurpose = i32;
/// encrypto mode
pub const HKS_KEY_PURPOSE_ENCRYPT: HksKeyPurpose = 1;
/// decrypto mode
pub const HKS_KEY_PURPOSE_DECRYPT: HksKeyPurpose = 2;

/// Nonce size, keep same with huks_wrapper
pub const NONCE_SIZE: u32 = 12;
/// Aead size, keep same with huks_wrapper
pub const AEAD_SIZE: u32 = 16;

/// handle len is sizeof(uint64_t) / sizeof(u8) = 8, for HuksInit
pub const HANDLE_LEN: u32 = 8;

/// chanllenge len for HuksInit
pub const CHALLENGE_LEN: u32 = 32;

/// crypto params, like cryptoMode, challenge_pos
#[repr(C)]
pub struct CryptParam {
    /// crypto mode
    pub crypto_mode: HksKeyPurpose,
    /// challenge position
    pub challenge_pos: u32,
}

/// const crypto blobs, keep same with crypto_wrapper.h
#[repr(C)]
pub struct ConstCryptoBlob {
    /// keyinfo size
    pub size: u32,
    /// keyinfo buff
    pub data: *const u8,
}

/// crypto blobs, keep same with crypto_wrapper.h
#[repr(C)]
pub struct CryptoBlob {
    /// keyinfo size
    pub size: u32,
    /// keyinfo buff
    pub data: *mut u8,
}

extern "C" {
    /// c generate key
    pub fn GenerateKey(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// c delete key
    pub fn DeleteKey(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// c key exist
    pub fn KeyExist(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// hks encrypt c func
    pub fn EncryptWrapper(key_alias: *const ConstCryptoBlob, aad_data: *const ConstCryptoBlob,
        in_data: *const ConstCryptoBlob, out_data: *mut CryptoBlob) -> HuksErrcode;

    /// hks decrypt c func
    pub fn DecryptWrapper(key_alias: *const ConstCryptoBlob, aad_data: *const ConstCryptoBlob,
        in_data: *const ConstCryptoBlob, out_data: *mut CryptoBlob) -> HuksErrcode;

    /// hks crypto init c func
    pub fn InitCryptoWrapper(param: *const CryptParam, key_data: *const ConstCryptoBlob,
        challenge_data: *mut CryptoBlob, handle_data: *mut CryptoBlob) -> HuksErrcode;

    /// hks execute crypto c func
    pub fn ExecCryptoWrapper(param: *const CryptParam, aad_data: *const ConstCryptoBlob,
        handle_data: *const ConstCryptoBlob, in_data: *const ConstCryptoBlob,
        out_data: *mut CryptoBlob) -> HuksErrcode;

    /// hks crypto drop c func
    pub fn DropCrypto(param: *const CryptParam, handle_data: *mut CryptoBlob) -> HuksErrcode;
}
