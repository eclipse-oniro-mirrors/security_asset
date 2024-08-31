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

//! This module is used to implement cryptographic algorithm operations, including key generation.

use asset_common::{transfer_error_code, CallingInfo, SUCCESS};
use asset_definition::{Accessibility, AuthType, ErrCode, Result};
use asset_log::logw;
use asset_utils::hasher;

use crate::{HksBlob, KeyId};

/// Struct to store key attributes, excluding key materials.
#[derive(Clone)]
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
    alias: Vec<u8>,
    calling_info: CallingInfo,
}

extern "C" {
    fn GenerateKey(keyId: *const KeyId, need_auth: bool, require_password_set: bool) -> i32;
    fn DeleteKey(keyId: *const KeyId) -> i32;
    fn IsKeyExist(keyId: *const KeyId) -> i32;
}

const MAX_ALIAS_SIZE: usize = 64;

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
    standard: bool,
) -> Result<Vec<u8>> {
    let mut alias: Vec<u8> = Vec::with_capacity(MAX_ALIAS_SIZE);
    alias.extend_from_slice(&calling_info.user_id().to_le_bytes());
    alias.push(b'_');
    alias.extend_from_slice(&calling_info.owner_type().to_le_bytes());
    alias.push(b'_');
    alias.extend(calling_info.owner_info());
    append_attr::<AuthType>("AuthType", auth_type, &mut alias);
    append_attr::<Accessibility>("Accessibility", access_type, &mut alias);
    append_attr::<bool>("RequirePasswordSet", require_password_set, &mut alias);
    hasher::sha256(standard, &alias)
}

impl SecretKey {
    /// New a secret key.
    pub fn new(
        calling_info: &CallingInfo,
        auth_type: AuthType,
        access_type: Accessibility,
        require_password_set: bool,
        alias: Option<Vec<u8>>,
    ) -> Result<Self> {
        if let Some(alias) = alias {
            return Ok(Self { auth_type, access_type, require_password_set, alias, calling_info: calling_info.clone() })
        }

        // Check whether new key exists.
        let alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set, true)?;
        let new_key = Self { auth_type, access_type, require_password_set, alias, calling_info: calling_info.clone() };
        if new_key.exists()? {
            return Ok(new_key);
        }

        // Check whether old key exists.
        logw!("[WARNING]Use old alias key.");
        let alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set, false)?;
        let old_key = Self { auth_type, access_type, require_password_set, alias, calling_info: calling_info.clone() };
        if old_key.exists()? {
            return Ok(old_key);
        }

        // Use new key.
        Ok(new_key)
    }

    /// Check whether the secret key exists.
    pub fn exists(&self) -> Result<bool> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };
        let key_id = KeyId::new(self.calling_info().user_id(), key_alias, self.access_type);
        let ret = unsafe { IsKeyExist(&key_id as *const KeyId) };
        match ret {
            SUCCESS => Ok(true),
            ret if ret == ErrCode::NotFound as i32 => Ok(false),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Generate the secret key and store in HUKS.
    pub fn generate(&self) -> Result<()> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };
        let key_id = KeyId::new(self.calling_info().user_id(), key_alias, self.access_type);
        let ret = unsafe { GenerateKey(&key_id as *const KeyId, self.need_user_auth(), self.require_password_set) };
        match ret {
            SUCCESS => Ok(()),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Delete the secret key.
    pub fn delete(&self) -> Result<()> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };
        let key_id = KeyId::new(self.calling_info().user_id(), key_alias, self.access_type);
        let ret = unsafe { DeleteKey(&key_id as *const KeyId) };
        match ret {
            ret if ((ret == ErrCode::NotFound as i32) || ret == SUCCESS) => Ok(()),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Delete secret key by owner.
    pub fn delete_by_owner(calling_info: &CallingInfo) -> Result<()> {
        let mut res = Ok(());
        let accessibilitys =
            [Accessibility::DevicePowerOn, Accessibility::DeviceFirstUnlocked, Accessibility::DeviceUnlocked];
        for accessibility in accessibilitys.into_iter() {
            let secret_key = SecretKey::new(calling_info, AuthType::None, accessibility, true, None)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };

            let secret_key = SecretKey::new(calling_info, AuthType::Any, accessibility, true, None)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };

            let secret_key = SecretKey::new(calling_info, AuthType::None, accessibility, false, None)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };

            let secret_key = SecretKey::new(calling_info, AuthType::Any, accessibility, false, None)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };
        }

        res
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

    /// Get the key access type
    pub(crate) fn access_type(&self) -> Accessibility {
        self.access_type
    }

    /// Get the key calling info
    pub(crate) fn calling_info(&self) -> &CallingInfo {
        &self.calling_info
    }
}
