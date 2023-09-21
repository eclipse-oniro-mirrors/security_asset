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

use asset_common::{definition::{AssetMap, Result, Tag, Value}, logi};
use crate::calling_process_info::CallingInfo;

fn construct_aad(info: &CallingInfo, auth_type: &u32, access_type: &u32) -> Vec<u8> {
    format!("{}_{}_{}_{}", info.user_id(), info.uid(), *auth_type, *access_type).into_bytes()
}

fn construct_key_info(calling_info: &CallingInfo, auth_type: &u32, access_type: &u32) -> KeyInfo {
    KeyInfo {
        user_id: calling_info.user_id(),
        uid: calling_info.uid(),
        auth_type: *auth_type,
        access_type: *access_type,
    }
}

pub(crate) fn encrypt(calling_info: &CallingInfo, input: &AssetMap, secret: &Vec<u8>)
    -> Result<Vec<u8>> {
    let auth_type = match input.get(&Tag::AuthType) {
        Some(Value::Number(res)) => res,
        _ => todo!(),
    };
    let access_type = match input.get(&Tag::Accessibility) {
        Some(Value::Number(res)) => res,
        _ => todo!(),
    };
    let key_info = construct_key_info(calling_info, auth_type, access_type);
    let secret_key = SecretKey::new(key_info);
    if secret_key.exists() != 0 { // todo 使用Ok（bool）类型判断
        let res = secret_key.generate(); // todo
        logi!("generete key res is [{}]", res);
        // return Err() // todo
    }
    let mut crypto = Crypto { key: secret_key };

    crypto.encrypt(secret, &construct_aad(calling_info, auth_type, access_type))
}

pub(crate) fn decrypt(calling_info: &CallingInfo, auth_type: &u32, access_type: &u32,
    cipher: &Vec<u8>) -> Result<Vec<u8>> {
    // todo： authtype 和 accesstype遍历

    let key_info = construct_key_info(calling_info, auth_type, access_type);
    let secret_key = SecretKey::new(key_info);
    if secret_key.exists() != 0 { // todo 使用Ok（bool）类型判断
        let res = secret_key.generate(); // todo
        logi!("generete key res is [{}]", res);
        // return Err() // todo
    }
    let mut crypto = Crypto { key: secret_key };

    crypto.decrypt(cipher, &construct_aad(calling_info, auth_type, access_type))
}