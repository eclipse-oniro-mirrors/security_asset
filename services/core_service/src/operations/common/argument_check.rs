/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

//! This module is used to verify the validity of asset attributes.

use asset_common::{is_user_id_exist, CallingInfo, OwnerType, ROOT_USER_UPPERBOUND};
use asset_definition::{
    log_throw_error, Accessibility, AssetMap, AuthType, ConflictResolution, Conversion, ErrCode, OperationType, Result,
    ReturnType, Tag, Value,
};
use asset_sdk::WrapType;

use crate::operations::common::{CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS, NORMAL_LOCAL_LABEL_ATTRS};

const MIN_NUMBER_VALUE: u32 = 0;
const MAX_RETURN_LIMIT: u32 = 0x10000; // 65536
const MAX_AUTH_VALID_PERIOD: u32 = 600; // 10min

const MIN_ARRAY_SIZE: usize = 0;
const MAX_SECRET_SIZE: usize = 1024;
const MAX_TIME_SIZE: usize = 1024;

const MAX_ALIAS_SIZE: usize = 256;
pub const MAX_LABEL_SIZE: usize = 2048;

const MAX_GROUP_ID_LEN: usize = 127;
const MIN_GROUP_ID_LEN: usize = 7;

const AUTH_TOKEN_SIZE: usize = 344;
const CHALLENGE_SIZE: usize = 32;
const SYNC_TYPE_MIN_BITS: u32 = 0;
const SYNC_TYPE_MAX_BITS: u32 = 3;

fn check_data_type(tag: &Tag, value: &Value) -> Result<()> {
    if tag.data_type() != value.data_type() {
        return log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL]The data type[{}] of the tag[{}] does not match that of the value.",
            value.data_type(),
            tag
        );
    }
    Ok(())
}

fn check_array_size(tag: &Tag, value: &Value, min: usize, max: usize) -> Result<()> {
    let Value::Bytes(v) = value else {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][{}] is not a bytes.", tag);
    };
    if v.len() > max || v.len() <= min {
        return log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL]The array length[{}] of Tag[{}], exceeds the valid range.",
            v.len(),
            tag
        );
    }
    Ok(())
}

fn check_enum_variant<T: TryFrom<u32>>(tag: &Tag, value: &Value) -> Result<()> {
    let Value::Number(n) = value else {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][{}] is not a number.", tag);
    };
    if T::try_from(*n).is_err() {
        return log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL]The value[{}] of Tag[{}] is not a legal enumeration variant",
            *n,
            tag
        );
    }
    Ok(())
}

fn check_valid_bits(tag: &Tag, value: &Value, min_bits: u32, max_bits: u32) -> Result<()> {
    let Value::Number(n) = value else {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][{}] is not a number.", tag);
    };
    if *n >= 2_u32.pow(max_bits) || *n < (2_u32.pow(min_bits) - 1) {
        // 2: binary system
        return log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL]The value[{}] of Tag[{}] is not in the valid bit number.",
            *n,
            tag
        );
    }
    Ok(())
}

fn check_number_range(tag: &Tag, value: &Value, min: u32, max: u32) -> Result<()> {
    let Value::Number(n) = value else {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][{}] is not a number.", tag);
    };
    if *n <= min || *n > max {
        return log_throw_error!(
            ErrCode::InvalidArgument,
            "[FATAL]The value[{}] of Tag[{}] is not in the valid number range.",
            *n,
            tag
        );
    }
    Ok(())
}

fn check_tag_range(tag: &Tag, value: &Value, tags: &[Tag]) -> Result<()> {
    let Value::Number(n) = value else {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][{}] is not a number.", tag);
    };
    match Tag::try_from(*n) {
        Ok(value) if tags.contains(&value) => Ok(()),
        _ => {
            log_throw_error!(
                ErrCode::InvalidArgument,
                "[FATAL]The value[{}] of Tag[{}] is not in the valid tag range.",
                *n,
                tag
            )
        },
    }
}

fn check_user_id(tag: &Tag, value: &Value) -> Result<()> {
    check_number_range(tag, value, ROOT_USER_UPPERBOUND, i32::MAX as u32)?;
    let Value::Number(n) = value else {
        return log_throw_error!(ErrCode::InvalidArgument, "[FATAL][{}] is not a number.", tag);
    };
    match is_user_id_exist(*n as i32) {
        Ok(res) if res => Ok(()),
        Ok(_) => log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The user id [{}] is not exist.", *n),
        Err(e) => Err(e),
    }
}

fn check_data_value(tag: &Tag, value: &Value) -> Result<()> {
    match tag {
        Tag::Secret => check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_SECRET_SIZE),
        Tag::Alias => check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_ALIAS_SIZE),
        Tag::Accessibility => check_enum_variant::<Accessibility>(tag, value),
        Tag::RequirePasswordSet | Tag::IsPersistent | Tag::RequireAttrEncrypted => Ok(()),
        Tag::AuthType => check_enum_variant::<AuthType>(tag, value),
        Tag::AuthValidityPeriod => check_number_range(tag, value, MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD),
        Tag::AuthChallenge => check_array_size(tag, value, CHALLENGE_SIZE - 1, CHALLENGE_SIZE),
        Tag::AuthToken => check_array_size(tag, value, AUTH_TOKEN_SIZE - 1, AUTH_TOKEN_SIZE),
        Tag::SyncType => check_valid_bits(tag, value, SYNC_TYPE_MIN_BITS, SYNC_TYPE_MAX_BITS),
        Tag::ConflictResolution => check_enum_variant::<ConflictResolution>(tag, value),
        Tag::DataLabelCritical1 | Tag::DataLabelCritical2 | Tag::DataLabelCritical3 | Tag::DataLabelCritical4 => {
            check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)
        },
        Tag::DataLabelNormal1 | Tag::DataLabelNormal2 | Tag::DataLabelNormal3 | Tag::DataLabelNormal4 => {
            check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)
        },
        Tag::DataLabelNormalLocal1
        | Tag::DataLabelNormalLocal2
        | Tag::DataLabelNormalLocal3
        | Tag::DataLabelNormalLocal4 => check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_LABEL_SIZE),
        Tag::ReturnType => check_enum_variant::<ReturnType>(tag, value),
        Tag::ReturnLimit => check_number_range(tag, value, MIN_NUMBER_VALUE, MAX_RETURN_LIMIT),
        Tag::ReturnOffset => Ok(()),
        Tag::ReturnOrderedBy => {
            check_tag_range(tag, value, &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS, NORMAL_LOCAL_LABEL_ATTRS].concat())
        },
        Tag::UserId => check_user_id(tag, value),
        Tag::UpdateTime => check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_TIME_SIZE),
        Tag::OperationType => check_enum_variant::<OperationType>(tag, value),
        Tag::GroupId => check_array_size(tag, value, MIN_GROUP_ID_LEN, MAX_GROUP_ID_LEN),
        Tag::WrapType => check_enum_variant::<WrapType>(tag, value),
    }
}

pub(crate) fn check_value_validity(attrs: &AssetMap) -> Result<()> {
    for (tag, value) in attrs {
        check_data_type(tag, value)?;
        check_data_value(tag, value)?;
    }
    Ok(())
}

pub(crate) fn check_required_tags(attrs: &AssetMap, required_tags: &[Tag]) -> Result<()> {
    for tag in required_tags {
        if !attrs.contains_key(tag) {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The required tag [{}] is missing.", tag);
        }
    }
    Ok(())
}

pub(crate) fn check_tag_validity(attrs: &AssetMap, valid_tags: &[Tag]) -> Result<()> {
    for tag in attrs.keys() {
        if !valid_tags.contains(tag) {
            return log_throw_error!(ErrCode::InvalidArgument, "[FATAL]The tag [{}] is illegal.", tag);
        }
    }
    Ok(())
}

pub(crate) fn check_group_validity(attrs: &AssetMap, calling_info: &CallingInfo) -> Result<()> {
    if attrs.get(&Tag::GroupId).is_some() {
        if let Some(Value::Bool(true)) = attrs.get(&Tag::IsPersistent) {
            return log_throw_error!(
                ErrCode::InvalidArgument,
                "[FATAL]The value of the tag [{}] cannot be set to true when the tag [{}] is specified.",
                &Tag::IsPersistent,
                &Tag::GroupId
            );
        }
        if calling_info.owner_type_enum() == OwnerType::Native {
            return log_throw_error!(
                ErrCode::Unsupported,
                "[FATAL]The tag [{}] is not yet supported for [{}] owner.",
                &Tag::GroupId,
                OwnerType::Native
            );
        }
        if calling_info.app_index() > 0 {
            return log_throw_error!(
                ErrCode::Unsupported,
                "[FATAL]The tag [{}] is not yet supported for clone or sandbox app.",
                &Tag::GroupId
            );
        }
    }
    Ok(())
}
