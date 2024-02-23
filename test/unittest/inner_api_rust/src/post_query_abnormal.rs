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
fn post_query_auth_challenge() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE - 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());

    query.insert_attr(Tag::AuthChallenge, vec![0; CHALLENGE_SIZE + 1]);
    expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());
}

#[test]
fn post_query_bytes_tag_with_unmatched_type() {
    let tags_bytes = vec![Tag::AuthChallenge];
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(tag, 0);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());

        query.insert_attr(tag, true);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());
    }
}

#[test]
fn post_query_unsupported_tags() {
    let mut tags_bytes = vec![Tag::Secret, Tag::Alias, Tag::AuthToken];
    let labels_bytes = [CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    tags_bytes.extend(labels_bytes);
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(tag, vec![0; MIN_ARRAY_SIZE + 1]);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());
    }

    let tags_num = [
        Tag::Availability,
        Tag::AuthType,
        Tag::AuthValidityPeriod,
        Tag::SyncType,
        Tag::ReturnType,
        Tag::ReturnLimit,
        Tag::ReturnOffset,
        Tag::ReturnOrderedBy,
        Tag::ConflictResolution,
    ];
    for tag in tags_num {
        let mut query = AssetMap::new();
        query.insert_attr(tag, 1);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());
    }

    let tags_bool = [Tag::RequirePasswordSet, Tag::IsPersistent];
    for tag in tags_bool {
        let mut query = AssetMap::new();
        query.insert_attr(tag, true);
        expect_error_eq(ErrCode::InvalidArgument, asset_sdk::Manager::build().unwrap().post_query(&query).unwrap_err());
    }
}
