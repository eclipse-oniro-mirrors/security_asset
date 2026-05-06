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

#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"
#include "isecure_access_fence.h"
#include "saf_agent_params_checker.h"

#include "iservice_registry.h"
#include "saf_log.h"

namespace OHOS {
namespace Security {
namespace SAF {

namespace {

constexpr int32_t WAIT_TIMEOUT_IN_SEC = 5;
constexpr int32_t SAF_SERVICE_ID = 66532;
constexpr int32_t EERR_REMOTE_DEAD = 29189;
constexpr int32_t E_SUCCESS = 0;

std::recursive_mutex g_mutex;

sptr<ISecureAccessFence> GetProxy(std::recursive_mutex &mutex, bool needCheck = true)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        LOGE("GetSystemAbilityManager failed");
        return nullptr;
    }

    if (needCheck) {
        auto ret = samgrProxy->CheckSystemAbility(SAF_SERVICE_ID);
        if (ret != nullptr) {
            LOGI("System ability exist.");
            return iface_cast<ISecureAccessFence>(ret);
        }
    }
    sptr<IRemoteObject> service = samgrProxy->LoadSystemAbility(SAF_SERVICE_ID, WAIT_TIMEOUT_IN_SEC);
    if (service == nullptr) {
        LOGE("load service failed!");
        return nullptr;
    }

    return iface_cast<ISecureAccessFence>(service);
}

} // namespace

int32_t SafAgentFence::BatchQueryCommandPermission(
    const std::vector<CommandInfo> &cmds,
    std::vector<CommandPermissionInfo> &cmdPermissions)
{
    LOGI("SafAgentFence::BatchQueryCommandPermission enter");
    int32_t ret = E_SUCCESS;
    int resultCode = E_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("load sa fail.");
        return SEC_SAF_SERVICE_UNAVAILABLE;
    }

    ret = proxy->BatchQueryCommandPermission(cmds, cmdPermissions, resultCode);
    if (ret == ERR_DEAD_OBJECT || ret == EERR_REMOTE_DEAD) {
        return SEC_SAF_SERVICE_UNAVAILABLE;
    }
    if (ret == SEC_SAF_SERVICE_IS_STOPPING) {
        LOGW("service is stopping, try to load sa");
        proxy = GetProxy(g_mutex, false);
        if (proxy == nullptr) {
            LOGE("try load sa fail.");
            return SEC_SAF_SERVICE_UNAVAILABLE;
        }
        ret = proxy->BatchQueryCommandPermission(cmds, cmdPermissions, resultCode);
    }

    LOGI("SafAgentFence::BatchQueryCommandPermission finished");
    if (ret != 0) {
        LOGE("IPC call failed, ret=%{public}d", ret);
        return SEC_SAF_IPC_ERROR;
    }
    return resultCode;
}

int32_t SafAgentFence::BatchGenerateTicket(
    uint32_t osAccountId,
    const std::string &callerId,
    const std::vector<std::string> &messages,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    LOGI("SafAgentFence::BatchGenerateTicket enter");
    
    int32_t checkResult = CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    if (checkResult != SEC_SAF_SUCCESS) {
        LOGE("BatchGenerateTicket params check failed, ret=%{public}d", checkResult);
        return checkResult;
    }
    
    int32_t ret = SEC_SAF_IPC_ERROR;
    int resultCode = E_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("load sa fail.");
        return SEC_SAF_SERVICE_UNAVAILABLE;
    }

    ret = proxy->BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos, resultCode);
    if (ret == ERR_DEAD_OBJECT || ret == EERR_REMOTE_DEAD) {
        return SEC_SAF_SERVICE_UNAVAILABLE;
    }
    if (ret == SEC_SAF_SERVICE_IS_STOPPING) {
        LOGW("service is stopping, try to load sa");
        proxy = GetProxy(g_mutex, false);
        if (proxy == nullptr) {
            LOGE("try load sa fail.");
            return SEC_SAF_SERVICE_UNAVAILABLE;
        }
        ret = proxy->BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos, resultCode);
    }

    LOGI("SafAgentFence::BatchGenerateTicket finished");
    if (ret != 0) {
        LOGE("IPC call failed, ret=%{public}d", ret);
        return SEC_SAF_IPC_ERROR;
    }
    return resultCode;
}

int32_t SafAgentFence::BatchVerifyTicket(
    uint32_t osAccountId,
    const std::string &callerId,
    const std::vector<VerifyTicketInfo> &verifyInfos,
    std::vector<int32_t> &verifyRes)
{
    LOGI("SafAgentFence::BatchVerifyTicket enter");
    
    int32_t checkResult = CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    if (checkResult != SEC_SAF_SUCCESS) {
        LOGE("BatchVerifyTicket params check failed, ret=%{public}d", checkResult);
        return checkResult;
    }
    
    int32_t ret = SEC_SAF_IPC_ERROR;
    int resultCode = E_SUCCESS;

    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("load sa fail.");
        return SEC_SAF_SERVICE_UNAVAILABLE;
    }

    ret = proxy->BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes, resultCode);
    if (ret == ERR_DEAD_OBJECT || ret == EERR_REMOTE_DEAD) {
        return SEC_SAF_SERVICE_UNAVAILABLE;
    }
    if (ret == SEC_SAF_SERVICE_IS_STOPPING) {
        LOGW("service is stopping, try to load sa");
        proxy = GetProxy(g_mutex, false);
        if (proxy == nullptr) {
            LOGE("try load sa fail.");
            return SEC_SAF_SERVICE_UNAVAILABLE;
        }
        ret = proxy->BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes, resultCode);
    }

    LOGI("SafAgentFence::BatchVerifyTicket finished");
    if (ret != 0) {
        LOGE("IPC call failed, ret=%{public}d", ret);
        return SEC_SAF_IPC_ERROR;
    }
    return resultCode;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS