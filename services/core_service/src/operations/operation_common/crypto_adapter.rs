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

use crypto_manager::crypto::{Crypto, SecretKey};

use asset_common::{definition::{Accessibility, AssetMap, AuthType, ErrCode, Result, Value}, hasher, loge, logi};
use db_operator::{database_table_helper::{COLUMN_AUTH_TYPE, COLUMN_ACCESSIBILITY, COLUMN_SECRET}, types::DbMap};
use crate::calling_info::CallingInfo;

// todo : zwz : 实现真的aad
fn construct_aad() -> Vec<u8> {
    "1_2_3_4".as_bytes().to_vec()
}

// todo : zwz : 切面编程
// logi!("reset calling indentity [{}]", ipc_rust::reset_calling_identity().unwrap());

fn build_secret_key(calling: &CallingInfo, attrs: &DbMap) -> Result<SecretKey> {
    let Value::Number(auth_type) = attrs[COLUMN_AUTH_TYPE] else { return Err(ErrCode::InvalidArgument) };
    let auth_type = AuthType::try_from(auth_type)?;

    let Value::Number(access_type) = attrs[COLUMN_ACCESSIBILITY] else { return Err(ErrCode::InvalidArgument) };
    let access_type = Accessibility::try_from(access_type)?;

    Ok(SecretKey::new(calling.user_id(), &hasher::sha256(calling.owner_info()), auth_type, access_type))
}

pub(crate) fn encrypt(calling_info: &CallingInfo, db_data: &DbMap) -> Result<Vec<u8>> {
    let secret_key = build_secret_key(calling_info, db_data)?;
    match secret_key.exists() {
        Ok(true) => (),
        Ok(false) => {
            logi!("[INFO]The key does not exist, generate it.");
            match secret_key.generate() {
                Ok(_) => (),
                Err(res) => loge!("Generete key failed, res is [{}].", res),
            };
        },
        _ => {
            loge!("[FATAL]HUKS failed to determine whether the key exists.");
            return Err(ErrCode::CryptoError);
        }
    };

    let crypto = Crypto { key: secret_key };
    let Value::Bytes(ref secret) = db_data[COLUMN_SECRET] else { return Err(ErrCode::InvalidArgument) };
    crypto.encrypt(secret, &construct_aad())
}

pub(crate) fn decrypt(calling_info: &CallingInfo, db_data: &mut DbMap) -> Result<()> {
    let Value::Bytes(ref secret) = db_data[COLUMN_SECRET] else { return Err(ErrCode::InvalidArgument) };
    let secret_key = build_secret_key(calling_info, db_data)?;
    let crypto = Crypto { key: secret_key };
    let secret = crypto.decrypt(secret, &construct_aad())?; // todo: 待处理HUKS返回值，比如密钥不存在，锁屏状态不正确
    db_data.insert(COLUMN_SECRET, Value::Bytes(secret));
    Ok(())
}

// todo : yyd : 改入参
pub(crate) fn init_decrypt(_calling_info: &CallingInfo, _input: &AssetMap, _auth_type: &u32, _access_type: &u32)
    -> Result<Vec<u8>> {
    // todo 这里需要等init_decrypt的接口搞定之后再写 先写个假的放上去
    Ok(vec![1, 2, 2, 2, 2, 1])
}