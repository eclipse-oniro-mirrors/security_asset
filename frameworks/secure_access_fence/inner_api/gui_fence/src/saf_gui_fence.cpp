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
#include "gui_fence_inner_helper.h"
#include "isecure_access_fence.h"
#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "saf_gui_thread_pool.h"
#include "saf_log.h"
#include "saf_result_code.h"
#include "settings_observer.h"
#include "os_account_manager.h"

namespace OHOS::Security::SAF {

namespace {

constexpr int32_t WAIT_TIMEOUT_IN_SEC = 5;
constexpr int32_t SAF_SERVICE_ID = 66532;

std::recursive_mutex g_mutex;
// Observer kept alive for the process lifetime. All reads/writes of this
// global are guarded by g_initMutex (held by InitCacheFromService) to prevent
// concurrent double-registration / sptr refcount races when two threads call
// PreloadAgentAppStatus or CheckControlPermission simultaneously.
sptr<SettingsDataObserver> g_settingsObserver;
// Serializes InitCacheFromService invocations and protects g_settingsObserver.
std::mutex g_initMutex;

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

// RAII guard that ensures EndRefresh() runs even if an exception is thrown
// between BeginRefresh() and EndRefresh(). Without this, a std::bad_alloc from
// UpdateFromFullList (std::set::insert of std::string keys) would leave
// refreshing_ permanently true and block all WaitForRefresh callers forever.
class RefreshGuard {
public:
    explicit RefreshGuard(GuiCacheManager &cache) : cache_(cache), active_(true) {}

    ~RefreshGuard()
    {
        if (active_) {
            cache_.EndRefresh();
        }
    }

    void Dismiss()
    {
        active_ = false;
    }

    RefreshGuard(const RefreshGuard &) = delete;
    RefreshGuard &operator=(const RefreshGuard &) = delete;

private:
    GuiCacheManager &cache_;
    bool active_;
};

// Fetch full denied-pairs list from service via IPC 505 and update the cache.
// Uses BeginRefresh/EndRefresh to synchronize with concurrent readers.
static int32_t RefreshDeniedPairs()
{
    auto &cache = GuiCacheManager::GetInstance();
    cache.BeginRefresh();
    RefreshGuard guard(cache);

    auto proxy = GetProxy(g_mutex);
    if (proxy == nullptr) {
        LOGE("RefreshDeniedPairs: load sa fail");
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }

    std::vector<std::string> deniedPairs;
    int32_t resultCode = SAF_SUCCESS;
    int32_t ret = HandleIpcError(proxy,
        [&proxy, &deniedPairs, &resultCode]() { return proxy->QueryAllDeniedPairs(deniedPairs, resultCode); });
    if (ret != SAF_SUCCESS) {
        LOGE("RefreshDeniedPairs: IPC call failed, ret=%{public}d", ret);
        return SAF_ERR_IPC_PROXY_FAIL;
    }
    if (resultCode != SAF_SUCCESS) {
        LOGE("RefreshDeniedPairs: service error, resultCode=%{public}d", resultCode);
        return resultCode;
    }

    std::vector<AppPairKey> deniedKeys;
    for (const auto &pair : deniedPairs) {
        AppPairKey key;
        if (ParseDeniedPair(pair, key)) {
            deniedKeys.push_back(key);
        }
    }
    cache.UpdateFromFullList(deniedKeys);
    LOGI("RefreshedDeniedPairs: success, deniedPairs size = %{public}zu", deniedKeys.size());
    return SAF_SUCCESS;
}

// Register a settings DataShare observer to refresh the cache when agent_app_control_status changes.
static void RegisterCacheObserver()
{
    // Unregister the previous observer if it exists (e.g., after a user switch).
    if (g_settingsObserver != nullptr) {
        int32_t oldUserId = GuiCacheManager::GetInstance().GetCachedUserId();
        if (oldUserId >= 0) {
            UnregisterSettingsObserver(oldUserId, g_settingsObserver);
        }
        g_settingsObserver = nullptr;
    }
    int32_t userId = 0;
    int32_t ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    if (ret != 0) {
        LOGE("RegisterCacheObserver: GetForegroundOsAccountLocalId failed, ret=%{public}d", ret);
        return;
    }

    g_settingsObserver = sptr<SettingsDataObserver>::MakeSptr([]() {
        LOGI("Settings observer: agent_app_control_status changed, refreshing cache");
        RefreshDeniedPairs();
    });
    if (g_settingsObserver == nullptr) {
        LOGE("RegisterCacheObserver: failed to create observer");
        return;
    }

    if (!RegisterSettingsObserver(userId, g_settingsObserver)) {
        LOGE("RegisterCacheObserver: RegisterSettingsObserver failed");
        g_settingsObserver = nullptr;
        return;
    }
    // Record the user the cache now corresponds to, so that subsequent
    // PreloadAgentAppStatus calls can detect a foreground-user switch.
    GuiCacheManager::GetInstance().SetCachedUserId(userId);
    LOGI("RegisterCacheObserver: success, cachedUserId=%{public}d", userId);
}

// Forward declaration: defined after CheckControlPermission below; used here by
// the init-guard re-check.
static bool IsCacheReadyForCurrentUser();

// First-time full-list fetch via IPC 505 to initialize the cache.
// Guarded by g_initMutex: the first caller performs the actual IPC + observer
// registration; concurrent/recursive callers skip to a re-check and either
// reuse the now-initialized cache or fail fast. This prevents two threads from
// racing on g_settingsObserver (double-register, leak unregister, sptr refcount
// corruption) and avoids redundant IPC 505 round-trips.
static int32_t InitCacheFromService()
{
    std::lock_guard<std::mutex> initLock(g_initMutex);
    // Re-check under the lock: another thread may have fully initialized the
    // cache while we were waiting for g_initMutex.
    if (IsCacheReadyForCurrentUser()) {
        LOGI("InitCacheFromService: cache already initialized by a concurrent call");
        return SAF_SUCCESS;
    }
    int32_t ret = RefreshDeniedPairs();
    if (ret != SAF_SUCCESS) {
        LOGE("InitCacheFromService: RefreshDeniedPairs failed, ret=%{public}d", ret);
        return ret;
    }
    // Register observer for settings changes.
    RegisterCacheObserver();
    return SAF_SUCCESS;
}

// Async logging via IPC 509 (does not block return).
static void AsyncStoreOperationRecord(const AppInfo &callerApp, const AppInfo &targetApp,
    ControlDecision decision)
{
    AppInfo callerCopy = callerApp;
    AppInfo targetCopy = targetApp;
    auto pool = SafGuiThreadPool::GetInstance();
    if (!pool || !pool->IsRunning()) {
        LOGE("AsyncStoreOperationRecord: thread pool not running");
        return;
    }
    pool->AddTask([callerCopy, targetCopy, decision]() {
        auto proxy = GetProxy(g_mutex);
        if (proxy == nullptr) {
            LOGE("StoreOperationRecord async: load sa fail");
            return;
        }
        int32_t resultCode = SAF_SUCCESS;
        int32_t ret = HandleIpcError(proxy,
            [&proxy, &callerCopy, &targetCopy, &decision, &resultCode]() {
                return proxy->StoreOperationRecord(callerCopy, targetCopy, decision, resultCode);
            });
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
}

int32_t SafGuiFence::CheckControlPermission(
    const AppInfo &callerApp,
    const AppInfo &targetApp,
    ControlDecision &decision)
{
    auto checkStart = std::chrono::steady_clock::now();
    int32_t ret = ValidateCheckControlParams(callerApp, targetApp);
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    auto threadPool = SafGuiThreadPool::GetInstance();
    if (threadPool && !threadPool->IsRunning()) {
        int32_t poolRet = threadPool->StartPool();
        if (poolRet != SAF_SUCCESS) {
            LOGE("SafGuiFence::CheckControlPermission: StartPool failed, ret=%{public}d", poolRet);
        }
    }

    auto &cache = GuiCacheManager::GetInstance();
    if (!cache.IsInitialized()) {
        // Synchronous IPC 505 below will block this thread. Callers on the UI
        // thread should call PreloadAgentAppStatus() in advance to warm the
        // cache; landing here means the cache was not preloaded.
        LOGI("CheckControlPermission: cache not initialized, performing synchronous init "
             "(consider calling PreloadAgentAppStatus beforehand)");
        ret = InitCacheFromService();
        if (ret != SAF_SUCCESS) {
            return ret;
        }
    }
    cache.WaitForRefresh();
    bool denied = cache.IsDenied(callerApp.bundleName, callerApp.index, targetApp.bundleName, targetApp.index);
    decision = denied ? ControlDecision::DENY : ControlDecision::ALLOW;
    AsyncStoreOperationRecord(callerApp, targetApp, decision);
    auto checkEnd = std::chrono::steady_clock::now();
    auto checkCostUs = std::chrono::duration_cast<std::chrono::microseconds>(checkEnd - checkStart).count();
    LOGI("CheckControlPermission: cost=%{public}lld us", static_cast<long long>(checkCostUs));
    return SAF_SUCCESS;
}

// Ensure the background thread pool is running for async operations.
// Returns SAF_SUCCESS if the pool is running, was started, or not present.
static int32_t EnsureThreadPoolStarted()
{
    auto threadPool = SafGuiThreadPool::GetInstance();
    if (!threadPool) {
        return SAF_SUCCESS;
    }
    int32_t poolRet = threadPool->StartPool();
    if (poolRet != SAF_SUCCESS) {
        LOGE("EnsureThreadPoolStarted: StartPool failed, ret=%{public}d", poolRet);
        return poolRet;
    }
    return SAF_SUCCESS;
}

// Check whether the cache is valid for the current foreground user.
// Returns true when the cache is initialized and corresponds to the current
// foreground OS account; false otherwise (including transient account-manager
// failures, which force a re-init so the next call can retry).
static bool IsCacheReadyForCurrentUser()
{
    auto &cache = GuiCacheManager::GetInstance();
    if (!cache.IsInitialized()) {
        return false;
    }
    int32_t currentUserId = 0;
    int32_t ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(currentUserId);
    if (ret != 0) {
        LOGI("IsCacheReadyForCurrentUser: GetForegroundOsAccountLocalId failed, ret=%{public}d, re-init", ret);
        return false;
    }
    if (currentUserId != cache.GetCachedUserId()) {
        LOGI("IsCacheReadyForCurrentUser: user switched (cached=%{public}d, current=%{public}d), re-init",
             cache.GetCachedUserId(), currentUserId);
        return false;
    }
    return true;
}

int32_t SafGuiFence::PreloadAgentAppStatus()
{
    int32_t poolRet = EnsureThreadPoolStarted();
    if (poolRet != SAF_SUCCESS) {
        return poolRet;
    }
    if (IsCacheReadyForCurrentUser()) {
        LOGI("PreloadAgentAppStatus: cache already initialized for current user");
        return SAF_SUCCESS;
    }
    return InitCacheFromService();
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
        [&proxy, &sessionId, &callback, &resultCode]() {
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
        [&proxy, &sessionId, &result, &resultCode]() {
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
        [&proxy, &sessionId, &resultCode]() {
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
