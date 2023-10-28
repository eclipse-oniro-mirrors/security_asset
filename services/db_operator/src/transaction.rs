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

use crate::{
    database::Database,
    statement::Statement,
    types::{SqliteErrCode, DATABASE_ERROR, SQLITE_OK},
};

/// Transaction for sqlite db
#[repr(C)]
pub struct Transaction<'a> {
    /// point to db
    db: &'a Database<'a>,
    started: bool,
}

impl<'a> Transaction<'a> {
    /// create transaction
    pub fn new(db: &'a Database) -> Transaction<'a> {
        Transaction { db, started: false }
    }

    /// start transaction
    pub fn begin(&mut self) -> SqliteErrCode {
        if self.started {
            // do nothing
            return SQLITE_OK;
        }
        let sql = "begin transaction";
        let stmt = Statement::<false>::new(sql, self.db);
        let ret = stmt.exec(None, 0);
        if ret == SQLITE_OK {
            self.started = true;
        }
        ret
    }

    /// cancel transaction
    pub fn rollback(self) -> SqliteErrCode {
        if !self.started {
            return DATABASE_ERROR;
        }
        let ret = self.rollback_transaction();
        core::mem::forget(self);
        ret
    }

    /// cancel transaction
    fn rollback_transaction(&self) -> SqliteErrCode {
        let sql = "rollback";
        let stmt = Statement::<false>::new(sql, self.db);
        stmt.exec(None, 0)
    }

    /// commit transaction
    pub fn commit(self) -> SqliteErrCode {
        if !self.started {
            return DATABASE_ERROR;
        }
        let sql = "commit";
        let stmt = Statement::<false>::new(sql, self.db);
        let ret = stmt.exec(None, 0);
        core::mem::forget(self);
        ret
    }
}

impl<'a> Drop for Transaction<'a> {
    fn drop(&mut self) {
        if self.started {
            let _ret = self.rollback_transaction();
            #[cfg(test)]
            if _ret != SQLITE_OK {
                println!("drop transaction fail {}", _ret);
            }
        }
    }
}
