/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

//! JSON builder module for message construction

use std::str::FromStr;
use ylong_json::{JsonValue, Object, Array, Number};
use saf_definition::{macros_lib, ErrCode, Result};

/// JSON builder for constructing JSON objects with chainable API.
pub struct JsonBuilder {
    root: Object,
}

impl JsonBuilder {
    /// Creates a new empty JSON builder.
    pub fn new() -> Self {
        Self { root: Object::new() }
    }

    /// Adds a string field to the JSON object.
    pub fn add_string(&mut self, key: &str, value: &str) {
        self.root.insert(key.to_string(), JsonValue::String(value.to_string()));
    }

    /// Adds a signed number field to the JSON object.
    pub fn add_number(&mut self, key: &str, value: i64) {
        self.root.insert(key.to_string(), JsonValue::Number(Number::Signed(value)));
    }

    /// Adds an unsigned 64-bit number field to the JSON object.
    pub fn add_u64(&mut self, key: &str, value: u64) {
        self.root.insert(key.to_string(), JsonValue::Number(Number::Unsigned(value)));
    }

    /// Adds a boolean field to the JSON object.
    pub fn add_bool(&mut self, key: &str, value: bool) {
        self.root.insert(key.to_string(), JsonValue::Boolean(value));
    }

    /// Adds a string array field to the JSON object.
    pub fn add_string_array(&mut self, key: &str, values: Vec<String>) {
        let mut arr = Array::new();
        for v in values {
            arr.push(JsonValue::String(v));
        }
        self.root.insert(key.to_string(), JsonValue::Array(arr));
    }

    /// Adds an object array field to the JSON object.
    pub fn add_object_array(&mut self, key: &str, objects: Vec<Object>) {
        let mut arr = Array::new();
        for obj in objects {
            arr.push(JsonValue::Object(obj));
        }
        self.root.insert(key.to_string(), JsonValue::Array(arr));
    }

    /// Adds a nested object field to the JSON object.
    pub fn add_object(&mut self, key: &str, obj: Object) {
        self.root.insert(key.to_string(), JsonValue::Object(obj));
    }

    /// Builds and serializes the JSON object to a compact string.
    pub fn build(self) -> Result<String> {
        let json = JsonValue::Object(self.root);
        json.to_compact_string().map_err(|e| {
            macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError, "JSON serialization failed: {}", e)
        })
    }

    /// Returns a mutable reference to the root object for advanced operations.
    pub fn root(&mut self) -> &mut Object {
        &mut self.root
    }
}

impl Default for JsonBuilder {
    fn default() -> Self {
        Self::new()
    }
}

/// Creates a new empty JSON object.
pub fn new_object() -> Object {
    Object::new()
}

/// Adds a string field to a JSON object.
pub fn object_add_string(obj: &mut Object, key: &str, value: &str) {
    obj.insert(key.to_string(), JsonValue::String(value.to_string()));
}

/// Adds a number field to a JSON object.
pub fn object_add_number(obj: &mut Object, key: &str, value: i64) {
    obj.insert(key.to_string(), JsonValue::Number(Number::Signed(value)));
}

/// Gets an optional string from a JSON object by key, removing the key from the object.
/// Returns empty string if the key doesn't exist or the value is Null.
/// Returns error if the key exists but type is not String.
pub fn take_optional_string(obj: &mut Object, key: &str) -> Result<String> {
    match obj.remove(key) {
        Some(JsonValue::String(s)) => Ok(s),
        Some(JsonValue::Null) | None => Ok(String::new()),
        Some(_) => macros_lib::log_throw_error!(
            ErrCode::DataTypeMismatch,
            "[FATAL][JSON]{} type mismatch, expected String", key
        )
    }
}

/// Gets a required string from a JSON object by key, removing the key from the object.
/// Returns error if the key doesn't exist, is Null, or type mismatch.
pub fn take_required_string(obj: &mut Object, key: &str) -> Result<String> {
    match obj.remove(key) {
        Some(JsonValue::String(s)) => Ok(s),
        Some(JsonValue::Null) | None => macros_lib::log_throw_error!(
            ErrCode::ArgEmpty,
            "[FATAL][JSON]{} not found or is null", key
        ),
        Some(_) => macros_lib::log_throw_error!(
            ErrCode::DataTypeMismatch,
            "[FATAL][JSON]{} type mismatch, expected String", key
        ),
    }
}

/// Gets a required value from a JSON object by key, removing the key from the object.
/// Returns error if the key doesn't exist or is Null.
pub fn take_required_value(obj: &mut Object, key: &str) -> Result<JsonValue> {
    match obj.remove(key) {
        None | Some(JsonValue::Null) => macros_lib::log_throw_error!(
            ErrCode::ArgEmpty,
            "[FATAL][JSON]{} is null", key
        ),
        Some(v) => Ok(v),
    }
}

/// Converts a JsonValue to Object, returning error if not an object.
pub fn json_into_object(value: JsonValue) -> Result<Object> {
    match value {
        JsonValue::Object(o) => Ok(o),
        _ => macros_lib::log_throw_error!(
            ErrCode::DataTypeMismatch,
            "[FATAL][JSON]value is not an object"
        ),
    }
}

/// Gets a required JSON value and converts it to compact string representation.
/// Returns error if the key doesn't exist or is Null.
/// For String values, returns the string directly.
/// For other types (Number, Boolean, etc.), converts to compact string representation.
pub fn get_compact_json_value(json: &JsonValue, key: &str) -> Result<String> {
    match &json[key] {
        JsonValue::Null => {
            macros_lib::log_throw_error!(
                ErrCode::ArgEmpty,
                "[FATAL][JSON]{} not found or is null", key
            )
        }
        JsonValue::String(s) => Ok(s.clone()),
        value => value.to_compact_string().map_err(|e| {
            macros_lib::log_and_into_saf_error!(
                ErrCode::JsonParseError,
                "[FATAL][JSON]{} to_compact_string failed: {}", key, e
            )
        }),
    }
}

/// template function for get array from json
pub fn get_array_from_json<T, F>(json_str: &str, key: &str, convert: F) -> Result<Vec<T>>
where
    F: Fn(&JsonValue) -> Result<T>,
{
    let json: JsonValue = JsonValue::from_str(json_str).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::JsonParseError, "JSON parse failed: {}", e)
    })?;
    
    match &json[key] {
        JsonValue::Null => {
            macros_lib::log_throw_error!(
                ErrCode::ArgEmpty,
                "[FATAL][JSON]{} not found or is null", key
            )
        }
        JsonValue::Array(arr) => arr.iter().map(convert).collect(),
        _ => macros_lib::log_throw_error!(
            ErrCode::DataTypeMismatch,
            "[FATAL][JSON]{} type mismatch, expected Array", key
        ),
    }
}

/// get string array from json
pub fn get_string_array_from_json(json_str: &str, key: &str) -> Result<Vec<String>> {
    get_array_from_json(json_str, key, |v| match v {
        JsonValue::String(s) => Ok(s.clone()),
        _ => macros_lib::log_throw_error!(
            ErrCode::DataTypeMismatch,
            "[FATAL][JSON]array contains non-string element"
        ),
    })
}
