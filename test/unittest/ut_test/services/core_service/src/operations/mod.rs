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
use asset_definition::*;
mod operation_add;
mod operation_remove;

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

pub(crate) fn add_all_tags(alias: &[u8]) -> AssetMap {
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
    attrs
}
