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

use asset_sdk::{AssetMap, AuthType, ConflictResolution, ErrCode, Insert, Tag, Value, Accessibility, SyncType};

use crate::common::*;

#[test]
fn add_all_tags() {
    let alias = "add_all_tags_alias".as_bytes();
    let secret = "add_all_tags_secret".as_bytes();
    let normal_label1 = "add_all_tags_normal_label1".as_bytes();
    let normal_label2 = "add_all_tags_normal_label2".as_bytes();
    let normal_label3 = "add_all_tags_normal_label3".as_bytes();
    let normal_label4 = "add_all_tags_normal_label4".as_bytes();
    let critical_label1 = "add_all_tags_critical_label1".as_bytes();
    let critical_label2 = "add_all_tags_critical_label2".as_bytes();
    let critical_label3 = "add_all_tags_critical_label3".as_bytes();
    let critical_label4 = "add_all_tags_critical_label4".as_bytes();

    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.to_vec()).unwrap();
    attrs.insert_attr(Tag::Secret, secret.to_vec()).unwrap();
    attrs.insert_attr(Tag::DataLabelNormal1, normal_label1.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelNormal2, normal_label2.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelNormal3, normal_label3.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelNormal4, normal_label4.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelCritical1, critical_label1.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelCritical2, critical_label2.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelCritical3, critical_label3.to_owned()).unwrap();
    attrs.insert_attr(Tag::DataLabelCritical4, critical_label4.to_owned()).unwrap();
    attrs.insert_attr(Tag::Accessibility, Accessibility::DeviceUnlock).unwrap();
    attrs.insert_attr(Tag::AuthType, AuthType::Any).unwrap();
    attrs.insert_attr(Tag::SyncType, SyncType::ThisDevice).unwrap();
    attrs.insert_attr(Tag::RequirePasswordSet, true).unwrap();
    attrs.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite).unwrap();
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(13, res[0].len());
    assert_eq!(alias, *get_bytes(&res[0], Tag::Alias).unwrap());
    assert_eq!(normal_label1, *get_bytes(&res[0], Tag::DataLabelNormal1).unwrap());
    assert_eq!(normal_label2, *get_bytes(&res[0], Tag::DataLabelNormal2).unwrap());
    assert_eq!(normal_label3, *get_bytes(&res[0], Tag::DataLabelNormal3).unwrap());
    assert_eq!(normal_label4, *get_bytes(&res[0], Tag::DataLabelNormal4).unwrap());
    assert_eq!(critical_label1, *get_bytes(&res[0], Tag::DataLabelCritical1).unwrap());
    assert_eq!(critical_label2, *get_bytes(&res[0], Tag::DataLabelCritical2).unwrap());
    assert_eq!(critical_label3, *get_bytes(&res[0], Tag::DataLabelCritical3).unwrap());
    assert_eq!(critical_label4, *get_bytes(&res[0], Tag::DataLabelCritical4).unwrap());
    assert_eq!(Accessibility::DeviceUnlock, get_enum_variant::<Accessibility>(&res[0], Tag::Accessibility).unwrap());
    assert_eq!(AuthType::Any, get_enum_variant::<AuthType>(&res[0], Tag::AuthType).unwrap());
    assert_eq!(SyncType::ThisDevice, get_enum_variant::<SyncType>(&res[0], Tag::SyncType).unwrap());
    assert!(get_bool(&res[0], Tag::RequirePasswordSet).unwrap());

    remove_by_alias(alias).unwrap();
}

#[test]
fn add_required_tags() {
    let func_name = std::any::type_name::<()>().as_bytes();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, func_name.to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, func_name.to_owned()).unwrap();
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_all_by_alias(func_name).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(6, res[0].len());
    assert_eq!(func_name, *get_bytes(&res[0], Tag::Alias).unwrap());
    assert_eq!(func_name, *get_bytes(&res[0], Tag::Secret).unwrap());
    assert_eq!(Accessibility::DeviceFirstUnlock, get_enum_variant::<Accessibility>(&res[0], Tag::Accessibility).unwrap());
    assert_eq!(AuthType::None, get_enum_variant::<AuthType>(&res[0], Tag::AuthType).unwrap());
    assert_eq!(SyncType::Never, get_enum_variant::<SyncType>(&res[0], Tag::SyncType).unwrap());
    assert!(!get_bool(&res[0], Tag::RequirePasswordSet).unwrap());
    remove_by_alias(func_name).unwrap();
}

#[test]
fn add_english_secret() {
    let func_name = std::any::type_name::<()>();
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, func_name.as_bytes().to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, func_name.as_bytes().to_owned()).unwrap();
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_all_by_alias(func_name.as_bytes()).unwrap();
    assert_eq!(1, res.len());
    let bytes = get_bytes(&res[0], Tag::Secret).unwrap();
    assert_eq!(func_name, String::from_utf8(bytes.to_owned()).unwrap());
    remove_by_alias(func_name.as_bytes()).unwrap();
}

#[test]
fn add_chinese_secret() {
    let alias = "Здравствуйте";
    let secret = "中文";
    let mut attrs = AssetMap::new();
    attrs.insert_attr(Tag::Alias, alias.as_bytes().to_owned()).unwrap();
    attrs.insert_attr(Tag::Secret, secret.as_bytes().to_owned()).unwrap();
    asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

    let res = query_all_by_alias(alias.as_bytes()).unwrap();
    assert_eq!(1, res.len());
    let bytes = get_bytes(&res[0], Tag::Secret).unwrap();
    assert_eq!(secret, String::from_utf8(bytes.to_owned()).unwrap());
    let bytes = get_bytes(&res[0], Tag::Alias).unwrap();
    assert_eq!(alias, String::from_utf8(bytes.to_owned()).unwrap());
    remove_by_alias(alias.as_bytes()).unwrap();
}

#[test]
fn add_conflict_throw_error() {
    let alias = std::any::type_name::<()>().as_bytes();
    let secret = std::any::type_name::<()>().as_bytes();

    let mut add_throw_error =
        AssetMap::from([(Tag::Alias, Value::Bytes(alias.to_owned())), (Tag::Secret, Value::Bytes(secret.to_owned()))]);
    asset_sdk::Manager::build().unwrap().add(&add_throw_error).unwrap();

    assert_eq!(Err(ErrCode::Duplicated), asset_sdk::Manager::build().unwrap().add(&add_throw_error));

    add_throw_error.insert_attr(Tag::ConflictResolution, ConflictResolution::ThrowError).unwrap();
    assert_eq!(Err(ErrCode::Duplicated), asset_sdk::Manager::build().unwrap().add(&add_throw_error));

    remove_by_alias(alias).unwrap();
}

#[test]
fn add_conflict_over_write() {
    let alias = std::any::type_name::<()>().as_bytes();
    let secret = std::any::type_name::<()>().as_bytes();
    let label_normal_1 = "add_conflict_over_write_label_normal_1".as_bytes();

    let mut add_over_write = AssetMap::new();
    add_over_write.insert_attr(Tag::Alias, alias.to_owned()).unwrap();
    add_over_write.insert_attr(Tag::Secret, secret.to_owned()).unwrap();

    asset_sdk::Manager::build().unwrap().add(&add_over_write).unwrap();

    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert!(res[0].get(&Tag::DataLabelNormal1).is_none());

    add_over_write.insert_attr(Tag::DataLabelNormal1, label_normal_1.to_owned()).unwrap();
    add_over_write.insert_attr(Tag::ConflictResolution, ConflictResolution::Overwrite).unwrap();

    asset_sdk::Manager::build().unwrap().add(&add_over_write).unwrap();
    let res = query_attr_by_alias(alias).unwrap();
    assert_eq!(1, res.len());
    assert_eq!(label_normal_1, *get_bytes(&res[0], Tag::DataLabelNormal1).unwrap());

    remove_by_alias(alias).unwrap();
}
