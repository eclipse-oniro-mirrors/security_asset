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

//! This module is used to adapt to the crypto manager.

use asset_common::{
    definition::{Accessibility, AssetMap, AuthType, DataType, ErrCode, Result, Value},
    loge, logi,
};
use asset_crypto_manager::crypto::{Crypto, SecretKey};
use asset_db_operator::{
    database_table_helper::{
        COLUMN_ACCESSIBILITY, COLUMN_ALIAS, COLUMN_AUTH_TYPE, COLUMN_CRITICAL1, COLUMN_CRITICAL2, COLUMN_CRITICAL3,
        COLUMN_CRITICAL4, COLUMN_DELETE_TYPE, COLUMN_GROUP_ID, COLUMN_OWNER, COLUMN_OWNER_TYPE,
        COLUMN_REQUIRE_PASSWORD_SET, COLUMN_SECRET, COLUMN_SYNC_TYPE, COLUMN_VERSION,
    },
    types::DbMap,
};

use asset_hasher::sha256;

use crate::calling_info::CallingInfo;

const AAD_ATTR: [(&str, DataType); 14] = [
    (COLUMN_ALIAS, DataType::Bytes),
    (COLUMN_OWNER, DataType::Bytes),
    (COLUMN_OWNER_TYPE, DataType::Number),
    (COLUMN_GROUP_ID, DataType::Bytes),
    (COLUMN_SYNC_TYPE, DataType::Number),
    (COLUMN_ACCESSIBILITY, DataType::Number),
    (COLUMN_REQUIRE_PASSWORD_SET, DataType::Bool),
    (COLUMN_AUTH_TYPE, DataType::Number),
    (COLUMN_DELETE_TYPE, DataType::Number),
    (COLUMN_VERSION, DataType::Number),
    (COLUMN_CRITICAL1, DataType::Bytes),
    (COLUMN_CRITICAL2, DataType::Bytes),
    (COLUMN_CRITICAL3, DataType::Bytes),
    (COLUMN_CRITICAL4, DataType::Bytes),
];

fn bytes_into_aad(column: &str, attrs: &DbMap, aad: &mut Vec<u8>) {
    if let Some(Value::Bytes(bytes)) = attrs.get(column) {
        aad.extend(bytes);
    }
}

fn u32_into_aad(column: &str, attrs: &DbMap, aad: &mut Vec<u8>) {
    if let Some(Value::Number(num)) = attrs.get(column) {
        aad.extend(num.to_le_bytes());
    }
}

fn bool_into_aad(column: &str, attrs: &DbMap, aad: &mut Vec<u8>) {
    if let Some(Value::Bool(num)) = attrs.get(column) {
        aad.push(*num as u8);
    }
}

fn construct_aad(attrs: &DbMap) -> Vec<u8> {
    let mut aad = Vec::new();
    for (column, data_type) in &AAD_ATTR {
        match *data_type {
            DataType::Number => u32_into_aad(column, attrs, &mut aad),
            DataType::Bytes => bytes_into_aad(column, attrs, &mut aad),
            DataType::Bool => bool_into_aad(column, attrs, &mut aad),
        }
    }
    aad
}

fn build_secret_key(calling: &CallingInfo, attrs: &DbMap) -> Result<SecretKey> {
    let Value::Number(auth_type) = attrs[COLUMN_AUTH_TYPE] else { return Err(ErrCode::InvalidArgument) };
    let auth_type = AuthType::try_from(auth_type)?;

    let Value::Number(access_type) = attrs[COLUMN_ACCESSIBILITY] else { return Err(ErrCode::InvalidArgument) };
    let access_type = Accessibility::try_from(access_type)?;

    Ok(SecretKey::new(calling.user_id(), &sha256(calling.owner_info()), auth_type, access_type))
}

pub(crate) fn encrypt(calling_info: &CallingInfo, db_data: &DbMap) -> Result<Vec<u8>> {
    let identity = ipc_rust::reset_calling_identity().map_err(|e| {
        loge!("Execute reset_calling_identity failed, error is [{}].", e);
        ErrCode::IpcError
    })?;

    let secret_key = build_secret_key(calling_info, db_data)?;
    match secret_key.exists() {
        Ok(true) => (),
        Ok(false) => {
            logi!("[INFO]The key does not exist, generate it.");
            match secret_key.generate() {
                Ok(_) => (),
                Err(res) => {
                    loge!("Generete key failed, res is [{}].", res);
                    return Err(ErrCode::CryptoError);
                },
            };
        },
        _ => {
            loge!("[FATAL]HUKS failed to determine whether the key exists.");
            return Err(ErrCode::CryptoError);
        },
    };

    let Value::Bytes(ref secret) = db_data[COLUMN_SECRET] else { return Err(ErrCode::InvalidArgument) };

    let cipher = Crypto::encrypt(&secret_key ,secret, &construct_aad(db_data))?;

    if !ipc_rust::set_calling_identity(identity) {
        loge!("Execute set_calling_identity failed.");
        return Err(ErrCode::IpcError);
    }

    Ok(cipher)
}

pub(crate) fn decrypt(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    let identity = ipc_rust::reset_calling_identity().map_err(|e| {
        loge!("Execute reset_calling_identity failed, error is [{}].", e);
        ErrCode::IpcError
    })?;

    let Value::Bytes(ref secret) = db_data[COLUMN_SECRET] else { return Err(ErrCode::InvalidArgument) };
    let secret_key = build_secret_key(calling_info, db_data)?;
    let secret = Crypto::decrypt(&secret_key, secret, &construct_aad(db_data))?; // todo: 待处理HUKS返回值，比如密钥不存在，锁屏状态不正确
    db_data.insert(COLUMN_SECRET, Value::Bytes(secret));
    if !ipc_rust::set_calling_identity(identity) {
        loge!("Execute set_calling_identity failed.");
        return Err(ErrCode::IpcError);
    }
    Ok(())
}

// todo : yyd : 改入参
pub(crate) fn init_decrypt(
    _calling_info: &CallingInfo,
    _input: &AssetMap,
    _auth_type: &u32,
    _access_type: &u32,
) -> Result<Vec<u8>> {
    // todo 这里需要等init_decrypt的接口搞定之后再写 先写个假的放上去
    Ok(vec![1, 2, 2, 2, 2, 1])
}
