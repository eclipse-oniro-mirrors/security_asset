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
fn update_query_without_alias() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());

    let query = AssetMap::new();
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_invalid_alias() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, vec![]);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );

    query.insert_attr(Tag::Alias, vec![0; MAX_ALIAS_SIZE + 1]);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_alias_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());

    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, 0);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );

    query.insert_attr(Tag::Alias, true);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_invalid_accessibility() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());

    query.insert_attr(Tag::Accessibility, (Accessibility::DeviceUnlocked as u32) + 1);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_required_pwd_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    query.insert_attr(Tag::RequirePasswordSet, vec![]);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );

    query.insert_attr(Tag::RequirePasswordSet, 0);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_invalid_auth_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    query.insert_attr(Tag::AuthType, (AuthType::None as u32) + 1);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );

    query.insert_attr(Tag::AuthType, (AuthType::Any as u32) + 1);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_invalid_sync_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    let sync_type = SyncType::ThisDevice as u32 | SyncType::TrustedDevice as u32;
    query.insert_attr(Tag::SyncType, sync_type + 1);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_invalid_delete_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());
    let delete_type = DeleteType::WhenPackageRemoved as u32 | DeleteType::WhenUserRemoved as u32;
    query.insert_attr(Tag::DeleteType, delete_type + 1);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_query_invalid_label() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let labels = &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    for &label in labels {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::Alias, function_name.to_owned());
        query.insert_attr(label, vec![]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );

        query.insert_attr(label, vec![0; MAX_LABEL_SIZE + 1]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}

#[test]
fn update_query_bytes_tag_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());

    let tags_bytes = [CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat();
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::Alias, function_name.to_owned());
        query.insert_attr(tag, 0);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );

        query.insert_attr(tag, true);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}

#[test]
fn update_query_number_tag_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let tags_num = [Tag::Accessibility, Tag::AuthType, Tag::SyncType, Tag::DeleteType];
    for tag in tags_num {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::Alias, function_name.to_owned());
        query.insert_attr(tag, vec![]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );

        query.insert_attr(tag, true);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}

#[test]
fn update_query_unsupported_tags() {
    let function_name = function!().as_bytes();
    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, function_name.to_owned());
    let tags_bytes = [Tag::Secret, Tag::AuthChallenge, Tag::AuthToken];
    for tag in tags_bytes {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::Alias, function_name.to_owned());
        query.insert_attr(tag, vec![0; MIN_ARRAY_SIZE + 1]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }

    let tags_num = [
        Tag::AuthValidityPeriod,
        Tag::ConflictResolution,
        Tag::ReturnLimit,
        Tag::ReturnOffset,
        Tag::ReturnOrderedBy,
        Tag::ReturnType,
    ];
    for tag in tags_num {
        let mut query = AssetMap::new();
        query.insert_attr(Tag::Alias, function_name.to_owned());
        query.insert_attr(tag, 1);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}

#[test]
fn update_empty_attrs() {
    let function_name = function!().as_bytes();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());

    let update = AssetMap::new();
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_invalid_secret() {
    let function_name = function!().as_bytes();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());

    let mut update = AssetMap::new();
    update.insert_attr(Tag::Secret, vec![]);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );

    update.insert_attr(Tag::Secret, vec![0; MAX_ARRAY_SIZE + 1]);
    expect_error_eq(
        ErrCode::InvalidArgument,
        asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
    );
}

#[test]
fn update_invalid_label() {
    let function_name = function!().as_bytes();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());

    let labels = NORMAL_LABEL_ATTRS.to_vec();
    for label in labels {
        let mut update = AssetMap::new();
        update.insert_attr(Tag::Secret, function_name.to_owned());
        update.insert_attr(label, vec![]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );

        update.insert_attr(label, vec![0; MAX_LABEL_SIZE + 1]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}

#[test]
fn update_bytes_tag_with_unmatched_type() {
    let function_name = function!().as_bytes();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());

    let mut tags_bytes = NORMAL_LABEL_ATTRS.to_vec();
    tags_bytes.extend(&[Tag::Secret]);
    for tag in tags_bytes {
        let mut update = AssetMap::new();
        update.insert_attr(Tag::Secret, function_name.to_owned());
        update.insert_attr(tag, 0);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );

        update.insert_attr(tag, true);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}

#[test]
fn update_unsupported_tags() {
    let function_name = function!().as_bytes();
    let mut query = AssetMap::new();
    query.insert_attr(Tag::Alias, function_name.to_owned());

    let mut tags_bytes = CRITICAL_LABEL_ATTRS.to_vec();
    tags_bytes.extend(&[Tag::Alias, Tag::AuthChallenge, Tag::AuthToken]);
    for tag in tags_bytes {
        let mut update = AssetMap::new();
        update.insert_attr(Tag::Secret, function_name.to_owned());
        update.insert_attr(tag, vec![0; MIN_ARRAY_SIZE + 1]);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }

    let tags_num = [
        Tag::Accessibility,
        Tag::AuthType,
        Tag::SyncType,
        Tag::DeleteType,
        Tag::RequirePasswordSet,
        Tag::AuthValidityPeriod,
        Tag::ConflictResolution,
        Tag::ReturnLimit,
        Tag::ReturnOffset,
        Tag::ReturnOrderedBy,
        Tag::ReturnType,
    ];
    for tag in tags_num {
        let mut update = AssetMap::new();
        update.insert_attr(Tag::Secret, function_name.to_owned());
        update.insert_attr(tag, 1);
        expect_error_eq(
            ErrCode::InvalidArgument,
            asset_sdk::Manager::build().unwrap().update(&query, &update).unwrap_err(),
        );
    }
}
