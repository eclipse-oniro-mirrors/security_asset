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

/// OwnerType
pub(crate) enum OwnerType {
    Hap(Vec<u8>),
    Native(Vec<u8>),
}

impl OwnerType {
    /// xx
    pub(crate) fn get_type_num(&self) -> u32 {
        match self {
            Self::Hap(_) => 1,
            Self::Native(_) => 2,
        }
    }

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
    fn GetCallingToken(tokenId: &mut u32) -> bool;
    fn GetCallingProcessName(tokenId: u32) -> *const c_char;
    fn GetHapOwnerInfo(tokenId: u32, userId: i32) -> *const c_char;
}

fn get_native_owner_info(token_id: u32, uid: u64) -> Result<OwnerType> {
    unsafe {
        let p_name = GetCallingProcessName(token_id);
        if p_name.is_null() {
            loge!("get calling PName failed!");
            return Err(ErrCode::BmsError);
        }
        let p_name_str = CStr::from_ptr(p_name as _).to_str().unwrap();
        logi!("get calling owner info success! uid:{} pname:{}", uid, p_name_str);
        Ok(OwnerType::Native(Vec::from(format!("{}{}", uid, p_name_str).as_bytes())))
    }
}

fn get_hap_owner_info(token_id: u32, user_id: i32) -> Result<OwnerType> {
    unsafe {
        let user_info = GetHapOwnerInfo(token_id, user_id);
        if !user_info.is_null() {
            let user_info_str = CString::from_raw(user_info as *mut c_char).into_string().unwrap();
            logi!("get calling owner info success! user_info:{}", user_info_str);
            Ok(OwnerType::Hap(Vec::from(user_info_str.as_bytes())))
        } else {
            loge!("get calling owner(hap) info failed!");
            Err(ErrCode::BmsError)
        }
    }
}

pub(crate) fn get_calling_owner_type(uid: u64, user_id: i32) -> Result<OwnerType> {
    unsafe {
        let mut token_id = 0;
        // 1 get calling tokenid
        if !GetCallingToken(&mut token_id) {
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