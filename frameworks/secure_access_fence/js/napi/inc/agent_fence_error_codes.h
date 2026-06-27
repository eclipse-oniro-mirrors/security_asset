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

#ifndef AGENT_FENCE_ERROR_CODES_H
#define AGENT_FENCE_ERROR_CODES_H

typedef enum {
    PERMISSION_DENAIL = 201,
    NOT_SYSTEM_APP = 202,
    GENERAL_PARAMETER_ERROR = 401,
    INVALID_PARAMETER = 24010000,
    SERVICE_ABNORMAL = 24010001,
    COMMON_INTERNAL_ERROR = 24010002,
    ENVIRONMENT_ERROR = 24010003,
    INVALID_PERMISSION = 24010004,
    GRANT_PERMISSION_FAILED = 24010005,
} AgentFenceErrorCode;

#endif // AGENT_FENCE_ERROR_CODES_H