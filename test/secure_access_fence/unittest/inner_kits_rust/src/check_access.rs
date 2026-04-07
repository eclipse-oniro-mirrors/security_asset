/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

use crate::common::*;
use crate::TEST_CASE_MUTEX;
use saf_sdk::*;

#[test]
fn add_all_tags() {
    let _lock = TEST_CASE_MUTEX.lock().unwrap();
    let mut attrs = SAFMap::new();
    attrs.insert_attr(Tag::CompationDeviceId, );
    attrs.insert_attr(Tag::AuthTrustLevel, func_name.to_owned());
    attrs.insert_attr(Tag::DeviceId, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::AccessBundleName, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::CallerBundleName, Accessibility::DevicePowerOn);

    let res = saf_sdk::Manager::build().unwrap().lock().unwrap().check_access(&attrs);
    assert_eq!(true, res.is_ok());
}
