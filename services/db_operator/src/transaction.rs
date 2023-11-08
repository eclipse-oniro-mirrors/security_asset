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

//! for db transaction, support closure.
//! transaction is auto rollback if not commit by RAII.

use asset_definition::{log_throw_error, ErrCode, Result};

use crate::database::Database;

/// Transaction for sqlite db
#[repr(C)]
pub struct Transaction<'a> {
    /// point to db
    db: &'a Database,
    started: bool,
}

impl<'a> Transaction<'a> {
    /// create transaction
    pub fn new(db: &'a Database) -> Transaction<'a> {
        Transaction { db, started: false }
    }

    /// start transaction
    pub fn begin(&mut self) -> Result<()> {
        if self.started {
            // do nothing
            return Ok(());
        }
        let sql = "begin transaction";
        let ret = self.db.exec(sql);
        if ret.is_ok() {
            self.started = true;
        }
        ret
    }

    /// cancel transaction
    pub fn rollback(self) -> Result<()> {
        if !self.started {
            return log_throw_error!(ErrCode::DatabaseError, "[FATAL]Transaction rollback without begin");
        }
        let ret = self.rollback_transaction();
        core::mem::forget(self);
        ret
    }

    /// cancel transaction
    fn rollback_transaction(&self) -> Result<()> {
        let sql = "rollback";
        self.db.exec(sql)
    }

    /// commit transaction
    pub fn commit(self) -> Result<()> {
        if !self.started {
            return log_throw_error!(ErrCode::DatabaseError, "[FATAL]Transaction commit without begin");
        }
        let sql = "commit";
        let ret = self.db.exec(sql);
        core::mem::forget(self);
        ret
    }
}

impl<'a> Drop for Transaction<'a> {
    fn drop(&mut self) {
        if self.started {
            let _ret = self.rollback_transaction();
            #[cfg(test)]
            if _ret.is_err() {
                println!("drop transaction fail");
            }
        }
    }
}
