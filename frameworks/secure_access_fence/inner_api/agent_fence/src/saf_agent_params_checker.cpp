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

#include "saf_agent_params_checker.h"
#include "saf_log.h"
#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"

namespace OHOS::Security::SAF {

int32_t CheckBatchGenerateTicketParams(
    int32_t osAccountId,
    const std::string &callerId,
    const std::vector<std::string> &messages)
{
    if (callerId.empty()) {
        LOGE("callerId is empty");
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (osAccountId < MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}u", osAccountId);
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (messages.empty() || messages.size() > MAX_VECTOR_SIZE) {
        LOGE("invalid messages size: %{public}zu", messages.size());
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    return SEC_SAF_SUCCESS;
}

int32_t CheckBatchVerifyTicketParams(
    int32_t osAccountId,
    const std::string &callerId,
    const std::vector<VerifyTicketInfo> &verifyInfos)
{
    if (callerId.empty()) {
        LOGE("callerId is empty");
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (osAccountId < MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}u", osAccountId);
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (verifyInfos.empty() || verifyInfos.size() > MAX_VECTOR_SIZE) {
        LOGE("invalid verifyInfos size: %{public}zu", verifyInfos.size());
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    return SEC_SAF_SUCCESS;
}

} // namespace OHOS::Security::SAF

extern "C" {

int32_t CheckBatchGenerateTicketParamsC(int32_t osAccountId, const char* callerId, size_t messagesCount)
{
    if (callerId == nullptr || callerId[0] == '\0') {
        LOGE("callerId is null or empty");
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (osAccountId < OHOS::Security::SAF::MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}u", osAccountId);
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (messagesCount == 0 || messagesCount > OHOS::Security::SAF::MAX_VECTOR_SIZE) {
        LOGE("invalid messagesCount: %{public}zu", messagesCount);
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    return SEC_SAF_SUCCESS;
}

int32_t CheckBatchVerifyTicketParamsC(int32_t osAccountId, const char* callerId, size_t verifyInfosCount)
{
    if (callerId == nullptr || callerId[0] == '\0') {
        LOGE("callerId is null or empty");
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (osAccountId < OHOS::Security::SAF::MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}u", osAccountId);
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    if (verifyInfosCount == 0 || verifyInfosCount > OHOS::Security::SAF::MAX_VECTOR_SIZE) {
        LOGE("invalid verifyInfosCount: %{public}zu", verifyInfosCount);
        return SEC_SAF_PARAM_VERICATION_FAILED;
    }

    return SEC_SAF_SUCCESS;
}
}