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

//! db operator module, use sqlite3

#[path = "database.rs"]
pub mod database;
#[path = "database_table_helper.rs"]
pub mod database_table_helper;
#[allow(non_snake_case, non_camel_case_types)]
#[path = "sqlite3_ffi.rs"]
mod sqlite3_ffi;
#[path = "statement.rs"]
pub mod statement;
#[path = "table.rs"]
pub mod table;
#[path = "transaction.rs"]
pub mod transaction;
#[path = "types.rs"]
pub mod types;
pub use sqlite3_ffi::*;
