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

#include "saf_agent_fence.h"

#include <functional>

#include "saf_result_code.h"
#include "isecure_access_fence.h"
#include "saf_agent_params_checker.h"

#include "iservice_registry.h"
#include "saf_log.h"
#include "saf_defines.h"

namespace OHOS {
namespace Security {
namespace SAF {

namespace {

constexpr int32_t WAIT_TIMEOUT_IN_SEC = 5;
constexpr int32_t SAF_SERVICE_ID = 66532;
constexpr int32_t EERR_REMOTE_DEAD = 29189;

std::recursive_mutex g_mutex;

sptr<ISecureAccessFence> GetProxy(std::recursive_mutex &mutex, bool needCheck = true)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    IF_TRUE_LOGE_RETURN_NULL(samgrProxy == nullptr, "GetSystemAbilityManager failed");

    if (needCheck) {
        auto ret = samgrProxy->CheckSystemAbility(SAF_SERVICE_ID);
        if (ret != nullptr) {
            LOGI("System ability exist.");
            return iface_cast<ISecureAccessFence>(ret);
        }
    }
    sptr<IRemoteObject> service = samgrProxy->LoadSystemAbility(SAF_SERVICE_ID, WAIT_TIMEOUT_IN_SEC);
    IF_TRUE_LOGE_RETURN_NULL(service == nullptr, "load service failed!");

    return iface_cast<ISecureAccessFence>(service);
}

int32_t HandleIpcError(int32_t &ret, sptr<ISecureAccessFence> &proxy, std::function<int32_t()> retryCall)
{
    if (ret == ERR_DEAD_OBJECT || ret == EERR_REMOTE_DEAD) {
        LOGW("service unavailable and not retry.");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    if (ret == SAF_ERR_SERVICE_IS_STOPPING) {
        LOGW("service is stopping, try to load sa");
        proxy = GetProxy(g_mutex, false);
        IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "try load sa fail.");
        ret = retryCall();
    }
    return SAF_SUCCESS;
}

} // namespace

int32_t SafAgentFence::BatchQueryCommandPermission(
    const std::vector<CommandInfo> &cmds,
    std::vector<CommandPermissionInfo> &cmdPermissions)
{
    LOGI("SafAgentFence::BatchQueryCommandPermission enter");
    int32_t ret = SAF_SUCCESS;
    int resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    ret = proxy->BatchQueryCommandPermission(cmds, cmdPermissions, resultCode);
    int32_t errResult = HandleIpcError(ret, proxy,
        [&]() { return proxy->BatchQueryCommandPermission(cmds, cmdPermissions, resultCode); });
    if (errResult != SAF_SUCCESS) {
        return errResult;
    }

    LOGI("SafAgentFence::BatchQueryCommandPermission finished");
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    return resultCode;
}

int32_t SafAgentFence::BatchGenerateTicket(
    int32_t osAccountId,
    const std::string &callerId,
    const std::vector<std::string> &messages,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    LOGI("SafAgentFence::BatchGenerateTicket enter");

    int32_t checkResult = CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    if (checkResult != SAF_SUCCESS) {
        LOGE("BatchGenerateTicket params check failed, ret=%{public}d", checkResult);
        return checkResult;
    }

    int32_t ret = SAF_ERR_IPC_PROXY_FAIL;
    int resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    ret = proxy->BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos, resultCode);
    int32_t errResult = HandleIpcError(ret, proxy,
        [&]() { return proxy->BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos, resultCode); });
    if (errResult != SAF_SUCCESS) {
        return errResult;
    }

    LOGI("SafAgentFence::BatchGenerateTicket finished");
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    return resultCode;
}

int32_t SafAgentFence::BatchVerifyTicket(
    int32_t osAccountId,
    const std::string &callerId,
    const std::vector<VerifyTicketInfo> &verifyInfos,
    std::vector<int32_t> &verifyRes)
{
    LOGI("SafAgentFence::BatchVerifyTicket enter");

    int32_t checkResult = CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    if (checkResult != SAF_SUCCESS) {
        LOGE("BatchVerifyTicket params check failed, ret=%{public}d", checkResult);
        return checkResult;
    }

    int32_t ret = SAF_ERR_IPC_PROXY_FAIL;
    int resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    ret = proxy->BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes, resultCode);
    int32_t errResult = HandleIpcError(ret, proxy,
        [&]() { return proxy->BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes, resultCode); });
    if (errResult != SAF_SUCCESS) {
        return errResult;
    }

    LOGI("SafAgentFence::BatchVerifyTicket finished");
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    return resultCode;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS