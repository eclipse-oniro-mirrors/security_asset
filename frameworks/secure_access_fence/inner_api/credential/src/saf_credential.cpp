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

#include "isecure_access_fence.h"

#include <functional>
#include <mutex>

#include "credential_inner_helper.h"
#include "saf_credential.h"
#include "saf_defines.h"
#include "saf_log.h"
#include "saf_result_code.h"

#include "iservice_registry.h"

namespace OHOS {
namespace Security {
namespace SAF {

namespace {

constexpr int32_t WAIT_TIMEOUT_IN_SEC = 5;
constexpr int32_t SAF_SERVICE_ID = 66532;

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
    int32_t normalized = NormalizeIpcResult(ret);
    if (normalized != ret) {
        LOGW("service unavailable and not retry.");
        return normalized;
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

// ===== Model Credential API =====

int32_t SafCredential::AddModelCredential(const ModelCredentialContent &modelCredentialContent,
                                          const ModelCredentialInfo &modelCredentialInfo,
                                          std::string &credentialHandle, std::string &credentialId)
{
    LOGI("SafCredential::AddModelCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->AddModelCredential(modelCredentialContent, modelCredentialInfo,
                                         credentialHandle, credentialId, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("AddModelCredential ret=%{public}d handle=%{public}s", resultCode, credentialHandle.c_str());
    return resultCode;
}

int32_t SafCredential::DeleteModelCredential(const std::vector<ModelCredential> &modelCredentials,
                                             CredentialBatchResult &result)
{
    LOGI("SafCredential::DeleteModelCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->DeleteModelCredential(modelCredentials, result, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("DeleteModelCredential ret=%{public}d failedCount=%{public}d", resultCode, result.failedCount);
    return resultCode;
}

int32_t SafCredential::UpdateModelCredential(const std::vector<ModelCredential> &sourceModelCredentials,
                                             const std::vector<ModelCredential> &destModelCredentials,
                                             CredentialBatchResult &result)
{
    LOGI("SafCredential::UpdateModelCredential start");
    if (!IsBatchSizeMatched(sourceModelCredentials.size(), destModelCredentials.size())) {
        LOGE("source/dest size mismatch: %{public}zu vs %{public}zu",
            sourceModelCredentials.size(), destModelCredentials.size());
        return SAF_ERR_ARG_INVALID;
    }
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->UpdateModelCredential(sourceModelCredentials, destModelCredentials, result, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("UpdateModelCredential ret=%{public}d failedCount=%{public}d", resultCode, result.failedCount);
    return resultCode;
}

int32_t SafCredential::GetActiveModelCredential(const std::string &credentialHandle, const std::string &model,
                                                ModelCredentialContent &modelCredentialContent)
{
    LOGI("SafCredential::GetActiveModelCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->GetActiveModelCredential(credentialHandle, model, modelCredentialContent, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("GetActiveModelCredential ret=%{public}d", resultCode);
    return resultCode;
}

int32_t SafCredential::PreGetAllModelCredentials(std::vector<ModelCredential> &modelCredentials)
{
    LOGI("SafCredential::PreGetAllModelCredentials start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->PreGetAllModelCredentials(modelCredentials, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("PreGetAllModelCredentials ret=%{public}d outSize=%{public}zu", resultCode, modelCredentials.size());
    return resultCode;
}

int32_t SafCredential::GetModelCredentialValue(const ModelCredentialIndex &modelCredentialIndex,
                                               std::string &credentialValue)
{
    LOGI("SafCredential::GetModelCredentialValue start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->GetModelCredentialValue(modelCredentialIndex, credentialValue, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("GetModelCredentialValue ret=%{public}d", resultCode);
    return resultCode;
}

// ===== Tool Credential API =====

int32_t SafCredential::AddToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                         CredentialBatchResult &result)
{
    LOGI("SafCredential::AddToolCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->AddToolCredential(toolCredentials, result, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("AddToolCredential ret=%{public}d failedCount=%{public}d", resultCode, result.failedCount);
    return resultCode;
}

int32_t SafCredential::UpdateToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                            CredentialBatchResult &result)
{
    LOGI("SafCredential::UpdateToolCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->UpdateToolCredential(toolCredentials, result, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("UpdateToolCredential ret=%{public}d failedCount=%{public}d", resultCode, result.failedCount);
    return resultCode;
}

int32_t SafCredential::DeleteToolCredential(const std::vector<ToolCredential> &toolCredentials,
                                            CredentialBatchResult &result)
{
    LOGI("SafCredential::DeleteToolCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->DeleteToolCredential(toolCredentials, result, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("DeleteToolCredential ret=%{public}d failedCount=%{public}d", resultCode, result.failedCount);
    return resultCode;
}

int32_t SafCredential::GetToolCredential(const ToolInfo &toolInfo,
                                         std::vector<ToolCredentialInfo> &toolCredentialInfos)
{
    LOGI("SafCredential::GetToolCredential start");
    auto proxy = GetProxy(g_mutex);
    IF_TRUE_LOGE_RETURN_ERR(proxy == nullptr, SAF_ERR_SERVICE_UNAVAILABLE, "load sa fail.");

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy, [&]() {
        return proxy->GetToolCredential(toolInfo, toolCredentialInfos, resultCode);
    });
    IF_ERROR_LOGE_RETURN_ERR(ret, SAF_ERR_IPC_PROXY_FAIL, "IPC call failed, ret=%{public}d", ret);
    LOGI("GetToolCredential ret=%{public}d outSize=%{public}zu", resultCode, toolCredentialInfos.size());
    return resultCode;
}

} // namespace SAF
} // namespace Security
} // namespace OHOS
