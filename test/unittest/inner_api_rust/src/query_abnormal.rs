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
fn query_alias_short_len() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::Alias, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_alias_long_len() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::Alias, vec![0; MAX_ALIAS_SIZE + 1]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_accessibility() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::Accessibility, (Accessibility::DeviceFirstUnlock as u32) - 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::Accessibility, (Accessibility::DeviceUnlock as u32) + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_required_pwd_with_unmatched_type() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::RequirePasswordSet, vec![]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::RequirePasswordSet, 0);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_auth_type() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthType, (AuthType::None as u32) + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthType, (AuthType::Any as u32) + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_sync_type() {
   let mut query = AssetMap::new();
   let sync_type = SyncType::ThisDevice as u32 | SyncType::TrustedAccount as u32 | SyncType::TrustedDevice as u32;
   query.insert_attr(Tag::SyncType, sync_type + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_delete_type() {
   let mut query = AssetMap::new();
   let delete_type = DeleteType::WhenPackageRemoved as u32 | DeleteType::WhenUserRemoved as u32;
   query.insert_attr(Tag::DeleteType, delete_type + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_label() {
   let labels = &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
   for &label in labels {
      let mut query = AssetMap::new();
      query.insert_attr(label, vec![]);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

      query.insert_attr(label, vec![0; MAX_LABEL_SIZE + 1]);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   }
}

#[test]
fn query_invalid_return_limit() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnLimit, MIN_NUMBER_VALUE);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::ReturnLimit, MAX_RETURN_LIMIT + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_return_ordered_by() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnOrderedBy, Tag::Alias);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_return_type() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::ReturnType, ReturnType::Attributes as u32 + 1);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_auth_challenge() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE - 1]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE + 1]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_auth_token() {
   let mut query = AssetMap::new();
   query.insert_attr(Tag::AuthToken, vec![0; AUTH_TOKEN_SIZE - 1]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

   query.insert_attr(Tag::AuthToken, vec![0; AUTH_TOKEN_SIZE + 1]);
   assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_bytes_tag_with_unmatched_type() {
   let mut tags_bytes = [CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
   tags_bytes.extend(&[Tag::AuthToken, Tag::AuthChallenge, Tag::Alias]);
   for tag in tags_bytes {
      let mut query = AssetMap::new();
      query.insert_attr(tag, 0);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

      query.insert_attr(tag, true);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   }
}

#[test]
fn query_number_tag_with_unmatched_type() {
   let tags_bytes = [
      Tag::Accessibility,
      Tag::AuthType,
      Tag::SyncType,
      Tag::DeleteType,
      Tag::ReturnLimit,
      Tag::ReturnOffset,
      Tag::ReturnOrderedBy,
      Tag::ReturnType,
   ];
   for tag in tags_bytes {
      let mut query = AssetMap::new();
      query.insert_attr(tag, 0);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

      query.insert_attr(tag, true);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   }
}

#[test]
fn query_unsupported_tags() {
   let tags_bytes = [Tag::Secret];
   for tag in tags_bytes {
      let mut query = AssetMap::new();
      query.insert_attr(tag, vec![0; MIN_ARRAY_SIZE + 1]);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   }

   let tags_num = [Tag::AuthValidityPeriod, Tag::ConflictResolution];
   for tag in tags_num {
      let mut query = AssetMap::new();
      query.insert_attr(tag, 1);
      assert_eq!(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
   }
}
