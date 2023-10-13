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

use asset_common::{definition::{AssetMap, Result, Tag, Value, ErrCode}, loge, logi, hasher};
use crate::calling_info::CallingInfo;

// todo : zwz : 实现真的aad
fn construct_aad() -> Vec<u8> {
    "1_2_3_4".as_bytes().to_vec()
}

// todo : zwz : 切面编程
// logi!("reset calling indentity [{}]", ipc_rust::reset_calling_identity().unwrap());

// todo : zwz : 传入map
fn construct_key_info(calling_info: &CallingInfo, input: &AssetMap) -> Result<KeyInfo> {
    let Some(Value::Number(auth_type)) = input.get(&Tag::AuthType) else {
        panic!()
    };
    let Some(Value::Number(access_type)) = input.get(&Tag::Accessibility) else {
        panic!()
    };

    logi!("user_id:[{}], owner_hash:[{}], auth_type:[{}],access_type:[{}]", calling_info.user_id(), String::from_utf8(calling_info.owner_text().clone()).unwrap(), *auth_type, *access_type);
    Ok(KeyInfo {
        user_id: calling_info.user_id(),
        owner_hash: hasher::sha256(calling_info.owner_text()).to_vec(),
        auth_type: *auth_type,
        access_type: *access_type,
    })
}

pub(crate) fn encrypt(calling_info: &CallingInfo, input: &AssetMap, secret: &Vec<u8>)
    -> Result<Vec<u8>> {
    let key_info = construct_key_info(calling_info, input)?;
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

    crypto.encrypt(secret, &construct_aad())
}

pub(crate) fn decrypt(calling_info: &CallingInfo, input: &AssetMap) -> Result<Vec<u8>> {
    let Some(Value::Bytes(secret)) = input.get(&Tag::Secret) else {
        loge!("get secret failed!");
        panic!()
    };
    let key_info = construct_key_info(calling_info, input)?;
    let secret_key = SecretKey::new(key_info);
    match secret_key.exists() { // todo 使用Ok（bool）类型判断
        Ok(true) => (),
        _ => {
            loge!("Found key failed.");
            return Err(ErrCode::NotFound);
        },
    };

    let crypto = Crypto { key: secret_key };

    crypto.decrypt(secret, &construct_aad())
}

// todo : yyd : 改入参
pub(crate) fn init_decrypt(calling_info: &CallingInfo, input: &AssetMap, _auth_type: &u32, _access_type: &u32)
    -> Result<Vec<u8>> {
    let key_info = construct_key_info(calling_info, input)?;
    let _secret_key = SecretKey::new(key_info);
    // todo 这里需要等init_decrypt的接口搞定之后再写 先写个假的放上去
    Ok(vec![1, 2, 2, 2, 2, 1])
}