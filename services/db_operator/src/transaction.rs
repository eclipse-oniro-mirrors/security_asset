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

//! This module is used to implement database transactions.
//! Transaction is auto rollback if not commit by RAII.

use asset_definition::Result;

use crate::database::Database;

/// Transaction for sqlite db
#[repr(C)]
pub struct Transaction<'a> {
    db: &'a Database,
}

impl<'a> Transaction<'a> {
    /// Create a transaction instance.
    pub(crate) fn new(db: &'a Database) -> Transaction<'a> {
        Transaction { db }
    }

    /// Begin a database transaction.
    /// Once the transaction is begun, the caller must call the rollback or commit function later.
    pub(crate) fn begin(&mut self) -> Result<()> {
        self.db.exec("begin transaction")
    }

    /// Rollback the database transaction.
    pub(crate) fn rollback(self) -> Result<()> {
        self.db.exec("rollback")
    }

    /// Commit the database transaction.
    pub(crate) fn commit(self) -> Result<()> {
        self.db.exec("commit")
    }
}
