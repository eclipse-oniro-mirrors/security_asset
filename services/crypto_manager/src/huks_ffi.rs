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

//! This crate implements the asset

/// HuksErrcode type
pub type HuksErrcode = i32;
/// HuksErrcode Success
pub const HKS_SUCCESS: HuksErrcode = 0;
/// HuksErrcode Failure
pub const HKS_FAILURE: HuksErrcode = -1;
/// HuksErrcode NOt EXIST
pub const HKS_ERROR_NOT_EXIST: HuksErrcode = -13;

/// Nonce size, keep same with huks_wrapper
pub const NONCE_SIZE: u32 = 12;
/// Aead size, keep same with huks_wrapper
pub const AEAD_SIZE: u32 = 16;

/// crypto params for crypt_wrapper, keep same with crypto_wrapper.h
#[repr(C)]
pub struct CryptParam {
    /// keyinfo size
    pub key_len: u32,
    /// keyinfo buff
    pub key_data: *const u8,
    /// asociate data size
    pub aad_len: u32,
    /// asociate data buff
    pub aad: *const u8,
    /// encrypt&decrypt input data len
    pub data_in_len: u32,
    /// encrypt&decrypt input data buff
    pub data_in: *const u8,
    /// encrypt&decrypt output data len
    pub data_out_len: u32,
    /// encrypt&decrypt output data buff
    pub data_out: *const u8,
}

extern "C" {
    /// c generate key
    pub fn GenerateKey(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// c delete key
    pub fn DeleteKey(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// c key exist
    pub fn KeyExist(keyLen: u32, keyData: *const u8) -> HuksErrcode;

    /// hks encrypt c func
    pub fn EncryptWrapper(data: *const CryptParam) -> HuksErrcode;

    /// hks decrypt c func
    pub fn DecryptWrapper(data: *const CryptParam) -> HuksErrcode;
}
