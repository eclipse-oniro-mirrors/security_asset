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

//! This create implement the asset

use asset_common::definition::{AssetMap, Result, Value, Tag};
use db_operator::{
    types::{Pair, DataValue},
    database_table_helper::{G_COLUMN_ACCESS_TYPE, G_COLUMN_SECRET, G_COLUMN_ALIAS, G_COLUMN_AUTH_TYPE,
        G_COLUMN_SYNC_TYPE, G_COLUMN_CRITICAL1, G_COLUMN_CRITICAL2, G_COLUMN_CRITICAL3,
        G_COLUMN_CRITICAL4, G_COLUMN_NORMAL1, G_COLUMN_NORMAL2, G_COLUMN_NORMAL3, G_COLUMN_NORMAL4}
};

use crate::definition_inner::{AssetInnerMap, InnerValue};

fn convert_value_into_db_value(value: &Value) -> Result<DataValue> {
    match value {
        Value::Number(n) => {
            Ok(DataValue::Integer(*n)) // to do 类型确认
        },
        Value::Bytes(v) => {
            Ok(DataValue::Blob(v))
        },
    }
}

fn convert_extra_value_into_db_value(value: &InnerValue) -> Result<DataValue> {
    match value {
        InnerValue::Number(n) => {
            Ok(DataValue::Integer(*n)) // to do 类型确认
        },
        InnerValue::Blob(v) => {
            Ok(DataValue::Blob(v))
        },
        InnerValue::Text(v) => {
            Ok(DataValue::Text(v))
        },
    }
}

fn get_tag_column_name(tag: &Tag) -> Option<&str> {
    match *tag {
        Tag::Accessibility => Some(G_COLUMN_ACCESS_TYPE),
        Tag::Secret => Some(G_COLUMN_SECRET),
        Tag::Alias => Some(G_COLUMN_ALIAS),
        Tag::AuthType => Some(G_COLUMN_AUTH_TYPE),
        Tag::SyncType => Some(G_COLUMN_SYNC_TYPE),
        Tag::DataLabelCritical1 => Some(G_COLUMN_CRITICAL1),
        Tag::DataLabelCritical2 => Some(G_COLUMN_CRITICAL2),
        Tag::DataLabelCritical3 => Some(G_COLUMN_CRITICAL3),
        Tag::DataLabelCritical4 => Some(G_COLUMN_CRITICAL4),
        Tag::DataLabelNormal1 => Some(G_COLUMN_NORMAL1),
        Tag::DataLabelNormal2 => Some(G_COLUMN_NORMAL2),
        Tag::DataLabelNormal3 => Some(G_COLUMN_NORMAL3),
        Tag::DataLabelNormal4 => Some(G_COLUMN_NORMAL4),
        _ => None,
    }
}

pub(crate) fn set_input_attr<'a>(input: &'a AssetMap, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    for (tag, value) in input.iter() {
        if let Some(column) = get_tag_column_name(tag) {
            vec.push(
                Pair {
                    column_name: column,
                    value: convert_value_into_db_value(value)?,
                }
            );
        }
    }
    Ok(())
}

/// xxx
pub(crate) fn set_extra_attrs<'a>(input: &'a AssetInnerMap, vec: &mut Vec<Pair<'a>>) -> Result<()> {
    for (tag, value) in input.iter() {
        vec.push(
            Pair {
                column_name: tag,
                value: convert_extra_value_into_db_value(value)?,
            }
        );
    }
    Ok(())
}