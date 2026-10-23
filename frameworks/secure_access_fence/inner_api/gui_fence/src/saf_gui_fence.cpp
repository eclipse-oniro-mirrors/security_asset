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

#include "saf_gui_fence.h"

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "errors.h"
#include "gui_cache_manager.h"
#include "isecure_access_fence.h"
#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "saf_log.h"
#include "saf_result_code.h"

namespace OHOS::Security::SAF {

namespace {

constexpr int32_t WAIT_TIMEOUT_IN_SEC = 5;
constexpr int32_t SAF_SERVICE_ID = 66532;
constexpr int32_t ERR_REMOTE_DEAD = 29189;

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
            return iface_cast<ISecureAccessFence>(ret);
        }
    }
    sptr<IRemoteObject> sa = samgrProxy->LoadSystemAbility(SAF_SERVICE_ID, WAIT_TIMEOUT_IN_SEC);
    if (sa == nullptr) {
        LOGE("LoadSystemAbility failed");
        return nullptr;
    }
    return iface_cast<ISecureAccessFence>(sa);
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
    return ret;
}

} // namespace

// Validate callerApp and targetApp parameters.
static int32_t ValidateCheckControlParams(const AppInfo &callerApp, const AppInfo &targetApp)
{
    if (callerApp.bundleName.empty()) {
        LOGE("CheckControlPermission failed, callerApp.bundleName is empty");
        return SAF_ERR_ARG_EMPTY;
    }
    if (targetApp.bundleName.empty()) {
        LOGE("CheckControlPermission failed, targetApp.bundleName is empty");
        return SAF_ERR_ARG_EMPTY;
    }
    return SAF_SUCCESS;
}

// Parse a deniedPair string "callerBundle:callerIdx:targetBundle:targetIdx" into AppPairKey.
// Returns false if the format is invalid.
static bool ParseDeniedPair(const std::string &pair, AppPairKey &key)
{
    size_t pos1 = pair.find(':');
    if (pos1 == std::string::npos) {
        return false;
    }
    std::string callerBundle = pair.substr(0, pos1);
    size_t pos2 = pair.find(':', pos1 + 1);
    if (pos2 == std::string::npos) {
        return false;
    }
    int32_t callerIdx = std::stoi(pair.substr(pos1 + 1, pos2 - pos1 - 1));
    size_t pos3 = pair.find(':', pos2 + 1);
    if (pos3 == std::string::npos) {
        return false;
    }
    std::string targetBundle = pair.substr(pos2 + 1, pos3 - pos2 - 1);
    int32_t targetIdx = std::stoi(pair.substr(pos3 + 1));
    key = std::make_tuple(callerBundle, callerIdx, targetBundle, targetIdx);
    return true;
}

// First-time full-list fetch via IPC 505 to initialize the cache.
// Returns SAF_SUCCESS on success.
static int32_t InitCacheFromService(const AppInfo &callerApp, const AppInfo &targetApp)
{
    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("CheckControlPermission failed, load sa fail");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }

    std::vector<std::string> deniedPairs;
    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&]() { return proxy->CheckControlPermission(callerApp, targetApp, deniedPairs, resultCode); });
    if (ret != SAF_SUCCESS) {
        LOGE("CheckControlPermission IPC call failed, ret=%{public}d", ret);
        return SAF_ERR_IPC_PROXY_FAIL;
    }
    if (resultCode != SAF_SUCCESS) {
        LOGE("CheckControlPermission service error, resultCode=%{public}d", resultCode);
        return resultCode;
    }

    std::vector<AppPairKey> deniedKeys;
    for (const auto &pair : deniedPairs) {
        AppPairKey key;
        if (ParseDeniedPair(pair, key)) {
            deniedKeys.push_back(key);
        }
    }
    GuiCacheManager::GetInstance().UpdateFromFullList(deniedKeys);
    return SAF_SUCCESS;
}

// Async logging via IPC 509 (does not block return).
static void AsyncStoreOperationRecord(const AppInfo &callerApp, const AppInfo &targetApp,
    ControlDecision decision)
{
    AppInfo callerCopy = callerApp;
    AppInfo targetCopy = targetApp;
    std::thread asyncThread([callerCopy, targetCopy, decision]() {
        auto proxy = GetProxy(g_mutex);
        if (proxy == nullptr) {
            LOGE("StoreOperationRecord async: load sa fail");
            return;
        }
        int32_t resultCode = SAF_SUCCESS;
        int32_t ret = HandleIpcError(proxy,
            [&]() { return proxy->StoreOperationRecord(callerCopy, targetCopy, decision, resultCode); });
        if (ret != SAF_SUCCESS) {
            LOGE("StoreOperationRecord async IPC failed, ret=%{public}d", ret);
            return;
        }
        if (resultCode != SAF_SUCCESS) {
            LOGE("StoreOperationRecord async service error, resultCode=%{public}d", resultCode);
            return;
        }
        LOGI("StoreOperationRecord async success");
    });
    asyncThread.detach();
}

int32_t SafGuiFence::CheckControlPermission(
    const AppInfo &callerApp,
    const AppInfo &targetApp,
    ControlDecision &decision)
{
    int32_t ret = ValidateCheckControlParams(callerApp, targetApp);
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    auto &cache = GuiCacheManager::GetInstance();
    if (!cache.IsInitialized()) {
        ret = InitCacheFromService(callerApp, targetApp);
        if (ret != SAF_SUCCESS) {
            return ret;
        }
    }

    bool denied = cache.IsDenied(callerApp.bundleName, callerApp.index, targetApp.bundleName, targetApp.index);
    decision = denied ? ControlDecision::DENY : ControlDecision::ALLOW;

    AsyncStoreOperationRecord(callerApp, targetApp, decision);
    return SAF_SUCCESS;
}

int32_t SafGuiFence::RegisterComponentClickCallback(
    uint64_t sessionId,
    const GuiCallback &callback)
{
    if (callback == nullptr) {
        LOGE("RegisterComponentClickCallback failed, callback is null");
        return SAF_ERR_NULL_PTR;
    }

    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("RegisterComponentClickCallback failed, load sa fail");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&]() {
            return proxy->RegisterComponentClickCallback(
                static_cast<int64_t>(sessionId), callback->AsObject(), resultCode);
        });
    if (ret != SAF_SUCCESS) {
        LOGE("RegisterComponentClickCallback IPC failed, ret=%{public}d", ret);
        return SAF_ERR_IPC_PROXY_FAIL;
    }
    if (resultCode != SAF_SUCCESS) {
        LOGE("RegisterComponentClickCallback service error, resultCode=%{public}d", resultCode);
        return resultCode;
    }
    return SAF_SUCCESS;
}

int32_t SafGuiFence::ReportComponentClickDecision(
    uint64_t sessionId,
    const ComponentClickResult &result)
{
    if (result.compId.empty()) {
        LOGE("ReportComponentClickDecision failed, compId is empty");
        return SAF_ERR_ARG_EMPTY;
    }

    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("ReportComponentClickDecision failed, load sa fail");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&]() {
            return proxy->ReportComponentClickDecision(
                static_cast<int64_t>(sessionId), result, resultCode);
        });
    if (ret != SAF_SUCCESS) {
        LOGE("ReportComponentClickDecision IPC failed, ret=%{public}d", ret);
        return SAF_ERR_IPC_PROXY_FAIL;
    }
    if (resultCode != SAF_SUCCESS) {
        LOGE("ReportComponentClickDecision service error, resultCode=%{public}d", resultCode);
        return resultCode;
    }
    return SAF_SUCCESS;
}

int32_t SafGuiFence::UnregisterComponentClickCallback(uint64_t sessionId)
{
    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("UnregisterComponentClickCallback failed, load sa fail");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }

    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&]() {
            return proxy->UnregisterComponentClickCallback(
                static_cast<int64_t>(sessionId), resultCode);
        });
    if (ret != SAF_SUCCESS) {
        LOGE("UnregisterComponentClickCallback IPC failed, ret=%{public}d", ret);
        return SAF_ERR_IPC_PROXY_FAIL;
    }
    if (resultCode != SAF_SUCCESS) {
        LOGE("UnregisterComponentClickCallback service error, resultCode=%{public}d", resultCode);
        return resultCode;
    }
    return SAF_SUCCESS;
}

} // namespace OHOS::Security::SAF
