/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! This file implement the optional param check

use asset_common::{
    definition::{AssetMap, ErrCode, Result, Tag},
    loge
};

use crate::argument_check::OperationCode;

const ADD_REQUIRED_PARAMS: [Tag; 2] = [
    Tag::Secret, Tag::Alias
];

const UPDATE_QUERY_REQUIRED_PARAMS: [Tag; 1] = [
    Tag::Alias
];

fn check_argument_exist(arguments: &AssetMap, required_arguments: &[Tag]) -> Result<()> {
    for param in required_arguments {
        if !arguments.contains_key(param) {
            loge!("tag [{}] missed", param);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}

// todo: 测试一把update的第二个map为空的情况
pub(crate) fn check_required_tags(arguments: &AssetMap, code: &OperationCode) -> Result<()> {
    match *code {
        OperationCode::Add => check_argument_exist(arguments, &ADD_REQUIRED_PARAMS),
        OperationCode::UpdateQuery => check_argument_exist(arguments, &UPDATE_QUERY_REQUIRED_PARAMS),
        _ => Ok(())
    }
}

// todo: 分成小数组进行组装
const ADD_AVAILABLE_ARGUMENTS: [Tag; 15] = [
    Tag::Secret, Tag::Alias, Tag::Accessibility, Tag::RequirePasswordSet, Tag::AuthType, Tag::SyncType,
    Tag::ConflictResolution, Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3,
    Tag::DataLabelCritical4, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4
];

const QUERY_AVAILABLE_ARGUMENTS: [Tag; 19] = [
    Tag::Alias, Tag::Accessibility, Tag::RequirePasswordSet, Tag::AuthType, Tag::SyncType,
    Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3,
    Tag::DataLabelCritical4, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4,
    Tag::ReturnLimit, Tag::ReturnOffset, Tag::ReturnOrderBy, Tag::ReturnType, Tag::AuthToken, Tag::AuthChallenge
];

const UPDATE_AVAILABLE_ARGUMENTS: [Tag; 5] = [
    Tag::Secret, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4
];

// todo
const UPDATE_MATCH_AVAILABLE_ARGUMENTS: [Tag; 13] = [
    Tag::Alias, Tag::Accessibility, Tag::RequirePasswordSet, Tag::AuthType, Tag::SyncType,
    Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3,
    Tag::DataLabelCritical4, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4,
];

fn check_optional_tags(argument: &AssetMap, available_arguments: &[Tag]) -> Result<()> {
    for tag in argument.keys() {
        if !available_arguments.contains(tag) {
            loge!("tag [{}] is not expected!", tag);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}

pub(crate) fn check_tag_validity(argument: &AssetMap, code: &OperationCode) -> Result<()> {  // todo: bool
    check_required_tags(argument, code)?;
    match *code { // add等code能否塞到数组里？
        OperationCode::Add => check_optional_tags(argument, &ADD_AVAILABLE_ARGUMENTS),
        OperationCode::Query => check_optional_tags(argument, &QUERY_AVAILABLE_ARGUMENTS),
        OperationCode::Update => check_optional_tags(argument, &UPDATE_AVAILABLE_ARGUMENTS),
        OperationCode::UpdateQuery => check_optional_tags(argument, &UPDATE_MATCH_AVAILABLE_ARGUMENTS),
        _ => Ok(())
    }
}