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
use asset_log::{loge, logi};
use asset_utils::hasher;

use crate::{HksBlob, KeyId};

/// Struct to store key attributes, excluding key materials.
#[derive(Clone)]
pub struct SecretKey {
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
    alias: Vec<u8>,
    user_id: i32,
}

enum KeyAliasVersion {
    V1(Vec<u8>), // Old secret key alias
    V2(Vec<u8>), // New secret key alias
    V3,          // Prefixed new secret key alias
    None,
}

extern "C" {
    fn GenerateKey(keyId: *const KeyId, need_auth: bool, require_password_set: bool) -> i32;
    fn DeleteKey(keyId: *const KeyId) -> i32;
    fn IsKeyExist(keyId: *const KeyId) -> i32;
    fn RenameKeyAlias(keyId: *const KeyId, newKeyAlias: *const HksBlob) -> i32;
}

const MAX_ALIAS_SIZE: usize = 64;
const ALIAS_PREFIX: [u8; 2] = [b'1', b'_'];

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
    hasher::sha256(standard, &alias)
}

fn get_existing_key_alias(
    calling_info: &CallingInfo,
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
) -> Result<KeyAliasVersion> {
    let new_alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set, true);
    let prefixed_new_alias = [ALIAS_PREFIX.to_vec(), new_alias.clone()].concat();
    let key = SecretKey {
        user_id: calling_info.user_id(),
        auth_type,
        access_type,
        require_password_set,
        alias: prefixed_new_alias.clone(),
    };
    if key.exists()? {
        logi!("[INFO][{access_type}]-typed secret key with version 3 alias exists.");
        return Ok(KeyAliasVersion::V3);
    }

    let key = SecretKey {
        user_id: calling_info.user_id(),
        auth_type,
        access_type,
        require_password_set,
        alias: new_alias.clone(),
    };
    if key.exists()? {
        logi!("[INFO][{access_type}]-typed secret key with version 2 alias exists.");
        return Ok(KeyAliasVersion::V2(new_alias));
    }

    let old_alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set, false);
    let key = SecretKey {
        user_id: calling_info.user_id(),
        auth_type,
        access_type,
        require_password_set,
        alias: old_alias.clone(),
    };
    if key.exists()? {
        logi!("[INFO][{access_type}]-typed secret key with version 1 alias exists.");
        return Ok(KeyAliasVersion::V1(old_alias));
    }

    loge!("[INFO][{access_type}]-typed secret key does not exist.");
    Ok(KeyAliasVersion::None)
}

fn huks_rename_key_alias(
    calling_info: &CallingInfo,
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
    alias: Vec<u8>,
) -> i32 {
    // Prepare secret key id with outdated alias.
    let alias_ref = &alias;
    let alias_blob = HksBlob { size: alias.len() as u32, data: alias_ref.as_ptr() };
    let key_id = KeyId::new(calling_info.user_id(), alias_blob, access_type);

    // Prepare secret key alias to be replaced in.
    let new_alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set, true);
    let prefixed_new_alias = [ALIAS_PREFIX.to_vec(), new_alias].concat();
    let prefixed_new_alias_ref = &prefixed_new_alias;
    let prefixed_new_alias_blob =
        HksBlob { size: prefixed_new_alias.len() as u32, data: prefixed_new_alias_ref.as_ptr() };

    unsafe { RenameKeyAlias(&key_id as *const KeyId, &prefixed_new_alias_blob as *const HksBlob) }
}

/// Rename a secret key alias.
pub fn rename_key_alias(
    calling_info: &CallingInfo,
    auth_type: AuthType,
    access_type: Accessibility,
    require_password_set: bool,
) -> Result<bool> {
    match get_existing_key_alias(calling_info, auth_type, access_type, require_password_set)? {
        KeyAliasVersion::V3 => {
            logi!("[INFO]Alias of [{access_type}]-typed secret key has already been renamed successfully.");
            Ok(true)
        },
        KeyAliasVersion::V2(alias) | KeyAliasVersion::V1(alias) => {
            let ret = huks_rename_key_alias(calling_info, auth_type, access_type, require_password_set, alias);
            if let SUCCESS = ret {
                logi!("[INFO]Rename alias of [{access_type}]-typed secret key success.");
                Ok(true)
            } else {
                loge!(
                    "[FATAL]Rename alias of [{access_type}]-typed secret key failed, err is {}.",
                    transfer_error_code(ErrCode::try_from(ret as u32)?)
                );
                Ok(false)
            }
        },
        KeyAliasVersion::None => {
            loge!("[FATAL][{access_type}]-typed secret key does not exist.");
            Ok(false)
        },
    }
}

impl SecretKey {
    /// New a secret key with the input key alias argument.
    pub fn new_with_alias(
        user_id: i32,
        auth_type: AuthType,
        access_type: Accessibility,
        require_password_set: bool,
        alias: Vec<u8>,
    ) -> Result<Self> {
        Ok(Self { user_id, auth_type, access_type, require_password_set, alias })
    }

    /// Calculate key alias and then new a secret key.
    pub fn new_without_alias(
        calling_info: &CallingInfo,
        auth_type: AuthType,
        access_type: Accessibility,
        require_password_set: bool,
    ) -> Result<Self> {
        let new_alias = calculate_key_alias(calling_info, auth_type, access_type, require_password_set, true);
        let prefixed_new_alias = [ALIAS_PREFIX.to_vec(), new_alias.clone()].concat();
        let key = Self {
            user_id: calling_info.user_id(),
            auth_type,
            access_type,
            require_password_set,
            alias: prefixed_new_alias,
        };
        logi!("[INFO]Use secret key with prefixed new alias.");
        Ok(key)
    }

    /// Check whether the secret key exists.
    pub fn exists(&self) -> Result<bool> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };
        let key_id = KeyId::new(self.user_id, key_alias, self.access_type);
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
        let key_id = KeyId::new(self.user_id, key_alias, self.access_type);
        let ret = unsafe { GenerateKey(&key_id as *const KeyId, self.need_user_auth(), self.require_password_set) };
        match ret {
            SUCCESS => Ok(()),
            _ => Err(transfer_error_code(ErrCode::try_from(ret as u32)?)),
        }
    }

    /// Delete the secret key.
    pub fn delete(&self) -> Result<()> {
        let key_alias = HksBlob { size: self.alias.len() as u32, data: self.alias.as_ptr() };
        let key_id = KeyId::new(self.user_id, key_alias, self.access_type);
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
            let secret_key = SecretKey::new_without_alias(calling_info, AuthType::None, accessibility, true)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };

            let secret_key = SecretKey::new_without_alias(calling_info, AuthType::Any, accessibility, true)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };

            let secret_key = SecretKey::new_without_alias(calling_info, AuthType::None, accessibility, false)?;
            let tmp = secret_key.delete();
            res = if tmp.is_err() { tmp } else { res };

            let secret_key = SecretKey::new_without_alias(calling_info, AuthType::Any, accessibility, false)?;
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

    /// Get the key user id.
    pub(crate) fn user_id(&self) -> i32 {
        self.user_id
    }
}
