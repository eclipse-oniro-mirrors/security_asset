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

#[macro_export]
macro_rules! function {
    () => {{
        fn f() {}
        fn type_name_of<T>(_: T) -> &'static str {
            std::any::type_name::<T>()
        }
        type_name_of(f).rsplit("::").find(|&part| part != "f" && part != "{{closure}}").expect("Short function name")
    }};
}

#[test]
fn test_add() {
    let func_name = function!().as_bytes();
    let calling_info = CallingInfo::new_self();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, func_name.to_owned());
    attrs.insert_attr(Tag::Secret, func_name.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    assert!(add_stub(&calling_info, &attrs).is_ok());
    attrs.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite);
    assert!(add_stub(&calling_info, &attrs).is_ok());
    let mut attrs_to_remove = AssetMap::new();
    attrs_to_remove.insert_attr(Tag::Alias, func_name.to_owned());
    assert!(remove_stub(&calling_info, &attrs_to_remove).is_ok());
}
