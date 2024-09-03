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

use std::ffi::{c_char, CString};
use std::ptr;

use asset_common::{CallingInfo, OwnerType};
use asset_crypto_manager::{crypto::*, crypto_manager::*, secret_key::*};
use asset_definition::{Accessibility, AuthType, ErrCode};

pub const AAD_SIZE: u32 = 8;

#[repr(C)]
struct TokenInfoParams {
    dcaps_num: i32,
    perms_num: i32,
    acls_num: i32,
    dcaps: *const *const c_char,
    perms: *const *const c_char,
    acls: *const *const c_char,
    process_name: *const c_char,
    apl_str: *const c_char,
}

extern "C" {
    fn GetAccessTokenId(token_info: *mut TokenInfoParams) -> u64;
    fn SetSelfTokenID(token_id: u64) -> i32;
}

/// Init access token ID for current process
fn grant_self_permission() -> i32 {
    let perms_str = CString::new("ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS").unwrap();
    let name = CString::new("asset_bin_test").unwrap();
    let apl = CString::new("system_basic").unwrap();
    let mut param = TokenInfoParams {
        dcaps_num: 0,
        perms_num: 1,
        acls_num: 0,
        dcaps: ptr::null(),
        perms: &perms_str.as_ptr(),
        acls: ptr::null(),
        process_name: name.as_ptr(),
        apl_str: apl.as_ptr(),
    };

    unsafe {
        let token_id = GetAccessTokenId(&mut param as *mut TokenInfoParams);
        SetSelfTokenID(token_id)
    }
}

#[test]
fn generate_and_delete() {
    assert_eq!(0, grant_self_permission());
    let calling_info = CallingInfo::new(0, OwnerType::Native, vec![b'2']);
    let secret_key = SecretKey::new(&calling_info, AuthType::None, Accessibility::DevicePowerOn, false, None).unwrap();
    secret_key.generate().unwrap();
    secret_key.exists().unwrap();
    let _ = SecretKey::delete_by_owner(&calling_info);
    assert!(secret_key.delete().is_ok())
}

#[test]
fn encrypt_and_decrypt() {
    assert_eq!(0, grant_self_permission());
    // generate key
    let calling_info = CallingInfo::new(0, OwnerType::Native, vec![b'2']);
    let secret_key = SecretKey::new(&calling_info, AuthType::None, Accessibility::DevicePowerOn, false, None).unwrap();
    secret_key.generate().unwrap();

    // encrypt data
    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let cipher = Crypto::encrypt(&secret_key, &msg, &aad).unwrap();
    assert!(!cipher.eq(&msg));

    // decrypt data
    let plaintext = Crypto::decrypt(&secret_key, &cipher, &aad).unwrap();
    assert!(plaintext.eq(&msg));

    // delete key
    let _ = secret_key.delete();
}

#[test]
fn crypto_init() {
    assert_eq!(0, grant_self_permission());
    let calling_info = CallingInfo::new(0, OwnerType::Native, vec![b'2']);
    let secret_key = SecretKey::new(&calling_info, AuthType::Any, Accessibility::DevicePowerOn, false, None).unwrap();
    secret_key.generate().unwrap();

    let mut crypto = Crypto::build(secret_key.clone(), 600).unwrap();
    crypto.init_key().unwrap();
    let _ = secret_key.delete();
}

#[test]
fn crypto_exec() {
    assert_eq!(0, grant_self_permission());
    let calling_info = CallingInfo::new(0, OwnerType::Native, vec![b'2']);
    let secret_key = SecretKey::new(&calling_info, AuthType::Any, Accessibility::DevicePowerOn, false, None).unwrap();
    secret_key.generate().unwrap();

    let msg = vec![1, 2, 3, 4, 5, 6];
    let aad = vec![0; AAD_SIZE as usize];
    let cipher = Crypto::encrypt(&secret_key, &msg, &aad).unwrap();
    let mut crypto = Crypto::build(secret_key.clone(), 600).unwrap();
    crypto.init_key().unwrap();

    let authtoken = vec![0; 280];
    assert!(crypto.exec_crypt(&cipher, &aad, &authtoken).is_err());
    let _ = secret_key.delete();
}

#[test]
fn crypto_manager() {
    assert_eq!(0, grant_self_permission());
    let calling_info = CallingInfo::new(0, OwnerType::Native, vec![b'2']);
    let secret_key1 = SecretKey::new(&calling_info, AuthType::Any, Accessibility::DevicePowerOn, false, None).unwrap();
    secret_key1.generate().unwrap();
    let mut crypto1 = Crypto::build(secret_key1.clone(), 600).unwrap();
    let challenge1 = crypto1.init_key().unwrap().clone();

    let secret_key2 = SecretKey::new(&calling_info, AuthType::Any, Accessibility::DevicePowerOn, false, None).unwrap();
    secret_key2.generate().unwrap();
    let mut crypto2 = Crypto::build(secret_key2.clone(), 600).unwrap();
    let challenge2 = crypto2.init_key().unwrap().clone();

    let arc_crypto_manager = CryptoManager::get_instance();
    let mut crypto_manager = arc_crypto_manager.lock().unwrap();
    crypto_manager.add(crypto1).unwrap();
    crypto_manager.add(crypto2).unwrap();

    let calling_info_2 = CallingInfo::new(0, OwnerType::Native, vec![b'3']);
    crypto_manager.find(&calling_info, &challenge1).unwrap();
    crypto_manager.find(&calling_info, &challenge2).unwrap();
    assert_eq!(ErrCode::NotFound, crypto_manager.find(&calling_info_2, &challenge2).err().unwrap().code);

    crypto_manager.remove(&calling_info, &challenge1);
    crypto_manager.remove(&calling_info_2, &challenge2);
    crypto_manager.find(&calling_info, &challenge2).unwrap();
    crypto_manager.remove(&calling_info, &challenge2);
    assert_eq!(ErrCode::NotFound, crypto_manager.find(&calling_info, &challenge2).err().unwrap().code);

    crypto_manager.remove_need_device_unlocked();

    let _ = secret_key1.delete();
    let _ = secret_key2.delete();
}
