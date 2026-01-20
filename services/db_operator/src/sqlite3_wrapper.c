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

#include <stddef.h>
#include <stdint.h>

#include "sqlite3sym.h"

const char *const DEFAULT_CIPHER = "aes-256-gcm";
const char *const DEFAULT_H_MAC_ALGO = "SHA1";
const char *const DEFAULT_KDF_ALOG = "KDF_SHA1";
const int DEFAULT_ITER = 10000;
const int DEFAULT_PAGE_SIZE = 1024;

int SqliteOpen(const char *fileName, void **ppDb)
{
    return sqlite3_open(fileName, (sqlite3 **)ppDb);
}

int SqliteCloseV2(void *db)
{
    return sqlite3_close_v2((sqlite3 *)db);
}

int SqliteExec(void *db, const char *zSql, char **pzErrMsg)
{
    return sqlite3_exec((sqlite3 *)db, zSql, NULL, NULL, pzErrMsg);
}

int SqliteFinalize(void *pStmt)
{
    return sqlite3_finalize((sqlite3_stmt *)pStmt);
}

void SqliteFree(void *p)
{
    sqlite3_free(p);
}

int SqliteChanges(void *db)
{
    return sqlite3_changes((sqlite3 *)db);
}

int SqlitePrepareV2(void *db, const char *zSql, void **ppStmt, const char **pzTail)
{
    return sqlite3_prepare_v2((sqlite3 *)db, zSql, -1, (sqlite3_stmt **)ppStmt, pzTail);
}

int SqliteBindBlob(void *pStmt, int index, const void *zData, int nData, void(*xDel)(void*))
{
    return sqlite3_bind_blob((sqlite3_stmt *)pStmt, index, zData, nData, xDel);
}

int SqliteBindInt64(void *pStmt, int index, int64_t iValue)
{
    return sqlite3_bind_int64((sqlite3_stmt *)pStmt, index, iValue);
}

int SqliteBindNull(void *pStmt, int index)
{
    return sqlite3_bind_null((sqlite3_stmt *)pStmt, index);
}

const char *SqliteErrMsg(void *db)
{
    return sqlite3_errmsg((sqlite3 *)db);
}

int SqliteStep(void *pStmt)
{
    return sqlite3_step((sqlite3_stmt *)pStmt);
}

const char *SqliteColumnName(void *pStmt, int col)
{
    return sqlite3_column_name((sqlite3_stmt *)pStmt, col);
}

int SqliteDataCount(void *pStmt)
{
    return sqlite3_data_count((sqlite3_stmt *)pStmt);
}

const void *SqliteColumnBlob(void *pStmt, int col)
{
    return sqlite3_column_blob((sqlite3_stmt *)pStmt, col);
}

int SqliteColumnInt(void *pStmt, int col)
{
    return sqlite3_column_int((sqlite3_stmt *)pStmt, col);
}

int64_t SqliteColumnInt64(void *pStmt, int col)
{
    return sqlite3_column_int64((sqlite3_stmt *)pStmt, col);
}

int SqliteColumnBytes(void *pStmt, int col)
{
    return sqlite3_column_bytes((sqlite3_stmt *)pStmt, col);
}

int SqliteColumnType(void *pStmt, int col)
{
    return sqlite3_column_type((sqlite3_stmt *)pStmt, col);
}

int SqliteReset(void *pStmt)
{
    return sqlite3_reset((sqlite3_stmt *)pStmt);
}

int SqliteKey(void *db, const void *pKey, int nKey)
{
    return sqlite3_key((sqlite3 *)db, pKey, nKey);
}

int SqliteReKeyToEmpty(const char *dbPath, const void *pKey, int nKey)
{
    CodecRekeyConfig rekeyCfg = {
        dbPath,
        { DEFAULT_CIPHER, DEFAULT_H_MAC_ALGO, DEFAULT_KDF_ALOG, pKey, nKey, DEFAULT_ITER, DEFAULT_PAGE_SIZE },
        { NULL, NULL, NULL, NULL, 0, 0, DEFAULT_PAGE_SIZE }
    };
    return sqlite3_rekey_v3(&rekeyCfg);
}
