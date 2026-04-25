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

#ifndef SAF_RESULT_H
#define SAF_RESULT_H
#define SAF_ASSIGN_ENUM_VALUE(x, y) (x) = (y),

#define SAF_ASSIGN_MODULE_TYPE_ENUM \
    // external dependencies
    SAF_ASSIGN_ENUM_VALUE(IPC, 0x10 << 12) // 0x10000 \

typedef enum {
    SAF_ASSIGN_MODULE_TYPE_ENUM
} SafModuleType;

#define SAF_ASSIGN_RESULT_CODE_ENUM \
    SAF_ASSIGN_ENUM_VALUE(SAF_SUCCESS, 0) \
    SAF_ASSIGN_ENUM_VALUE(SAF_EVALUATE_DENY, 1) \
    SAF_ASSIGN_ENUM_VALUE(SAF_ERROR, 2) \

typedef enum {
    SAF_ASSIGN_RESULT_CODE_ENUM
} SafResultCode;

#endif // SAF_RESULT_H