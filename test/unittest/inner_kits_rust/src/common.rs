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

#![allow(dead_code)]

use asset_sdk::*;

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

pub(crate) const MIN_NUMBER_VALUE: u32 = 0;
pub(crate) const MAX_RETURN_LIMIT: u32 = 0x10000; // 65536
pub(crate) const MAX_AUTH_VALID_PERIOD: u32 = 600; // 10min
pub(crate) const CRYPTO_CAPACITY: u32 = 16;

pub(crate) const MIN_ARRAY_SIZE: usize = 0;
pub(crate) const MAX_SECRET_SIZE: usize = 1024;

pub(crate) const MAX_ALIAS_SIZE: usize = 256;
pub(crate) const MAX_LABEL_SIZE: usize = 2048;

pub(crate) const AUTH_TOKEN_SIZE: usize = 344;
pub(crate) const CHALLENGE_SIZE: usize = 32;
pub(crate) const SYNC_TYPE_MIN_BITS: u32 = 0;
pub(crate) const SYNC_TYPE_MAX_BITS: u32 = 3;

pub(crate) const CRITICAL_LABEL_ATTRS: [Tag; 4] =
    [Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3, Tag::DataLabelCritical4];

pub(crate) const NORMAL_LABEL_ATTRS: [Tag; 4] =
    [Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4];

pub(crate) fn remove_by_alias(alias: &[u8]) -> Result<()> {
    asset_sdk::Manager::build()?.remove(&AssetMap::from([(Tag::Alias, Value::Bytes(alias.to_vec()))]))
}

pub(crate) const SECRET: &[u8] = "all_tags_secret".as_bytes();
pub(crate) const NORMAL_LABEL1: &[u8] = "all_tags_normal_label1".as_bytes();
pub(crate) const NORMAL_LABEL2: &[u8] = "all_tags_normal_label2".as_bytes();
pub(crate) const NORMAL_LABEL3: &[u8] = "all_tags_normal_label3".as_bytes();
pub(crate) const NORMAL_LABEL4: &[u8] = "all_tags_normal_label4".as_bytes();
pub(crate) const NORMAL_LOCAL_LABEL1: &[u8] = "all_tags_normal_local_label1".as_bytes();
pub(crate) const NORMAL_LOCAL_LABEL2: &[u8] = "all_tags_normal_local_label2".as_bytes();
pub(crate) const NORMAL_LOCAL_LABEL3: &[u8] = "all_tags_normal_local_label3".as_bytes();
pub(crate) const NORMAL_LOCAL_LABEL4: &[u8] = "all_tags_normal_local_label4".as_bytes();
pub(crate) const CRITICAL_LABEL1: &[u8] = "all_tags_critical_label1".as_bytes();
pub(crate) const CRITICAL_LABEL2: &[u8] = "all_tags_critical_label2".as_bytes();
pub(crate) const CRITICAL_LABEL3: &[u8] = "all_tags_critical_label3".as_bytes();
pub(crate) const CRITICAL_LABEL4: &[u8] = "all_tags_critical_label4".as_bytes();

pub(crate) fn remove_all() -> Result<()> {
    asset_sdk::Manager::build()?.remove(&AssetMap::new())
}

pub(crate) fn query_all_by_alias(alias: &[u8]) -> Result<Vec<AssetMap>> {
    asset_sdk::Manager::build()?.query(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::ReturnType, Value::Number(ReturnType::All as u32)),
    ]))
}

pub(crate) fn query_attr_by_alias(alias: &[u8]) -> Result<Vec<AssetMap>> {
    asset_sdk::Manager::build()?.query(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::ReturnType, Value::Number(ReturnType::Attributes as u32)),
    ]))
}

pub(crate) fn add_default_asset(alias: &[u8], secret: &[u8]) -> Result<()> {
    asset_sdk::Manager::build()?.add(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::Secret, Value::Bytes(secret.to_vec())),
        (Tag::Accessibility, Value::Number(Accessibility::DevicePowerOn as u32)),
    ]))
}

pub(crate) fn add_default_auth_asset(alias: &[u8], secret: &[u8]) -> Result<()> {
    asset_sdk::Manager::build()?.add(&AssetMap::from([
        (Tag::Alias, Value::Bytes(alias.to_vec())),
        (Tag::Secret, Value::Bytes(secret.to_vec())),
        (Tag::Accessibility, Value::Number(Accessibility::DevicePowerOn as u32)),
        (Tag::AuthType, Value::Number(AuthType::Any as u32)),
    ]))
}

pub(crate) fn add_all_tags_asset(alias: &[u8]) -> Result<()> {
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.to_vec());
    attrs.insert_attr(Tag::Secret, SECRET.to_vec());
    attrs.insert_attr(Tag::DataLabelNormal1, NORMAL_LABEL1.to_owned());
    attrs.insert_attr(Tag::DataLabelNormal2, NORMAL_LABEL2.to_owned());
    attrs.insert_attr(Tag::DataLabelNormal3, NORMAL_LABEL3.to_owned());
    attrs.insert_attr(Tag::DataLabelNormal4, NORMAL_LABEL4.to_owned());
    attrs.insert_attr(Tag::DataLabelNormalLocal1, NORMAL_LOCAL_LABEL1.to_owned());
    attrs.insert_attr(Tag::DataLabelNormalLocal2, NORMAL_LOCAL_LABEL2.to_owned());
    attrs.insert_attr(Tag::DataLabelNormalLocal3, NORMAL_LOCAL_LABEL3.to_owned());
    attrs.insert_attr(Tag::DataLabelNormalLocal4, NORMAL_LOCAL_LABEL4.to_owned());
    attrs.insert_attr(Tag::DataLabelCritical1, CRITICAL_LABEL1.to_owned());
    attrs.insert_attr(Tag::DataLabelCritical2, CRITICAL_LABEL2.to_owned());
    attrs.insert_attr(Tag::DataLabelCritical3, CRITICAL_LABEL3.to_owned());
    attrs.insert_attr(Tag::DataLabelCritical4, CRITICAL_LABEL4.to_owned());
    attrs.insert_attr(Tag::Accessibility, Accessibility::DevicePowerOn);
    attrs.insert_attr(Tag::AuthType, AuthType::Any);
    attrs.insert_attr(Tag::SyncType, SyncType::ThisDevice);
    attrs.insert_attr(Tag::RequirePasswordSet, false);
    attrs.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite);
    asset_sdk::Manager::build().unwrap().add(&attrs)
}

pub(crate) fn expect_error_eq(expect_err: ErrCode, real_err: AssetError) {
    assert_eq!(expect_err, real_err.code)
}
