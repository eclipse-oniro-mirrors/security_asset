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
use asset_sdk::*;

#[test]
fn query_alias_min_len() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::Alias, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::Alias, vec![0; 1]);
   assert_eq!(1, asset_sdk::Manager::build().unwrap().query(&query).unwrap().len());
}

#[test]
fn query_alias_max_len() {
   let alias = vec![0; 256];
   let secret = function!().as_bytes();
   add_default_asset(&alias, secret).unwrap();

   let mut query_1 = AssetMap::new();
   query_1.insert_attr(Tag::Alias, vec![0; 257]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query_1).unwrap_err());

   let mut query_2 = AssetMap::new();
   query_2.insert_attr(Tag::Alias, vec![0; 256]);

   assert_eq!(1, asset_sdk::Manager::build().unwrap().query(&query_2).unwrap().len());
   remove_by_alias(&alias).unwrap();
}

#[test]
fn query_alias_with_unmatched_type() {
   let alias = vec![0; 256];
   let secret = function!().as_bytes();
   add_default_asset(&alias, secret).unwrap();

   let mut query_1 = AssetMap::new();
   query_1.insert_attr(Tag::Alias, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query_1).unwrap_err());

   let mut query_2 = AssetMap::new();
   query_2.insert_attr(Tag::Alias, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query_2).unwrap_err());
   remove_by_alias(&alias).unwrap();
}

#[test]
fn query_invalid_accessibility() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::Accessibility, 2);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::Accessibility, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::Accessibility, 3);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_accessibility_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::Accessibility, 2);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();

   query.insert_attr(Tag::Accessibility, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::Accessibility, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_auth_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthType, 0);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthType, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthType, 256);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_auth_type_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthType, 0);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();

   query.insert_attr(Tag::AuthType, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthType, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_sync_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::SyncType, SyncType::Never);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::SyncType, 5);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::SyncType, 3);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_sync_type_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthType, 0);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthType, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthType, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_critical_1() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical1, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical1, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical1, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_critical_1_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical1, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical1, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical1, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_critical_2() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical2, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical2, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical2, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_critical_2_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical2, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical2, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical2, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_critical_3() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical3, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical3, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical3, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_critical_3_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical3, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical3, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical3, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_critical_4() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical4, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical4, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical4, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_critical_4_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelCritical4, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelCritical4, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelCritical4, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_normal_1() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal1, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal1, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal1, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_normal_1_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal1, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal1, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal1, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_normal_2() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal2, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal2, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal2, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_normal_2_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal2, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal2, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal2, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_normal_3() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal3, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal3, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal3, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_normal_3_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal3, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal3, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal3, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_data_lable_normal_4() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal4, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal4, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal4, 513);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_data_lable_normal_4_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::DataLabelNormal4, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::DataLabelNormal4, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::DataLabelNormal4, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_return_limit() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnLimit, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnLimit, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::ReturnLimit, 65537);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_return_limit_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnLimit, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnLimit, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::ReturnLimit, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_return_offset_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnOffset, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnOffset, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::ReturnOffset, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_return_ordered_by() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnOrderedBy, Tag::DataLabelCritical1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnOrderedBy, Tag::Alias);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_return_ordered_by_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnOrderedBy, Tag::DataLabelCritical1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();

   query.insert_attr(Tag::ReturnOrderedBy, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::ReturnOrderedBy, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_return_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnType, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnType, 2);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_return_type_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::ReturnType, 1);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();

   query.insert_attr(Tag::ReturnType, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::ReturnType, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_auth_challenge() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthChallenge, vec![0; 32]);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthChallenge, vec![0; 31]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthChallenge, vec![0; 33]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_auth_challenge_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthChallenge, vec![0; 32]);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();

   query.insert_attr(Tag::AuthChallenge, 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthChallenge, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_invalid_auth_token() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthToken, vec![0; 148]);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthToken, vec![0; 147]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthToken, vec![0; 149]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}

#[test]
fn query_auth_token_with_unmatched_type() {
   let alias = "alias_unlock".as_bytes();
   let secret = "secret_unlock".as_bytes();
   let mut attrs = AssetMap::new();
   attrs.insert_attr(Tag::Alias, alias.to_vec());
   attrs.insert_attr(Tag::Secret, secret.to_vec());
   attrs.insert_attr(Tag::AuthToken, vec![0; 148]);
   asset_sdk::Manager::build().unwrap().add(&attrs).unwrap();

   let mut query = AssetMap::new();

   query.insert_attr(Tag::AuthToken, 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthToken, true);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   remove_by_alias(alias).unwrap();
}
