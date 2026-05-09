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

//! AccountBasedTicketKeyManager implementation.

use std::collections::HashMap;
use saf_definition::{ErrCode, Result, macros_lib};
use saf_sdk::Value;
use saf_plugin::saf_plugin::SAFPlugin;
use saf_plugin_interface::plugin_interface::{EventType, ExtMap};
use crate::ticket_operation::ticket_key_manager::TicketKeyManager;

const PARAM_OS_ACCOUNT_ID: &str = "OsAccountId";
const PARAM_DERIVE_FACTOR: &str = "DeriveFactor";
const PARAM_DERIVED_KEY: &str = "DerivedKey";

pub struct AccountBasedTicketKeyManager;

impl AccountBasedTicketKeyManager {
    pub fn new() -> Self {
        Self
    }
}

impl TicketKeyManager for AccountBasedTicketKeyManager {
    fn derive_ticket_session_key(&self, os_account_id: i32, derive_factor: &[u8]) -> Result<Vec<u8>> {
        let plugin = SAFPlugin::get_instance();
        let loader = plugin.load_plugin()?;

        let mut params: ExtMap = HashMap::new();
        params.insert(PARAM_OS_ACCOUNT_ID, Value::Number(os_account_id as u32));
        params.insert(PARAM_DERIVE_FACTOR, Value::Bytes(derive_factor.to_vec()));

        let result = loader.process_event(EventType::DeriveTicketSessionKey, &mut params)
            .map_err(|e| saf_definition::SAFError::new(ErrCode::try_from(result.code)?,
            format!("derive key failed: {}", e)))?;

        match result.get(PARAM_DERIVED_KEY) {
            Some(Value::Bytes(key)) => Ok(key.clone()),
            _ => macros_lib::log_throw_error!(ErrCode::HashMapKeyNotFound, "derived key not found"),
        }
    }
}