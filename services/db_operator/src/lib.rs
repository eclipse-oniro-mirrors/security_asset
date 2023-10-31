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
#[path = "statement.rs"]
#[allow(dead_code)]
pub mod statement;
#[path = "table.rs"]
#[allow(dead_code)]
pub mod table;
#[cfg(test)]
#[path = "../../../test/unittest/db_operator_test/src/test_for_helper.rs"]
mod test_for_helper;
#[path = "transaction.rs"]
pub mod transaction;
#[path = "types.rs"]
pub mod types;
