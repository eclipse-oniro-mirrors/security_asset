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

#ifndef CLI_TOOL_WRAPPER_H
#define CLI_TOOL_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CxxStr {
    const char* ptr;
    int32_t len;
};

struct CxxCmdInfo {
    CxxStr cmd_name;
    CxxStr sub_cmd;
};

struct CxxQueryResult {
    int32_t ret_code;
    int32_t result_code;
    int32_t perm_count;
};

int32_t CxxBatchQueryCliPermissions(
    const CxxCmdInfo* cmds,
    int32_t cmd_count,
    char* out_buf,
    int32_t buf_size,
    CxxQueryResult* out_result);

#ifdef __cplusplus
}
#endif

#endif