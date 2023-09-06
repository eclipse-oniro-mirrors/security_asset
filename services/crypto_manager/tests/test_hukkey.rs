//!
//! Copyright (C) 2023 Huawei Device Co., Ltd.
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//! http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
use crypto_manager::hukkey::*;

#[test]
fn test_hukkey_new(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let secret_key = SecretKey::new(info);
    assert_eq!(secret_key.alias,"1_2_3_4".to_string());
}

#[test]
fn test_hukkey_generate(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let mut secret_key = SecretKey::new(info);
    match secret_key.generate(){
        Ok((_a,_b)) =>{
            println!("test_hukkey_generate pass");
        }
        Err(error) =>{
            panic!("test_hukkey_generate fail error = {}", error);
        }
    }
}

#[test]
fn test_hukkey_delete(){
    let info = KeyInfo {
        user_id: 1,
        uid: 2,
        auth_type: 3,
        access_type: 4,
    };
    let mut secret_key = SecretKey::new(info);
    match secret_key.generate(){
        Ok((a,b)) =>{
            assert_eq!(secret_key.delete(a,b),0);
        }
        Err(error) =>{
            panic!("test_hukkey_generate fail error = {}", error);
        }
    }
}

#[test]
fn test_hukkey_encrypt(){
    let _crypto = Crypto{};
    let msg = vec![1,2,3,4,5,6];
    match Crypto::encrypt(&msg){
        Ok(result) =>{
            assert_eq!(result,msg);
        }
        Err(error) =>{
            panic!("test_hukkey_encrypt fail error = {}", error);
        }
    }
}

#[test]
fn test_hukkey_decrypt(){
    let _crypto = Crypto{};
    let msg = vec![1,2,3,4,5,6];
    match Crypto::decrypt(&msg){
        Ok(result) =>{
            assert_eq!(result,msg);
        }
        Err(error) =>{
            panic!("test_hukkey_encrypt fail error = {}", error);
        }
    }
}


