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

//! This module is used to verify the validity of asset attributes.

use asset_definition::{
    Accessibility, AssetMap, AuthType, ConflictResolution, Conversion, ErrCode, Result, ReturnType, Tag, Value,
};
use asset_log::loge;

use crate::operations::common::{CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS};

const MIN_NUMBER_VALUE: u32 = 0;
const MAX_RETURN_LIMIT: u32 = 0x10000; // 65536
const MAX_AUTH_VALID_PERIOD: u32 = 600; // 10min

const MIN_ARRAY_SIZE: usize = 0;
const MAX_ARRAY_SIZE: usize = 1024;

const MAX_ALIAS_SIZE: usize = 256;
const MAX_LABEL_SIZE: usize = 512;

const AUTH_TOKEN_SIZE: usize = 148;
const CHALLENGE_SIZE: usize = 32;
const SYNC_TYPE_MIN_BITS: u32 = 0;
const SYNC_TYPE_MAX_BITS: u32 = 3;
const DELETE_TYPE_MIN_BITS: u32 = 1;
const DELETE_TYPE_MAX_BITS: u32 = 2;

fn check_data_type(tag: &Tag, value: &Value) -> Result<()> {
    if tag.data_type() != value.data_type() {
        loge!("[FATAL]The data type[{}] of the tag[{}] does not match that of the value.", value.data_type(), tag);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_array_size(tag: &Tag, value: &Value, min: usize, max: usize) -> Result<()> {
    let Value::Bytes(v) = value else {
        return Err(ErrCode::InvalidArgument);
    };
    if v.len() > max || v.len() <= min {
        loge!("[FATAL]The array length[{}] of Tag[{}], exceeds the valid range.", v.len(), tag);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_enum_variant<T: TryFrom<u32>>(tag: &Tag, value: &Value) -> Result<()> {
    let Value::Number(n) = value else {
        return Err(ErrCode::InvalidArgument);
    };
    if T::try_from(*n).is_err() {
        loge!("[FATAL]The value[{}] of Tag[{}] is not a legal enumeration variant", *n, tag);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_valid_bits(tag: &Tag, value: &Value, min_bits: u32, max_bits: u32) -> Result<()> {
    let Value::Number(n) = value else {
        return Err(ErrCode::InvalidArgument);
    };
    if *n >= 2_u32.pow(max_bits) || *n < (2_u32.pow(min_bits) - 1) {
        // 2: binary system
        loge!("[FATAL]The value[{}] of Tag[{}] is not in the valid bit number.", *n, tag);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_number_range(tag: &Tag, value: &Value, min: u32, max: u32) -> Result<()> {
    let Value::Number(n) = value else {
        return Err(ErrCode::InvalidArgument);
    };
    if *n <= min || *n > max {
        loge!("[FATAL]The value[{}] of Tag[{}] is not in the valid number range.", *n, tag);
        return Err(ErrCode::InvalidArgument);
    }
    Ok(())
}

fn check_tag_range(tag: &Tag, value: &Value, tags: &[Tag]) -> Result<()> {
    let Value::Number(n) = value else {
        return Err(ErrCode::InvalidArgument);
    };
    match Tag::try_from(*n) {
        Ok(value) if tags.contains(&value) => Ok(()),
        _ => {
            loge!("[FATAL]The value[{}] of Tag[{}] is not in the valid tag range.", *n, tag);
            Err(ErrCode::InvalidArgument)
        },
    }
}

fn check_data_value(tag: &Tag, value: &Value) -> Result<()> {
    match tag {
        Tag::Secret => check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_ARRAY_SIZE),
        Tag::Alias => check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_ALIAS_SIZE),
        Tag::Accessibility => check_enum_variant::<Accessibility>(tag, value),
        Tag::RequirePasswordSet => Ok(()),
        Tag::AuthType => check_enum_variant::<AuthType>(tag, value),
        Tag::AuthValidityPeriod => check_number_range(tag, value, MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD),
        Tag::AuthChallenge => check_array_size(tag, value, CHALLENGE_SIZE - 1, CHALLENGE_SIZE),
        Tag::AuthToken => check_array_size(tag, value, AUTH_TOKEN_SIZE - 1, AUTH_TOKEN_SIZE),
        Tag::SyncType => check_valid_bits(tag, value, SYNC_TYPE_MIN_BITS, SYNC_TYPE_MAX_BITS),
        Tag::DeleteType => check_valid_bits(tag, value, DELETE_TYPE_MIN_BITS, DELETE_TYPE_MAX_BITS),
        Tag::ConflictResolution => check_enum_variant::<ConflictResolution>(tag, value),
        Tag::DataLabelCritical1 | Tag::DataLabelCritical2 | Tag::DataLabelCritical3 | Tag::DataLabelCritical4 => {
            check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)
        },
        Tag::DataLabelNormal1 | Tag::DataLabelNormal2 | Tag::DataLabelNormal3 | Tag::DataLabelNormal4 => {
            check_array_size(tag, value, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)
        },
        Tag::ReturnType => check_enum_variant::<ReturnType>(tag, value),
        Tag::ReturnLimit => check_number_range(tag, value, MIN_NUMBER_VALUE, MAX_RETURN_LIMIT),
        Tag::ReturnOffset => Ok(()),
        Tag::ReturnOrderedBy => check_tag_range(tag, value, &[CRITICAL_LABEL_ATTRS, NORMAL_LABEL_ATTRS].concat()),
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
            loge!("[FATAL]The required tag [{}] is missing.", tag);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}

pub(crate) fn check_tag_validity(attrs: &AssetMap, valid_tags: &[Tag]) -> Result<()> {
    for tag in attrs.keys() {
        if !valid_tags.contains(tag) {
            loge!("[FATAL]The tag [{}] is illegal.", tag);
            return Err(ErrCode::InvalidArgument);
        }
    }
    Ok(())
}
