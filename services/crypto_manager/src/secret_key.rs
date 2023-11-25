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

//! This module is used to implement cryptographic algorithm operations, including key generation.

use asset_constants::CallingInfo;
use asset_definition::{log_throw_error, Accessibility, AuthType, ErrCode, Result};
use asset_utils::hasher;

use crate::identity_scope::IdentityScope;
use crate::{HksBlob, KeyId};

/// Struct to store key attributes, excluding key materials.
#[derive(Clone)]
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
    user_id: i32,
    alias: Vec<u8>,
}

extern "C" {
    fn GenerateKey(keyId: *const KeyId, need_auth: bool, require_password_set: bool) -> i32;
    fn DeleteKey(alias: *const HksBlob) -> i32;
    fn IsKeyExist(alias: *const HksBlob) -> i32;
}

const HKS_SUCCESS: i32 = 0;
const MAX_ALIAS_SIZE: usize = 64;
const HKS_ERROR_NOT_EXIST: i32 = -13;

fn append_attr<T>(tag: &str, value: T, vec: &mut Vec<u8>)
where
    T: Default + std::cmp::PartialEq,
    u32: std::convert::From<T>,
{
    if value != T::default() {
        vec.push(b'_');
        vec.extend_from_slice(tag.as_bytes());
        vec.push(b':');
        vec.extend_from_slice(&u32::from(value).to_le_bytes());
    }
}

fn calculate_key_alias(
    calling_info: &CallingInfo,
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
) -> Vec<u8> {
    let mut alias: Vec<u8> = Vec::with_capacity(MAX_ALIAS_SIZE);
    alias.extend_from_slice(&calling_info.user_id().to_le_bytes());
    alias.push(b'_');
    alias.extend_from_slice(&calling_info.owner_type().to_le_bytes());
    alias.push(b'_');
    alias.extend(calling_info.owner_info());
    append_attr::<AuthType>("AuthType", auth_type, &mut alias);
    append_attr::<Accessibility>("Accessibility", access_type, &mut alias);
    append_attr::<bool>("RequirePasswordSet", require_password_set, &mut alias);
    hasher::sha256(&alias)
}

impl SecretKey {
    /// New a secret key.
    pub fn new(
        calling_info: &CallingInfo,
        auth_type: AuthType,
        access_type: Accessibility,
        require_password_set: bool,
    ) -> Self {
        let alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set);
        Self { auth_type, access_type, require_password_set, user_id: calling_info.user_id(), alias }
    }

    /// Check whether the secret key exists.
    pub fn exists(&self) -> Result<bool> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };

        let _identity = IdentityScope::build()?;
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
        let key_id = KeyId::new(self.user_id, key_alias, self.access_type);
        let _identity = IdentityScope::build()?;
        let ret = unsafe { GenerateKey(&key_id as *const KeyId, self.need_user_auth(), self.require_password_set) };
        match ret {
            HKS_SUCCESS => Ok(()),
            _ => {
                log_throw_error!(ErrCode::CryptoError, "[FATAL]secret key generate failed ret {}", ret)
            },
        }
    }

    /// Delete the secret key.
    pub fn delete(&self) -> Result<()> {
        Self::delete_key(&self.alias)
    }

    /// Delete secret key by owner.
    pub fn delete_by_owner(calling_info: &CallingInfo) -> Result<()> {
        let mut res = Ok(());
        let accessibilitys =
            [Accessibility::DevicePowerOn, Accessibility::DeviceFirstUnlocked, Accessibility::DeviceUnlocked];
        for accessibility in accessibilitys.into_iter() {
            let alias = calculate_key_alias(calling_info, AuthType::None, accessibility, true);
            let tmp = Self::delete_key(&alias);
            res = if tmp.is_err() { tmp } else { res };

            let alias = calculate_key_alias(calling_info, AuthType::Any, accessibility, true);
            let tmp = Self::delete_key(&alias);
            res = if tmp.is_err() { tmp } else { res };

            let alias = calculate_key_alias(calling_info, AuthType::None, accessibility, false);
            let tmp = Self::delete_key(&alias);
            res = if tmp.is_err() { tmp } else { res };

            let alias = calculate_key_alias(calling_info, AuthType::Any, accessibility, false);
            let tmp = Self::delete_key(&alias);
            res = if tmp.is_err() { tmp } else { res };
        }
        res
    }

    fn delete_key(alias: &Vec<u8>) -> Result<()> {
        let key_alias = HksBlob { size: alias.len() as u32, data: alias.as_ptr() };

        let _identity = IdentityScope::build()?;
        let ret = unsafe { DeleteKey(&key_alias as *const HksBlob) };
        match ret {
            HKS_SUCCESS | HKS_ERROR_NOT_EXIST => Ok(()),
            _ => {
                log_throw_error!(ErrCode::CryptoError, "[FATAL]secret key delete failed ret {}", ret)
            },
        }
    }

    /// Determine whether user auth is required.
    pub(crate) fn need_user_auth(&self) -> bool {
        self.auth_type == AuthType::Any
    }

    /// Determine whether device unlock is required.
    pub(crate) fn need_device_unlock(&self) -> bool {
        self.access_type == Accessibility::DeviceUnlocked
    }

    /// Get the key alias.
    pub(crate) fn alias(&self) -> &Vec<u8> {
        &self.alias
    }

    /// Get the key user id
    pub(crate) fn user_id(&self) -> i32 {
        self.user_id
    }

    /// Get the key access type
    pub(crate) fn access_type(&self) -> Accessibility {
        self.access_type
    }
}
