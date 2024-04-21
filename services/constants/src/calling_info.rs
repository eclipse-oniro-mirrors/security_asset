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

//! This module implements the capability of processing the identity information of the Asset caller.

use ipc::Skeleton;

use asset_definition::{ErrCode, Result, Value};

use crate::{get_user_id, transfer_error_code, SUCCESS};

use super::OwnerType;

/// The identity of calling process.
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct CallingInfo {
    user_id: i32,
    owner_type: OwnerType,
    owner_info: Vec<u8>,
}

#[allow(dead_code)]
#[repr(C)]
enum ResultCode {
    Success = 0,
    InvalidArgument = 1,
    BmsError = 2,
    AccessTokenError = 3,
}

extern "C" {
    fn GetOwnerInfo(userId: i32, uid: u64, ownerType: *mut OwnerType, ownerInfo: *mut u8, infoLen: *mut u32) -> i32;
}

impl CallingInfo {
    /// Build identity of current process.
    pub fn new_self() -> Self {
        Self::new(0, OwnerType::Native, "asset_service_8100".as_bytes().to_vec())
    }

    /// Build identity of the specified owner.
    pub fn new(user_id: i32, owner_type: OwnerType, owner_info: Vec<u8>) -> Self {
        Self { user_id, owner_type, owner_info }
    }

    /// Build a instance of CallingInfo.
    pub fn build(specific_user_id: Option<Value>) -> Result<Self> {
        let uid = Skeleton::calling_uid();
        let mut user_id: i32 = get_user_id(uid)?;
        let mut owner_info = vec![0u8; 256];
        let mut len = 256u32;
        let mut owner_type = OwnerType::Hap;
        let err = unsafe { GetOwnerInfo(user_id, uid, &mut owner_type, owner_info.as_mut_ptr(), &mut len) };
        match err {
            SUCCESS => {
                owner_info.truncate(len as usize);
                if let Some(Value::Number(num)) = specific_user_id {
                    user_id = num as i32;
                }
                Ok(CallingInfo { user_id, owner_type, owner_info })
            },
            _ => Err(transfer_error_code(ErrCode::try_from(err as u32)?)),
        }
    }

    /// Get owner type of calling.
    pub fn owner_type(&self) -> u32 {
        self.owner_type as u32
    }

    /// Get owner info of calling.
    pub fn owner_info(&self) -> &Vec<u8> {
        &self.owner_info
    }

    /// Get user id of calling.
    pub fn user_id(&self) -> i32 {
        self.user_id
    }
}
