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

//! This module implements the capability of processing the identity information of the Asset caller.

use ipc_rust::get_calling_uid;

use asset_common::{definition::{ErrCode, Result}, impl_enum_trait, logi};

impl_enum_trait!{
    #[repr(C)]
    #[derive(Copy, Clone)]
    pub(crate) enum OwnerType {
        Hap = 0,
        Native = 1,
    }
}

pub(crate) struct CallingInfo {
    owner_type: OwnerType,
    owner_info: Vec<u8>,
    user_id: i32,
}

extern "C" {
    fn GetUserIdByUid(uid: u64, userId: &mut i32) -> bool;
    fn GetOwnerInfo(userId: i32, uid: u64, ownerType: *mut OwnerType,
        ownerInfo: *mut libc::c_char, infoLen: *mut u32) -> bool;
}

pub(crate) fn get_user_id(uid: u64) -> Result<i32> {
    unsafe {
        let mut user_id = 0;
        if GetUserIdByUid(uid, &mut user_id) {
            Ok(user_id)
        } else {
            Err(ErrCode::AccountError)
        }
    }
}

impl CallingInfo {
    pub(crate) fn build() -> Result<Self> {
        let uid = get_calling_uid();
        let user_id: i32 = get_user_id(uid)?;
        let mut owner_info = vec![0u8; 256];
        let mut len = 256u32;
        let mut owner_type = OwnerType::Hap;
        unsafe {
            GetOwnerInfo(user_id, uid, &mut owner_type, owner_info.as_mut_ptr(), &mut len);
        }
        owner_info.truncate(len as usize);

        logi!("reset calling indentity [{}]", ipc_rust::reset_calling_identity().unwrap());  // todo 换个位置

        Ok(CallingInfo { owner_type, owner_info, user_id })
    }

    pub(crate) fn owner_type(&self) -> u32 {
        self.owner_type as u32
    }

    pub(crate) fn owner_info(&self) -> &Vec<u8> {
        &self.owner_info
    }

    pub(crate) fn user_id(&self) -> i32 {
        self.user_id
    }
}