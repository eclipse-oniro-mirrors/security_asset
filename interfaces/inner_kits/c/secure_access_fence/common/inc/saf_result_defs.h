/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SAF_RESULT_DEFS_H
#define SAF_RESULT_DEFS_H

#define SAF_ASSIGN_ENUM_VALUE(x, y) x = (y),

#define SAF_ASSIGN_MODULE_TYPE_ENUM \
    /* external dependencies */ \
    SAF_ASSIGN_ENUM_VALUE(IPC, 0x10 << 12) /* 0x10000 */ \
    SAF_ASSIGN_ENUM_VALUE(BMS, 0x11 << 12) /* 0x11000 */ \
    SAF_ASSIGN_ENUM_VALUE(SAMGR, 0x12 << 12) /* 0x12000 */ \
    SAF_ASSIGN_ENUM_VALUE(THREAD_POOL, 0x13 << 12) \
    SAF_ASSIGN_ENUM_VALUE(OS_ACCOUNT, 0x14 << 12) \
    SAF_ASSIGN_ENUM_VALUE(ACCESS_TOKEN, 0x15 << 12) \
    SAF_ASSIGN_ENUM_VALUE(HUKS, 0x17 << 12) /* 0x17000 */ \
    SAF_ASSIGN_ENUM_VALUE(CRYPTO, 0x18 << 12) \
    /* common */ \
    SAF_ASSIGN_ENUM_VALUE(ARGUMENT, 0x30 << 12) \
    SAF_ASSIGN_ENUM_VALUE(MEMORY, 0x31 << 12) \
    SAF_ASSIGN_ENUM_VALUE(PERMISSION, 0x32 << 12) \
    /* module */ \
    SAF_ASSIGN_ENUM_VALUE(TICKET_OPERATION, 0x70 << 12)

typedef enum {
    SAF_ASSIGN_MODULE_TYPE_ENUM
} SafModuleType;

#define SAF_ASSIGN_RESULT_CODE_ENUM \
    SAF_ASSIGN_ENUM_VALUE(SAF_SUCCESS, 0) \
    SAF_ASSIGN_ENUM_VALUE(SAF_EVALUATE_DENY, 1) \
    SAF_ASSIGN_ENUM_VALUE(SAF_ERROR, 2) \
    /* SAMGR */ \
    SAF_ASSIGN_ENUM_VALUE(SAF_ERR_SERVICE_UNAVAILABLE, SAMGR | 1) \
    SAF_ASSIGN_ENUM_VALUE(SAF_ERR_SERVICE_IS_STOPPING, SAMGR | 2)

typedef enum {
    SAF_ASSIGN_RESULT_CODE_ENUM
} SafResultCode;

#endif // SAF_RESULT_DEFS_H