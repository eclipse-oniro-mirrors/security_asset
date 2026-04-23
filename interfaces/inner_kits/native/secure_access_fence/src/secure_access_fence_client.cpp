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

#include "secure_access_fence_client.h"

#include "iservice_registry.h"
#include "saf_log.h"

namespace OHOS {
namespace Security {
namespace SecureAccessFence {

std::mutex SecureAccessFenceClient::instanceLock_;
sptr<SecureAccessFenceClient> SecureAccessFenceClient::instance_;

void SafDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    LOGI("OnRemoteDied called");
    SecureAccessFenceClient::GetInstance()->ClearProxy();
}

sptr<SecureAccessFenceClient> SecureAccessFenceClient::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new SecureAccessFenceClient();
        }
    }
    return instance_;
}

SecureAccessFenceClient::SecureAccessFenceClient()
{
    ConnectService();
}

SecureAccessFenceClient::~SecureAccessFenceClient()
{
    if (proxy_ != nullptr) {
        auto remote = proxy_->AsObject();
        if (remote != nullptr && deathRecipient_ != nullptr) {
            remote->RemoveDeathRecipient(deathRecipient_);
        }
    }
}

bool SecureAccessFenceClient::ConnectService()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOGE("Get SystemAbilityManager failed");
        return false;
    }

    auto remote = samgr->GetSystemAbility(SAF_SERVICE_ID);
    if (remote == nullptr) {
        LOGE("Get SystemAbility %{public}d failed", SAF_SERVICE_ID);
        return false;
    }

    if (deathRecipient_ == nullptr) {
        deathRecipient_ = new SafDeathRecipient();
    }
    remote->AddDeathRecipient(deathRecipient_);

    proxy_ = iface_cast<ISecureAccessFence>(remote);
    if (proxy_ == nullptr) {
        LOGE("iface_cast ISecureAccessFence failed");
        return false;
    }

    LOGI("ConnectService success");
    return true;
}

sptr<ISecureAccessFence> SecureAccessFenceClient::GetProxy()
{
    std::lock_guard<std::mutex> lock(proxyLock_);
    return proxy_;
}

int32_t SecureAccessFenceClient::QueryPermissionBySubCommandBatch(
    const std::vector<Command> &cmds,
    std::vector<CommandPermission> &cmdPermissions)
{
    auto proxy = GetProxy();
    if (proxy == nullptr) {
        if (!ConnectService()) {
            return E_SERVICE_UNAVAILABLE;
        }
        proxy = GetProxy();
        if (proxy == nullptr) {
            return E_SERVICE_UNAVAILABLE;
        }
    }

    if (cmds.empty()) {
        LOGE("cmds is empty");
        return E_PARAM_INVALID;
    }

    int resultCode = E_SUCCESS;
    int ret = proxy->QueryPermissionBySubCommandBatch(cmds, cmdPermissions, resultCode);

    if (ret != 0) {
        LOGE("IPC call failed, ret=%{public}d", ret);
        return E_IPC_ERROR;
    }

    LOGI("QueryPermissionBySubCommandBatch success, resultCount=%{public}zu",
          cmdPermissions.size());
    return resultCode;
}

int32_t SecureAccessFenceClient::BatchGenerateTicket(
    uint32_t osAccountId,
    const std::string &callerId,
    const std::vector<std::string> &messages,
    std::vector<VerifyTicketInfo> &ticketInfos)
{
    auto proxy = GetProxy();
    if (proxy == nullptr) {
        if (!ConnectService()) {
            return E_SERVICE_UNAVAILABLE;
        }
        proxy = GetProxy();
        if (proxy == nullptr) {
            return E_SERVICE_UNAVAILABLE;
        }
    }

    if (callerId.empty() || messages.empty()) {
        LOGE("callerId or messages is empty");
        return E_PARAM_INVALID;
    }

    int resultCode = E_SUCCESS;
    int ret = proxy->BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos, resultCode);

    if (ret != 0) {
        LOGE("IPC call failed, ret=%{public}d", ret);
        return E_IPC_ERROR;
    }

    LOGI("BatchGenerateTicket success, ticketCount=%{public}zu", ticketInfos.size());
    return resultCode;
}

int32_t SecureAccessFenceClient::BatchVerifyTicket(
    uint32_t osAccountId,
    const std::string &callerId,
    const std::vector<VerifyTicketInfo> &verifyInfos,
    std::vector<int32_t> &verifyRes)
{
    auto proxy = GetProxy();
    if (proxy == nullptr) {
        if (!ConnectService()) {
            return E_SERVICE_UNAVAILABLE;
        }
        proxy = GetProxy();
        if (proxy == nullptr) {
            return E_SERVICE_UNAVAILABLE;
        }
    }

    if (callerId.empty() || verifyInfos.empty()) {
        LOGE("callerId or verifyInfos is empty");
        return E_PARAM_INVALID;
    }

    int resultCode = E_SUCCESS;
    int ret = proxy->BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes, resultCode);

    if (ret != 0) {
        LOGE("IPC call failed, ret=%{public}d", ret);
        return E_IPC_ERROR;
    }

    LOGI("BatchVerifyTicket success, resultCount=%{public}zu", verifyRes.size());
    return resultCode;
}

void SecureAccessFenceClient::ClearProxy()
{
    LOGI("ClearProxy called");
    std::lock_guard<std::mutex> lock(proxyLock_);
    proxy_ = nullptr;
}

} // namespace SecureAccessFence
} // namespace Security
} // namespace OHOS