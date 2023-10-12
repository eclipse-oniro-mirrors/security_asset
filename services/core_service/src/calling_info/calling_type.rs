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
#![allow(dead_code)]

use asset_common::{
    definition::{ErrCode, Result},
    loge, logi,
};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

/// Owner
pub(crate) enum Owner {
    Hap(Vec<u8>),
    Native(Vec<u8>),
}

impl Owner {
    /// xx
    pub(crate) fn get_type_num(&self) -> u32 {
        match self {
            Self::Hap(_) => 1,
            Self::Native(_) => 2,
        }
    }

    // to do : zwz : 不要String，直接vec<u8>
    /// xx
    pub(crate) fn get_owner_text(&self) -> Result<String> {
        match self {
            Self::Hap(owner_text) => Ok(String::from_utf8(owner_text.clone()).map_err(|e| {
                loge!("get hap owner name failed, get error [{}]", e);
                ErrCode::BmsError
            })?),
            Self::Native(owner_text) => Ok(String::from_utf8(owner_text.clone()).map_err(|e| {
                loge!("get native owner name failed, get error [{}]", e);
                ErrCode::IpcError
            })?),
        }
    }
}

extern {
    fn GetCallingOwnerType(callingTokenId: u32, ownerType: &mut i32) -> bool; // ownerType: 0-> hap; 1->native; 2->shell;
    fn GetCallingTokenId(tokenId: &mut u32) -> bool;
    fn GetCallingProcessName(tokenId: u32) -> *const c_char;
    fn GetHapOwnerInfo(tokenId: u32, userId: i32, addId: *mut *mut c_char, appIndex: *mut i32) -> bool;
    fn FreeMemory(freeStr: *const c_char);
}

fn get_native_owner_info(token_id: u32, uid: u64) -> Result<Owner> {
    unsafe {
        let p_name = GetCallingProcessName(token_id);
        if p_name.is_null() {
            loge!("get calling PName failed!");
            return Err(ErrCode::BmsError);
        }
        let p_name_str = CStr::from_ptr(p_name as _).to_str().unwrap();
        logi!("get calling owner info success! uid:{} pname:{}", uid, p_name_str);
        // FreeMemory(p_name);
        Ok(Owner::Native(Vec::from(format!("{}{}", uid, p_name_str).as_bytes())))
    }
}

fn get_hap_owner_info(token_id: u32, user_id: i32) -> Result<Owner> {
    unsafe {
        let mut app_id: *mut c_char = std::ptr::null_mut();
        let mut app_index: i32 = 0;
        if !GetHapOwnerInfo(token_id, user_id, &mut app_id, &mut app_index) {
            loge!("Get hap owner info failed.");
            return Err(ErrCode::BmsError);
        }
        let app_id_str = CString::from_raw(app_id).into_string().map_err(|e| {
            loge!("get string from add id failed [{}].", e);
            ErrCode::BmsError
        })?;
        // free c memory
        // FreeMemory(app_id);
        Ok(Owner::Hap(format!("{}_{}", app_id_str, app_index).as_bytes().to_vec()))
    }
}

pub(crate) fn get_calling_owner(uid: u64, user_id: i32) -> Result<Owner> { // todo: 将本函数的功能都封装到C++ 中，只对rust开放一个函数
    unsafe {
        let mut token_id = 0;
        // 1 get calling tokenid
        if !GetCallingTokenId(&mut token_id) {
            loge!("get calling token failed!");
            return Err(ErrCode::BmsError);
        }
        let mut owner_type = 0; // store owner type
        // 2 find this calling onwer type 0:hap 1: native 2: shell
        if GetCallingOwnerType(token_id, &mut owner_type) {
            match owner_type {
                0 => Ok(get_hap_owner_info(token_id, user_id)?),
                1 => Ok(get_native_owner_info(token_id, uid)?),
                2 => Ok(get_native_owner_info(token_id, uid)?),
                _ => Err(ErrCode::BmsError)
            }
        } else {
            loge!("get calling owner type failed!");
            Err(ErrCode::BmsError)
        }
    }
}