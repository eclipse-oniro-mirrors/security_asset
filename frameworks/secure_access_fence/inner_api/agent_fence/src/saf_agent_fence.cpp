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
constexpr int32_t ERR_REMOTE_DEAD = 29189;

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

int32_t HandleIpcError(sptr<ISecureAccessFence> &proxy, std::function<int32_t()> call)
{
    if (call == nullptr) {
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    int32_t ret = call();
    if (ret == ERR_DEAD_OBJECT || ret == ERR_REMOTE_DEAD) {
        LOGW("service unavailable and not retry.");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    if (ret == SAF_ERR_SERVICE_IS_STOPPING) {
        LOGW("service is stopping, try to load sa");
        proxy = GetProxy(g_mutex, false);
        IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "try load sa fail.");
        ret = call();
    }
    return ret;
}

} // namespace

int32_t SafAgentFence::BatchQueryCommandPermission(
    const std::vector<CommandInfo> &cmds,
    std::vector<CommandPermissionInfo> &cmdPermissions)
{
    LOGI("SafAgentFence::BatchQueryCommandPermission enter");
    int32_t resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t ret = HandleIpcError(proxy,
        [&]() { return proxy->BatchQueryCommandPermission(cmds, cmdPermissions, resultCode); });

    LOGI("SafAgentFence::BatchQueryCommandPermission finished, ret = 0x%{public}x", resultCode);
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

    int32_t resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t ret = HandleIpcError(proxy,
        [&]() { return proxy->BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos, resultCode); });

    LOGI("SafAgentFence::BatchGenerateTicket finished, ret = 0x%{public}x", resultCode);
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

    int32_t resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t ret = HandleIpcError(proxy,
        [&]() { return proxy->BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes, resultCode); });
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    LOGI("SafAgentFence::BatchVerifyTicket finished, ret = 0x%{public}x", resultCode);
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    return resultCode;
}

int32_t SafAgentFence::RequestToolPermissions(
    const PermissionQuery& permissionQuery,
    PermissionQueryResult& permissionQueryResult)
{
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");
    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&]() { return proxy->RequestToolPermissions(permissionQuery, permissionQueryResult, resultCode); });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    return resultCode;
}

int32_t SafAgentFence::GrantToolPermissionsByUser(
    const std::vector<UserAuthResult>& userAuthResult,
    std::vector<VerifyTicketInfo>& ticketInfo)
{
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");
    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&]() { return proxy->GrantToolPermissionsByUser(userAuthResult, ticketInfo, resultCode); });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    return resultCode;
}

int32_t SafAgentFence::VerifyTicket(
    int32_t osAccountId,
    const std::string &callerId,
    const std::string &verifyInfo,
    std::vector<CliInfo> &cliInfos)
{
    LOGI("SafAgentFence::VerifyTicket enter");

    int32_t checkResult = CheckVerifyTicketParams(osAccountId, callerId, verifyInfo);
    if (checkResult != SAF_SUCCESS) {
        LOGE("VerifyTicket params check failed, ret=%{public}d", checkResult);
        return checkResult;
    }

    int32_t resultCode = SAF_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->VerifyTicket(osAccountId, callerId, verifyInfo, cliInfos, resultCode);
    });
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    LOGI("SafAgentFence::VerifyTicket finished, resultCode = 0x%{public}x", resultCode);
    return resultCode;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS