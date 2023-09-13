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

use asset_common::{definition::{AssetMap, ErrCode, Result, Tag, Value, Accessibility, AuthType, SyncType, ConflictResolution, ReturnType}, loge};

const MAX_BYTES_LEN: usize = 256;

fn check_accessibility(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_accessibility failed!")
    };
    if Accessibility::try_from(*v).is_err() {
        loge!("check accessibility value failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_secret(value: &Value) -> Result<()>
{
    let Value::Bytes(v) = value else {
        panic!("convert value to Value::Bytes in check_secret failed!")
    };
    if v.len() > MAX_BYTES_LEN {
        loge!("check secret len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_alias(value: &Value) -> Result<()>
{
    let Value::Bytes(v) = value else {
        panic!("convert value to Value::Bytes in check_alias failed!")
    };
    if v.len() > MAX_BYTES_LEN {
        loge!("check alias len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_auth_type(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_auth_type failed!")
    };
    if AuthType::try_from(*v).is_err() {
        loge!("check auth type value failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_auth_validity_period(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_auth_validity_period failed!")
    };
    if *v > 600 { // todo 限时多少
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_challenge(value: &Value) -> Result<()>
{
    let Value::Bytes(v) = value else {
        panic!("convert value to Value::Bytes in check_challenge failed!")
    };
    if v.len() > MAX_BYTES_LEN { // todo 限长多少
        loge!("check challenge len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_auth_token(value: &Value) -> Result<()>
{
    let Value::Bytes(v) = value else {
        panic!("convert value to Value::Bytes in check_auth_token failed!")
    };
    if v.len() > MAX_BYTES_LEN { // todo 限长多少
        loge!("check auth token len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_sync_type(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_sync_type failed!")
    };
    if SyncType::try_from(*v).is_err() {
        loge!("check sync type value failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_conflict_policy(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_conflict_policy failed!")
    };
    if ConflictResolution::try_from(*v).is_err() {
        loge!("check conflict policy value failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_data_label_critical(value: &Value) -> Result<()>
{
    let Value::Bytes(v) = value else {
        panic!("convert value to Value::Bytes in check_data_label_critical failed!")
    };
    if v.len() > MAX_BYTES_LEN {
        loge!("check data label critical len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_data_label_normal(value: &Value) -> Result<()>
{
    let Value::Bytes(v) = value else {
        panic!("convert value to Value::Bytes in check_data_label_normal failed!")
    };
    if v.len() > MAX_BYTES_LEN {
        loge!("check data label normal len [{}] failed!", v.len());
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_type(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_return_type failed!")
    };
    if ReturnType::try_from(*v).is_err() {
        loge!("check return type value failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_limit(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_return_limit failed!")
    };
    if *v > 100 { // todo limit限制多少
        loge!("check return limit value failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_offset(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_return_offset failed!")
    };
    if *v > 100 { // todo limit限制多少
        loge!("check return offset failed!");
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_return_order_by(value: &Value) -> Result<()>
{
    let Value::NUMBER(v) = value else {
        panic!("convert value to Value::NUMBER in check_return_type failed!")
    };
    match Tag::try_from(*v)? {
        Tag::DataLabelCritical1 | Tag::DataLabelCritical2 | Tag::DataLabelCritical3 | Tag::DataLabelCritical4 => Ok(()),
        Tag::DataLabelNormal1 | Tag::DataLabelNormal2 | Tag::DataLabelNormal3 | Tag::DataLabelNormal4 => Ok(()),
        _ => Err(ErrCode::InvalidArgument)
    }
}

fn match_tag_and_check(tag: &Tag, value: &Value) -> Result<()>
{
    match tag {
        Tag::Accessibility => check_accessibility(value),
        Tag::Secret => check_secret(value),
        Tag::Alias => check_alias(value),
        Tag::AuthType => check_auth_type(value),
        Tag::AuthValidityPeriod => check_auth_validity_period(value),
        Tag::AuthChallenge => check_challenge(value),
        Tag::AuthToken => check_auth_token(value),
        Tag::SyncType => check_sync_type(value),
        Tag::ConfictPolicy => check_conflict_policy(value),
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

pub(crate) fn check_value_validity(params: &AssetMap) -> Result<()>
{
    for (tag, value) in params {
        match_tag_and_check(tag, value)?;
    }
    Ok(())
}