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

use crypto_manager::crypto::{Crypto, KeyInfo, SecretKey};

use asset_common::{definition::{AssetMap, Result, Tag, Value, ErrCode}, loge};
use crate::calling_info::CallingInfo;
use crate::operations::operation_common::hasher;

fn construct_aad(info: &CallingInfo, auth_type: &u32, access_type: &u32) -> Vec<u8> {
    format!("{}_{}_{}", info.user_id(), *auth_type, *access_type).into_bytes()
}

// todo : zwz : 传入map
fn construct_key_info(calling_info: &CallingInfo, auth_type: &u32, access_type: &u32) -> Result<KeyInfo> {
    Ok(KeyInfo {
        user_id: calling_info.user_id(),
        owner_hash: hasher::sha256(calling_info.owner_text()?.as_bytes()).to_vec(),
        auth_type: *auth_type,
        access_type: *access_type,
    })
}

pub(crate) fn encrypt(calling_info: &CallingInfo, input: &AssetMap, secret: &Vec<u8>)
    -> Result<Vec<u8>> {
    let auth_type = match input.get(&Tag::AuthType) {
        Some(Value::Number(res)) => res,
        _ => panic!("get number from auth_type failed."),
    };
    let access_type = match input.get(&Tag::Accessibility) {
        Some(Value::Number(res)) => res,
        _ => todo!(),
    };
    let key_info = construct_key_info(calling_info, auth_type, access_type)?;
    let secret_key = SecretKey::new(key_info);
    match secret_key.exists() { // todo 使用Ok（bool）类型判断
        Ok(true) => (),
        Ok(false) => {
            match secret_key.generate() {
                Ok(_) => (),
                Err(res) => loge!("Generete key failed, res is [{}].", res),
            };
        },
        _ => {
            loge!("Check key exist failed.");
            return Err(ErrCode::CryptoError);
        }
    };

    let crypto = Crypto { key: secret_key };

    crypto.encrypt(secret, &construct_aad(calling_info, auth_type, access_type))
}

pub(crate) fn decrypt(calling_info: &CallingInfo, auth_type: &u32, access_type: &u32,
    cipher: &Vec<u8>) -> Result<Vec<u8>> {

    let key_info = construct_key_info(calling_info, auth_type, access_type)?;
    let secret_key = SecretKey::new(key_info);
    match secret_key.exists() { // todo 使用Ok（bool）类型判断
        Ok(true) => (),
        _ => {
            loge!("Found key failed.");
            return Err(ErrCode::NotFound);
        },
    };

    let crypto = Crypto { key: secret_key };

    crypto.decrypt(cipher, &construct_aad(calling_info, auth_type, access_type))
}

pub(crate) fn init_decrypt(calling_info: &CallingInfo, auth_type: &u32, access_type: &u32)
    -> Result<Vec<u8>> {
    let key_info = construct_key_info(calling_info, auth_type, access_type)?;
    let _secret_key = SecretKey::new(key_info);
    // todo 这里需要等init_decrypt的接口搞定之后再写 先写个假的放上去
    Ok(vec![1, 2, 2, 2, 2, 1])
}