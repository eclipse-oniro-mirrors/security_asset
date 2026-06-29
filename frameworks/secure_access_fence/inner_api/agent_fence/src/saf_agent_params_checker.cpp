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
#include "saf_result_code.h"

namespace OHOS::Security::SAF {

int32_t CheckBatchGenerateTicketParams(
    int32_t osAccountId,
    const std::string &callerId,
    const std::vector<std::string> &messages)
{
    if (callerId.empty()) {
        LOGE("callerId is empty");
        return SAF_ERR_ARG_EMPTY;
    }

    if (osAccountId < MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}d", osAccountId);
        return SAF_ERR_INVALID_OS_ACCOUNT_ID;
    }

    if (messages.empty() || messages.size() > MAX_VECTOR_SIZE) {
        LOGE("invalid messages size: %{public}zu", messages.size());
        return SAF_ERR_INVALID_ARRAY_LEN;
    }
    return SAF_SUCCESS;
}

int32_t CheckBatchVerifyTicketParams(
    int32_t osAccountId,
    const std::string &callerId,
    const std::vector<VerifyTicketInfo> &verifyInfos)
{
    if (callerId.empty()) {
        LOGE("callerId is empty");
        return SAF_ERR_ARG_EMPTY;
    }

    if (osAccountId < MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}d", osAccountId);
        return SAF_ERR_INVALID_OS_ACCOUNT_ID;
    }

    if (verifyInfos.empty() || verifyInfos.size() > MAX_VECTOR_SIZE) {
        LOGE("invalid verifyInfos size: %{public}zu", verifyInfos.size());
        return SAF_ERR_INVALID_ARRAY_LEN;
    }
    return SAF_SUCCESS;
}

int32_t CheckVerifyTicketParams(
    int32_t osAccountId,
    const std::string &callerId,
    const std::string &verifyInfo)
{
    if (callerId.empty() || verifyInfo.empty()) {
        LOGE("callerId or verifyInfo is empty");
        return SAF_ERR_ARG_EMPTY;
    }

    if (osAccountId < MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}d", osAccountId);
        return SAF_ERR_INVALID_OS_ACCOUNT_ID;
    }
    return SAF_SUCCESS;
}

} // namespace OHOS::Security::SAF

extern "C" {

int32_t CheckBatchGenerateTicketParamsC(int32_t osAccountId, const char* callerId, size_t messagesCount)
{
    if (callerId == nullptr || callerId[0] == '\0') {
        LOGE("callerId is null or empty");
        return SAF_ERR_ARG_EMPTY;
    }

    if (osAccountId < OHOS::Security::SAF::MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}d", osAccountId);
        return SAF_ERR_INVALID_OS_ACCOUNT_ID;
    }

    if (messagesCount == 0 || messagesCount > OHOS::Security::SAF::MAX_VECTOR_SIZE) {
        LOGE("invalid messagesCount: %{public}zu", messagesCount);
        return SAF_ERR_INVALID_ARRAY_LEN;
    }
    return SAF_SUCCESS;
}

int32_t CheckBatchVerifyTicketParamsC(int32_t osAccountId, const char* callerId, size_t verifyInfosCount)
{
    if (callerId == nullptr || callerId[0] == '\0') {
        LOGE("callerId is null or empty");
        return SAF_ERR_ARG_EMPTY;
    }

    if (osAccountId < OHOS::Security::SAF::MIN_OS_ACCOUNT_ID) {
        LOGE("invalid osAccountId: %{public}d", osAccountId);
        return SAF_ERR_INVALID_OS_ACCOUNT_ID;
    }

    if (verifyInfosCount == 0 || verifyInfosCount > OHOS::Security::SAF::MAX_VECTOR_SIZE) {
        LOGE("invalid verifyInfosCount: %{public}zu", verifyInfosCount);
        return SAF_ERR_INVALID_ARRAY_LEN;
    }
    return SAF_SUCCESS;
}

}