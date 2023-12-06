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
fn query_invalid_alias() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, vec![]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.insert_attr(Tag::Alias, vec![0; MAX_ALIAS_SIZE + 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_accessibility() {
    let mut query = AssetMap::new();

    query.insert_attr(Tag::Accessibility, (Accessibility::DeviceUnlocked as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_auth_type() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthType, (AuthType::None as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.insert_attr(Tag::AuthType, (AuthType::Any as u32) + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_sync_type() {
    let mut query = AssetMap::new();
    let sync_type = SyncType::ThisDevice as u32 | SyncType::TrustedDevice as u32;
    query.insert_attr(Tag::SyncType, sync_type + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_label() {
    let labels = &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    for &label in labels {
        let mut query = AssetMap::new();
        query.insert_attr(label, vec![]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

        query.insert_attr(label, vec![0; MAX_LABEL_SIZE + 1]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }
}

#[test]
fn query_invalid_return_limit() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::ReturnLimit, MIN_NUMBER_VALUE);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.insert_attr(Tag::ReturnLimit, MAX_RETURN_LIMIT + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_return_ordered_by() {
    let tags = [
        Tag::Secret,
        Tag::Alias,
        Tag::Accessibility,
        Tag::RequirePasswordSet,
        Tag::AuthType,
        Tag::SyncType,
        Tag::IsPersistent,
        Tag::AuthValidityPeriod,
        Tag::ReturnType,
        Tag::ReturnLimit,
        Tag::ReturnOffset,
        Tag::ReturnOrderedBy,
        Tag::ConflictResolution,
        Tag::AuthChallenge,
        Tag::AuthToken,
    ];
    for tag in tags {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::ReturnOrderedBy, tag);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }
}

#[test]
fn query_invalid_return_type() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::ReturnType, ReturnType::Attributes as u32 + 1);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_auth_challenge() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE - 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE + 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_invalid_auth_token() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthToken, vec![0; AUTH_TOKEN_SIZE - 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

    query.insert_attr(Tag::AuthToken, vec![0; AUTH_TOKEN_SIZE + 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_with_auth_token_without_auth_challenge() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, vec![1]);
    query.insert_attr(Tag::AuthType, AuthType::Any);
    query.insert_attr(Tag::ReturnType, ReturnType::All);
    query.insert_attr(Tag::AuthToken, vec![0; AUTH_TOKEN_SIZE]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_with_auth_challenge_without_auth_token() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, vec![1]);
    query.insert_attr(Tag::AuthType, AuthType::Any);
    query.insert_attr(Tag::ReturnType, ReturnType::All);
    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
}

#[test]
fn query_bool_tag_with_unmatched_type() {
    let tags = [Tag::RequirePasswordSet, Tag::IsPersistent];
    for tag in tags {
        let mut query = AssetMap::new();
        query.insert_attr(tag, vec![]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

        query.insert_attr(tag, 0);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }
}

#[test]
fn query_bytes_tag_with_unmatched_type() {
    let mut tags_bytes = [CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    tags_bytes.extend(&[Tag::AuthToken, Tag::AuthChallenge, Tag::Alias]);
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(tag, 0);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

        query.insert_attr(tag, true);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }
}

#[test]
fn query_number_tag_with_unmatched_type() {
    let tags_num = [
        Tag::Accessibility,
        Tag::AuthType,
        Tag::SyncType,
        Tag::ReturnLimit,
        Tag::ReturnOffset,
        Tag::ReturnOrderedBy,
        Tag::ReturnType,
    ];
    for tag in tags_num {
        let mut query = AssetMap::new();
        query.insert_attr(tag, vec![]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());

        query.insert_attr(tag, true);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }
}

#[test]
fn query_unsupported_tags() {
    let tags_bytes = [Tag::Secret];
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(tag, vec![0; MIN_ARRAY_SIZE + 1]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }

    let tags_num = [Tag::AuthValidityPeriod, Tag::ConflictResolution];
    for tag in tags_num {
        let mut query = AssetMap::new();
        query.insert_attr(tag, 1);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().query(&query).unwrap_err());
    }
}
