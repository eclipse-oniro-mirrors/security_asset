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

#ifndef GUI_CACHE_MANAGER_H
#define GUI_CACHE_MANAGER_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <vector>

namespace OHOS::Security::SAF {

/**
 * @brief Cache key type: (callerBundleName, callerIndex, targetBundleName, targetIndex) quartet.
 */
using AppPairKey = std::tuple<std::string, int32_t, std::string, int32_t>;

/**
 * @brief Memory cache manager for GUI control permission.
 *
 * Maintains a set of explicitly denied app-pairs in the caller process.
 * CheckControlPermission queries this set: in set = DENY, not in set = ALLOW (default allow).
 * Empty set means user has never set any denial, all queries return ALLOW.
 *
 * Cache size is proportional to the number of explicitly denied pairs (not total app count),
 * so memory usage is well-bounded and expected to be far below 200KB.
 *
 * Thread safety: shared_mutex for read-heavy scenario.
 * - Query (IsDenied, IsInitialized) acquires shared lock.
 * - Cache update (UpdateFromFullList) acquires unique lock.
 *
 * Refresh synchronization: atomic flag + condition_variable ensure that
 * CheckControlPermission waits for any in-progress observer-triggered refresh
 * to complete before reading the cache.
 *
 * Lifecycle: process-level singleton, lives as long as the caller process.
 */
class GuiCacheManager {
public:
    /**
     * @brief Get the singleton instance.
     * @return The singleton instance reference.
     */
    static GuiCacheManager &GetInstance();

    /**
     * @brief Check whether the app-pair (callerApp, targetApp) is denied.
     *
     * Acquires shared lock. O(log n) lookup.
     * Uninitialized state returns false (default ALLOW).
     *
     * @param callerBundleName The caller app (A app) bundle name.
     * @param callerIndex The caller app index.
     * @param targetBundleName The target app (B app) bundle name.
     * @param targetIndex The target app index.
     * @return true if the pair is explicitly denied; false otherwise (including uninitialized state).
     */
    bool IsDenied(const std::string &callerBundleName, int32_t callerIndex,
                  const std::string &targetBundleName, int32_t targetIndex);

    /**
     * @brief Update the cache from a full list of denied app-pairs.
     *
     * Acquires unique lock. Replaces all existing entries.
     * Used by both first-time initialization and settings observer triggered refresh.
     * Sets the initialized flag to true.
     *
     * @param deniedPairs Full list of denied app-pairs.
     */
    void UpdateFromFullList(const std::vector<AppPairKey> &deniedPairs);

    /**
     * @brief Check whether the cache has been initialized.
     *
     * Acquires shared lock.
     *
     * @return true if initialized; false otherwise.
     */
    bool IsInitialized();

    /**
     * @brief Mark the start of a cache refresh.
     *
     * Sets the refreshing flag so that concurrent CheckControlPermission calls
     * will wait until EndRefresh is called.
     */
    void BeginRefresh();

    /**
     * @brief Mark the end of a cache refresh.
     *
     * Clears the refreshing flag and notifies all waiting threads.
     */
    void EndRefresh();

    /**
     * @brief Wait for any in-progress refresh to complete.
     *
     * If a refresh is in progress, blocks until EndRefresh is called.
     * Returns immediately if no refresh is active.
     */
    void WaitForRefresh();

    static void ResetForTesting();

private:
    GuiCacheManager() = default;
    ~GuiCacheManager() = default;
    GuiCacheManager(const GuiCacheManager &) = delete;
    GuiCacheManager &operator=(const GuiCacheManager &) = delete;

    std::shared_mutex mutex_;
    std::set<AppPairKey> deniedSet_;
    bool initialized_ {false};
    std::atomic<bool> refreshing_ {false};
    std::mutex refreshMutex_;
    std::condition_variable refreshCv_;
};

} // namespace OHOS::Security::SAF

#endif // GUI_CACHE_MANAGER_H
