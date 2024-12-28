/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

use crate::{process_info::ProcessInfoDetail, OwnerType, ProcessInfo, GROUP_SEPARATOR};
use asset_definition::Value;

/// The identity of calling process.
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct Group {
    /// The developer id.
    pub developer_id: Vec<u8>,
    /// The group id.
    pub group_id: Vec<u8>,
}

/// The identity of calling process.
#[derive(Clone)]
#[derive(PartialEq, Eq)]
pub struct CallingInfo {
    user_id: i32,
    owner_type: OwnerType,
    owner_info: Vec<u8>,
    group: Option<Group>,
}

impl CallingInfo {
    /// Build identity of current process.
    pub fn new_self() -> Self {
        Self::new(0, OwnerType::Native, "asset_service_8100".as_bytes().to_vec(), None)
    }

    /// Build identity of part process info.
    pub fn new_part_info(user_id: i32) -> Self {
        Self::new(user_id, OwnerType::Native, "asset_service_8100".as_bytes().to_vec(), None)
    }

    /// Build identity of the specified owner.
    pub fn new(user_id: i32, owner_type: OwnerType, owner_info: Vec<u8>, group: Option<Group>) -> Self {
        Self { user_id, owner_type, owner_info, group }
    }

    /// Build a instance of CallingInfo.
    pub fn build(specific_user_id: Option<Value>, process_info: &ProcessInfo) -> Self {
        let mut user_id = process_info.user_id;
        if let Some(Value::Number(specific_user_id)) = specific_user_id {
            user_id = specific_user_id;
        };

        let mut owner_info = Vec::new();
        match &process_info.process_info_detail {
            ProcessInfoDetail::Hap(hap_info) => {
                owner_info.append(&mut hap_info.app_id.clone());
                owner_info.append(&mut "_".to_string().as_bytes().to_vec());
                owner_info.append(&mut hap_info.app_index.to_string().as_bytes().to_vec());
                let group = match (&hap_info.developer_id, &hap_info.group_id) {
                    (Some(developer_id), Some(group_id)) => {
                        Some(Group { developer_id: developer_id.to_vec(), group_id: group_id.to_vec() })
                    },
                    _ => None,
                };
                CallingInfo { user_id: user_id as i32, owner_type: process_info.owner_type, owner_info, group }
            },
            ProcessInfoDetail::Native(native_info) => {
                owner_info.append(&mut process_info.process_name.clone());
                owner_info.append(&mut "_".to_string().as_bytes().to_vec());
                owner_info.append(&mut native_info.uid.to_string().as_bytes().to_vec());
                CallingInfo { user_id: user_id as i32, owner_type: process_info.owner_type, owner_info, group: None }
            },
        }
    }

    /// Get owner type of calling.
    pub fn owner_type(&self) -> u32 {
        self.owner_type as u32
    }

    /// Get owner type enum of calling.
    pub fn owner_type_enum(&self) -> OwnerType {
        self.owner_type
    }

    /// Get owner info of calling.
    pub fn owner_info(&self) -> &Vec<u8> {
        &self.owner_info
    }

    /// Get user id of calling.
    pub fn user_id(&self) -> i32 {
        self.user_id
    }

    /// Get developer id of calling.
    pub fn developer_id(&self) -> Option<&Vec<u8>> {
        match &self.group {
            Some(group) => Some(&group.developer_id),
            _ => None,
        }
    }

    /// Get group id of calling.
    pub fn group_id(&self) -> Option<&Vec<u8>> {
        match &self.group {
            Some(group) => Some(&group.group_id),
            _ => None,
        }
    }

    /// Get group (developer id + group id) of calling.
    pub fn group(&self) -> Option<Vec<u8>> {
        match &self.group {
            Some(group) => {
                let mut group_vec: Vec<u8> = Vec::new();
                group_vec.extend(group.developer_id.clone());
                group_vec.push(GROUP_SEPARATOR as u8);
                group_vec.extend(group.group_id.clone());
                Some(group_vec)
            },
            _ => None,
        }
    }

    /// Get appindex.
    pub fn app_index(&self) -> u32 {
        match self.owner_type_enum() {
            OwnerType::Hap | OwnerType::Group => {
                let owner_info_str = String::from_utf8_lossy(self.owner_info()).to_string();
                let owner_info_vec: Vec<_> = owner_info_str.split('_').collect();
                match owner_info_vec.last().unwrap().parse::<u32>() {
                    Ok(num) => num,
                    Err(_e) => 0,
                }
            },
            OwnerType::Native => 0,
        }
    }
}

#[cfg(test)]
use crate::process_info::{HapInfo, NativeInfo};

#[test]
fn test_build_callig_info_specific_and_hap() {
    let specific_user_id = 100;
    let process_name = "test_process".as_bytes().to_vec();
    let app_id = "test_app_id".as_bytes().to_vec();
    let app_index = 0;
    let group_id = None;
    let developer_id = None;
    let process_info = ProcessInfo {
        user_id: 0,
        owner_type: OwnerType::Hap,
        process_name,
        process_info_detail: ProcessInfoDetail::Hap(HapInfo { app_id, app_index, group_id, developer_id }),
    };

    let calling_info = CallingInfo::build(Some(Value::Number(specific_user_id)), &process_info);
    assert_eq!(calling_info.user_id(), specific_user_id as i32);

    let owner_info = "test_app_id_0".as_bytes().to_vec();
    assert_eq!(calling_info.owner_info(), &owner_info);
}

#[test]
fn test_build_callig_info_hap() {
    let process_name = "test_process".as_bytes().to_vec();
    let app_id = "test_app_id".as_bytes().to_vec();
    let app_index = 0;
    let user_id = 0;
    let group_id = None;
    let developer_id = None;
    let process_info = ProcessInfo {
        user_id,
        owner_type: OwnerType::Hap,
        process_name,
        process_info_detail: ProcessInfoDetail::Hap(HapInfo { app_id, app_index, group_id, developer_id }),
    };

    let calling_info = CallingInfo::build(None, &process_info);
    assert_eq!(calling_info.user_id(), user_id as i32);
    let owner_info = "test_app_id_0".as_bytes().to_vec();
    assert_eq!(calling_info.owner_info(), &owner_info);
}

#[test]
fn test_build_callig_info_native() {
    let process_name = "test_process".as_bytes().to_vec();
    let user_id = 0;
    let uid = 999;
    let process_info = ProcessInfo {
        user_id,
        owner_type: OwnerType::Native,
        process_name,
        process_info_detail: ProcessInfoDetail::Native(NativeInfo { uid }),
    };

    let calling_info = CallingInfo::build(None, &process_info);
    assert_eq!(calling_info.user_id(), user_id as i32);
    let owner_info = "test_process_999".as_bytes().to_vec();
    assert_eq!(calling_info.owner_info(), &owner_info);
}

#[test]
fn test_build_callig_info_specific_and_native() {
    let specific_user_id = 100;
    let process_name = "test_process".as_bytes().to_vec();
    let user_id = 0;
    let uid = 999;
    let process_info = ProcessInfo {
        user_id,
        owner_type: OwnerType::Native,
        process_name,
        process_info_detail: ProcessInfoDetail::Native(NativeInfo { uid }),
    };

    let calling_info = CallingInfo::build(Some(Value::Number(specific_user_id)), &process_info);

    assert_eq!(calling_info.user_id(), specific_user_id as i32);
    let owner_info = "test_process_999".as_bytes().to_vec();
    assert_eq!(calling_info.owner_info(), &owner_info);
}
