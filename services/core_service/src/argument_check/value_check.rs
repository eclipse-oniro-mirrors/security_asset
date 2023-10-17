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

//! This file implement the value validity check

use asset_common::{
    definition::{AssetMap, ErrCode, Result, Tag, Value, Accessibility, AuthType, SyncType, ConflictResolution, ReturnType},
    loge
};

const MAX_SECRET_LEN: usize = 1024;
const MAX_ALIAS_LEN: usize = 512;
const MAX_AUTH_TOKEN_LEN: usize = 512;
const MAX_RETURN_LIMIT: usize = 65536;
const MAX_AUTH_VALID_PERIOD: usize = 600;

const DEFAULT_CHALLENGE_LEN: usize = 32;

fn check_bool_type(value: &Value) -> Result<()> {
    let Value::Bool(_) = value else {
        loge!("convert value to Value::Number in check_bool_type failed!");
        return Err(ErrCode::InvalidArgument);
    };
    Ok(())
}

fn check_accessibility(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_accessibility failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if Accessibility::try_from(*v).is_err() {
        loge!("check accessibility value failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_secret(value: &Value) -> Result<()> {
    let Value::Bytes(v) = value else {
        loge!("convert value to Value::Number in check_secret failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() > MAX_SECRET_LEN {
        loge!("check secret len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_alias(value: &Value) -> Result<()> {
    let Value::Bytes(v) = value else {
        loge!("convert value to Value::Number in check_alias failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() > MAX_ALIAS_LEN {
        loge!("check alias len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_auth_type(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_auth_type failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if AuthType::try_from(*v).is_err() {
        loge!("check auth type value failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_auth_validity_period(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_auth_validity_period failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if *v > MAX_AUTH_VALID_PERIOD {
        loge!("check auth valid period failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_challenge(value: &Value) -> Result<()> {
    let Value::Bytes(v) = value else {
        loge!("convert value to Value::Number in check_challenge failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() != DEFAULT_CHALLENGE_LEN {
        loge!("check challenge len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_auth_token(value: &Value) -> Result<()> {
    let Value::Bytes(v) = value else {
        loge!("convert value to Value::Number in check_auth_token failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() > MAX_AUTH_TOKEN_LEN {
        loge!("check auth token len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_sync_type(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_sync_type failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if SyncType::try_from(*v).is_err() {
        loge!("check sync type value failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_conflict_resolution(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_conflict resolution failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if ConflictResolution::try_from(*v).is_err() {
        loge!("check conflict resolution value failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_data_label_critical(value: &Value) -> Result<()> {
    let Value::Bytes(v) = value else {
        loge!("convert value to Value::Bytes in check_data_label_critical failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() > MAX_ALIAS_LEN {
        loge!("check data label critical len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_data_label_normal(value: &Value) -> Result<()> {
    let Value::Bytes(v) = value else {
        loge!("convert value to Value::Bytes in check_data_label_normal failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() > MAX_ALIAS_LEN {
        loge!("check data label normal len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_type(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_return_type failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if ReturnType::try_from(*v).is_err() {
        loge!("check return type value failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_limit(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_return_limit failed!");
        return Err(ErrCode::InvalidArgument);
    };
    if *v > MAX_RETURN_LIMIT {
        loge!("check return limit value failed! found [{}]", *v);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_offset(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_return_offset failed!");
        return Err(ErrCode::InvalidArgument);
    };
    Ok(())
}

fn check_return_order_by(value: &Value) -> Result<()> {
    let Value::Number(v) = value else {
        loge!("convert value to Value::Number in check_return_type failed!");
        return Err(ErrCode::InvalidArgument);
    };
    match Tag::try_from(*v)? {
        Tag::DataLabelCritical1 | Tag::DataLabelCritical2 | Tag::DataLabelCritical3 | Tag::DataLabelCritical4 => Ok(()),
        Tag::DataLabelNormal1 | Tag::DataLabelNormal2 | Tag::DataLabelNormal3 | Tag::DataLabelNormal4 => Ok(()),
        _ => {
            loge!("check return order by failed! found [{}]", *v);
            Err(ErrCode::InvalidArgument)
        }
    }
}

fn check_value(tag: &Tag, value: &Value) -> Result<()> {
    match tag {
        Tag::Secret => check_secret(value),
        Tag::Alias => check_alias(value),
        Tag::Accessibility => check_accessibility(value),
        Tag::RequirePasswordSet => check_bool_type(value),
        Tag::AuthType => check_auth_type(value),
        Tag::AuthValidityPeriod => check_auth_validity_period(value),
        Tag::AuthChallenge => check_challenge(value),
        Tag::AuthToken => check_auth_token(value),
        Tag::SyncType => check_sync_type(value),
        Tag::ConflictResolution => check_conflict_resolution(value),
        Tag::DataLabelCritical1 => check_data_label_critical(value),
        Tag::DataLabelCritical2 => check_data_label_critical(value),
        Tag::DataLabelCritical3 => check_data_label_critical(value),
        Tag::DataLabelCritical4 => check_data_label_critical(value),
        Tag::DataLabelNormal1 => check_data_label_normal(value),
        Tag::DataLabelNormal2 => check_data_label_normal(value),
        Tag::DataLabelNormal3 => check_data_label_normal(value),
        Tag::DataLabelNormal4 => check_data_label_normal(value),
        Tag::ReturnType => check_return_type(value),
        Tag::ReturnLimit => check_return_limit(value),
        Tag::ReturnOffset => check_return_offset(value),
        Tag::ReturnOrderBy => check_return_order_by(value),
    }
}

pub(crate) fn check_value_validity(params: &AssetMap) -> Result<()> {
    for (tag, value) in params {
        check_value(tag, value)?;
    }
    Ok(())
}