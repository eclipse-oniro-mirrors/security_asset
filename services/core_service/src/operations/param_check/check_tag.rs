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

//! This file implement the asset param check

use asset_common::{
    definition::{AssetMap, ErrCode, Result, Tag},
    loge};

use crate::operations::param_check::ParamCode;

const ADD_REQUIRED_PARAMS: [Tag; 2] = [
    Tag::Secret, Tag::Alias
];

const UPDATE_QUERY_REQUIRED_PARAMS: [Tag; 1] = [
    Tag::Alias
];

fn check_required_params_inner(params: &AssetMap, required_params: &[Tag]) -> Result<()> {
    for param in required_params {
        if !params.contains_key(param) {
            loge!("tag [{}] missed", param);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}

pub(crate) fn check_required_tags(params: &AssetMap, code: &ParamCode) -> Result<()> {
    match *code {
        ParamCode::Add => {
            check_required_params_inner(params, &ADD_REQUIRED_PARAMS)
        },
        ParamCode::UpdateQuery => {
            check_required_params_inner(params, &UPDATE_QUERY_REQUIRED_PARAMS)
        },
        _ => {
            Ok(())
        }
    }
}

const ADD_AVAILABLE_PARAMS: [Tag; 15] = [
    Tag::Secret, Tag::Alias, Tag::Accessibility, Tag::RequirePasswordSet, Tag::AuthType, Tag::SyncType,
    Tag::ConflictResolution, Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3,
    Tag::DataLabelCritical4, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4
];

const QUERY_AVAILABLE_PARAMS: [Tag; 19] = [
    Tag::Alias, Tag::Accessibility, Tag::RequirePasswordSet, Tag::AuthType, Tag::SyncType,
    Tag::DataLabelCritical1, Tag::DataLabelCritical2, Tag::DataLabelCritical3,
    Tag::DataLabelCritical4, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4,
    Tag::ReturnLimit, Tag::ReturnOffset, Tag::ReturnOrderBy, Tag::ReturnType, Tag::AuthToken, Tag::AuthChallenge
];

const UPDATE_AVAILABLE_PARAMS: [Tag; 5] = [
    Tag::Secret, Tag::DataLabelNormal1, Tag::DataLabelNormal2, Tag::DataLabelNormal3, Tag::DataLabelNormal4
];

// todo
const UPDATE_MATCH_AVAILABLE_PARAMS: [Tag; 1] = [
    Tag::Alias
];

fn check_tag_validity_inner(params: &AssetMap, available_params: &[Tag]) -> Result<()> {
    for (tag, _) in params.iter() {
        if !available_params.contains(tag) {
            loge!("tag [{}] is not expected!", tag);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}

pub(crate) fn check_tag_validity(params: &AssetMap, code: &ParamCode) -> Result<()> {
    check_required_tags(params, code)?;
    match *code {
        ParamCode::Add => check_tag_validity_inner(params, &ADD_AVAILABLE_PARAMS),
        ParamCode::Query => check_tag_validity_inner(params, &QUERY_AVAILABLE_PARAMS),
        ParamCode::Update => check_tag_validity_inner(params, &UPDATE_AVAILABLE_PARAMS),
        ParamCode::UpdateQuery => check_tag_validity_inner(params, &UPDATE_MATCH_AVAILABLE_PARAMS),
        _ => {
            Ok(())
        }
    }
}