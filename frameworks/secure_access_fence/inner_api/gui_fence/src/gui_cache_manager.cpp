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

#include "gui_cache_manager.h"

namespace OHOS::Security::SAF {

GuiCacheManager &GuiCacheManager::GetInstance()
{
    static GuiCacheManager instance;
    return instance;
}

bool GuiCacheManager::IsDenied(const std::string &callerBundleName, int32_t callerIndex,
                               const std::string &targetBundleName, int32_t targetIndex)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    AppPairKey key = std::make_tuple(callerBundleName, callerIndex, targetBundleName, targetIndex);
    return deniedSet_.find(key) != deniedSet_.end();
}

void GuiCacheManager::UpdateFromFullList(const std::vector<AppPairKey> &deniedPairs)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    deniedSet_.clear();
    for (const auto &pair : deniedPairs) {
        deniedSet_.insert(pair);
    }
    initialized_ = true;
}

bool GuiCacheManager::IsInitialized()
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return initialized_;
}

void GuiCacheManager::BeginRefresh()
{
    refreshing_ = true;
}

void GuiCacheManager::EndRefresh()
{
    refreshing_ = false;
    std::lock_guard<std::mutex> lock(refreshMutex_);
    refreshCv_.notify_all();
}

void GuiCacheManager::WaitForRefresh()
{
    if (!refreshing_) {
        return;
    }
    std::unique_lock<std::mutex> lock(refreshMutex_);
    refreshCv_.wait(lock, [this]() {
        return !refreshing_;
    });
}

void GuiCacheManager::ResetForTesting()
{
    auto &instance = GetInstance();
    std::unique_lock<std::shared_mutex> lock(instance.mutex_);
    instance.deniedSet_.clear();
    instance.initialized_ = false;
    instance.refreshing_ = false;
    std::lock_guard<std::mutex> refreshLock(instance.refreshMutex_);
    instance.refreshCv_.notify_all();
}

} // namespace OHOS::Security::SAF
