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

#ifndef SAF_AGENT_PARAMS_CHECKER_H
#define SAF_AGENT_PARAMS_CHECKER_H

#include <string>
#include <vector>
#include <stdint.h>
#include "secure_access_fence_type.h"

namespace OHOS::Security::SAF {

constexpr uint32_t MIN_OS_ACCOUNT_ID = 100;
constexpr size_t MAX_VECTOR_SIZE = 99;

int32_t CheckBatchGenerateTicketParams(
    uint32_t osAccountId,
    const std::string &callerId,
    const std::vector<std::string> &messages);

int32_t CheckBatchVerifyTicketParams(
    uint32_t osAccountId,
    const std::string &callerId,
    const std::vector<VerifyTicketInfo> &verifyInfos);

} // namespace OHOS::Security::SAF

#ifdef __cplusplus
extern "C" {
#endif

int32_t CheckBatchGenerateTicketParamsC(uint32_t osAccountId, const char* callerId, size_t messagesCount);
int32_t CheckBatchVerifyTicketParamsC(uint32_t osAccountId, const char* callerId, size_t verifyInfosCount);

#ifdef __cplusplus
}
#endif

#endif // SAF_AGENT_PARAMS_CHECKER_H