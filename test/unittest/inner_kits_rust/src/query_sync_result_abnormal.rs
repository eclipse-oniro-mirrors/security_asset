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

use crate::common::*;
use asset_sdk::*;

#[test]
fn query_sync_result_invalid_group() {
    let mut query = AssetMap::new();
    query.insert_attr(Tag::GroupId, vec![]);
    let mut manager = asset_sdk::Manager::build().unwrap();
    expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());

    query.insert_attr(Tag::GroupId, vec![0; MAX_GROUP_ID_SIZE + 1]);
    expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());
}

#[test]
fn query_sync_result_bool_tag_with_unmatched_type() {
    let tags = [Tag::RequireAttrEncrypted];
    let mut manager = asset_sdk::Manager::build().unwrap();
    for tag in tags {
        let mut query = AssetMap::new();
        query.insert_attr(tag, vec![]);
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());

        query.insert_attr(tag, 0);
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());
    }
}

#[test]
fn query_sync_result_bytes_tag_with_unmatched_type() {
    let tags_bytes = [Tag::GroupId];
    let mut manager = asset_sdk::Manager::build().unwrap();
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(tag, 0);
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());

        query.insert_attr(tag, true);
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());
    }
}

#[test]
fn query_sync_result_unsupported_tags() {
    let mut tags_bytes = vec![Tag::Secret, Tag::Alias, Tag::AuthToken, Tag::AuthChallenge];
    let labels_bytes = [CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    tags_bytes.extend(labels_bytes);
    let mut manager = asset_sdk::Manager::build().unwrap();
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(tag, vec![0; MIN_GROUP_ID_SIZE + 1]);
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());
    }

    let tags_num = [
        Tag::Accessibility,
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
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());
    }

    let tags_bool = [Tag::RequirePasswordSet, Tag::IsPersistent];
    for tag in tags_bool {
        let mut query = AssetMap::new();
        query.insert_attr(tag, true);
        expect_error_eq(ErrCode::ParamVerificationFailed, manager.query_sync_result(&query).unwrap_err());
    }
}