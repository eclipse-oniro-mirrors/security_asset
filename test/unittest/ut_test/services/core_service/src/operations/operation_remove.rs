/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! the module test for core_service
use asset_common::*;
use asset_definition::*;
use asset_service::ut_core_service_lib_stub::*;
use crate::function;
use crate::operations::*;

#[test]
fn test_remove() {
    let func_name = function!().as_bytes();
    let calling_info = CallingInfo::new_self();
    let mut attrs = add_all_tags(func_name);
    attrs.insert_attr(Tag::SyncType, SyncType::TrustedAccount);
    assert!(add_stub(&calling_info, &attrs).is_ok());
    attrs.remove(&Tag::ConflictResolution);
    assert_eq!(ErrCode::Duplicated, add_stub(&calling_info, &attrs).unwrap_err().code);
    let mut attrs_to_remove = AssetMap::new();
    attrs_to_remove.insert_attr(Tag::Alias, func_name.to_owned());
    assert!(remove_stub(&calling_info, &attrs_to_remove).is_ok());
    // cover update remove logic
    assert_eq!(ErrCode::NotFound, remove_stub(&calling_info, &attrs_to_remove).unwrap_err().code);
    // delete logic remove data. add no trusted account data first then delete.
    attrs.remove(&Tag::SyncType);
    assert!(add_stub(&calling_info, &attrs).is_ok());
    assert!(remove_stub(&calling_info, &attrs_to_remove).is_ok());
    // cover normal remove logic
    assert_eq!(ErrCode::NotFound, remove_stub(&calling_info, &attrs_to_remove).unwrap_err().code);

    // cover group
    attrs.insert_attr(Tag::GroupId, func_name.to_owned());
    assert_eq!(ErrCode::NotFound, remove_stub(&calling_info, &attrs_to_remove).unwrap_err().code);
}
