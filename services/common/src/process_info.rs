/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

use std::ptr::{null, null_mut};

use ipc::Skeleton;

use asset_definition::{log_throw_error, ErrCode, Result, Value};

use crate::{get_user_id, ConstAssetBlob, MutAssetBlob, OwnerType, SUCCESS};

#[repr(C)]
struct HapInfoFfi {
    app_index: i32,
    app_id: MutAssetBlob,
    group_id: ConstAssetBlob,
    developer_id: MutAssetBlob,
}

#[repr(C)]
struct NativeInfoFfi {
    uid: u32,
}

#[repr(C)]
struct ProcessInfoFfi {
    user_id: u32,
    owner_type: u32,
    process_name: MutAssetBlob,
    hap_info: HapInfoFfi,
    native_info: NativeInfoFfi,
}

impl ProcessInfoFfi {
    fn init(
        user_id: u32,
        uid: u32,
        process_name: &mut Vec<u8>,
        app_id: &mut Vec<u8>,
        (group_id, developer_id): (&Option<Vec<u8>>, &mut Option<Vec<u8>>),
    ) -> Self {
        let process_name = MutAssetBlob { size: process_name.len() as u32, data: process_name.as_mut_ptr() };
        let app_id = MutAssetBlob { size: app_id.len() as u32, data: app_id.as_mut_ptr() };
        let group_id = match group_id {
            Some(group_id) => ConstAssetBlob { size: group_id.len() as u32, data: group_id.as_ptr() },
            None => ConstAssetBlob { size: 0, data: null() },
        };
        let developer_id = match developer_id {
            Some(developer_id) => MutAssetBlob { size: developer_id.len() as u32, data: developer_id.as_mut_ptr() },
            None => MutAssetBlob { size: 0, data: null_mut() },
        };
        ProcessInfoFfi {
            user_id,
            owner_type: 0,
            process_name,
            hap_info: HapInfoFfi { app_index: 0, app_id, group_id, developer_id },
            native_info: NativeInfoFfi { uid },
        }
    }
}

extern "C" {
    fn GetCallingProcessInfo(userId: u32, uid: u64, ownerInfo: *mut ProcessInfoFfi) -> i32;
}

/// hap-relative information
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct HapInfo {
    /// app id for a hap
    pub app_id: Vec<u8>,

    /// app index
    pub app_index: i32,

    /// group id for a hap
    pub group_id: Option<Vec<u8>>,

    /// developer id for a hap
    pub developer_id: Option<Vec<u8>>,
}

/// native-relative information
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct NativeInfo {
    /// uid
    pub uid: u32,
}

/// process detail information
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub enum ProcessInfoDetail {
    /// hap-relative information
    Hap(HapInfo),

    /// native-relative information
    Native(NativeInfo),
}

/// The identity of calling process.
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct ProcessInfo {
    /// user id of the process
    pub user_id: u32,

    /// the owner type of the process
    pub owner_type: OwnerType,

    /// process name
    pub process_name: Vec<u8>,

    /// process information
    pub process_info_detail: ProcessInfoDetail,
}

impl ProcessInfo {
    /// Build process info.
    pub fn build(group_attr: Option<&Value>) -> Result<Self> {
        let uid = Skeleton::calling_uid();
        let user_id = get_user_id(uid)?;
        let mut process_name = vec![0u8; 256];
        let mut app_id = vec![0u8; 256];
        let (group_id, mut developer_id) = match group_attr {
            Some(Value::Bytes(group_attr)) => (Some(group_attr.clone()), Some(vec![0u8; 20])),
            _ => (None, None),
        };
        let mut process_info_ffi =
            ProcessInfoFfi::init(user_id, uid as u32, &mut process_name, &mut app_id, (&group_id, &mut developer_id));
        match unsafe { GetCallingProcessInfo(user_id, uid, &mut process_info_ffi) } {
            SUCCESS => {
                process_name.truncate(process_info_ffi.process_name.size as usize);
                app_id.truncate(process_info_ffi.hap_info.app_id.size as usize);
                if let Some(developer_id) = &mut developer_id {
                    developer_id.truncate(process_info_ffi.hap_info.developer_id.size as usize);
                }
            },
            error => {
                let error = ErrCode::try_from(error as u32)?;
                return log_throw_error!(error, "[FATAL]Get calling package name failed, res is {}.", error);
            },
        }

        let process_info_detail = match OwnerType::try_from(process_info_ffi.owner_type)? {
            OwnerType::Hap => ProcessInfoDetail::Hap(HapInfo {
                app_id,
                app_index: process_info_ffi.hap_info.app_index,
                group_id,
                developer_id,
            }),
            OwnerType::Native => ProcessInfoDetail::Native(NativeInfo { uid: process_info_ffi.native_info.uid }),
        };

        Ok(Self {
            user_id,
            owner_type: OwnerType::try_from(process_info_ffi.owner_type)?,
            process_name,
            process_info_detail,
        })
    }
}
