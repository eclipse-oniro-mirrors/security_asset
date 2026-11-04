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

#include "saf_gui_thread_pool.h"
#include "saf_log.h"
#include "saf_result_code.h"

namespace OHOS::Security::SAF {

static constexpr int32_t ERR_OUT_OF_MEMORY = 0x31001;

SafGuiThreadPool::SafGuiThreadPool() : pool_(nullptr), isRunning_(false)
{}

SafGuiThreadPool::~SafGuiThreadPool()
{
    (void)StopPool();
}

int32_t SafGuiThreadPool::StartPool()
{
    std::lock_guard<std::mutex> lock(poolMutex_);
    if (isRunning_) {
        LOGI("SafGuiThreadPool is already running.");
        return SAF_SUCCESS;
    }

    if (pool_ != nullptr) {
        LOGI("SafGuiThreadPool reusing existing thread pool");
        isRunning_ = true;
        return SAF_SUCCESS;
    }
    
    auto newPool = std::unique_ptr<ThreadPool>(new (std::nothrow) ThreadPool());
    if (newPool == nullptr) {
        LOGE("SafGuiThreadPool failed to allocate thread pool");
        return ERR_OUT_OF_MEMORY;
    }

    uint32_t ret = newPool->Start(SAF_GUI_THREAD_NUM);
    if (ret != 0) {
        LOGE("SafGuiThreadPool failed to start thread pool, ret=%{public}u", ret);
        return SAF_ERR_THREAD_POOL_START_FAIL;
    }

    pool_ = std::move(newPool);
    pool_->SetMaxTaskNum(SAF_GUI_MAX_TASK_NUM);
    isRunning_ = true;
    LOGI("SafGuiThreadPool started successfully with %{public}u threads", SAF_GUI_THREAD_NUM);
    return SAF_SUCCESS;
}

int32_t SafGuiThreadPool::StopPool()
{
    std::lock_guard<std::mutex> lock(poolMutex_);
    isRunning_ = false;
    if (pool_ == nullptr) {
        LOGI("SafGuiThreadPool already stopped");
        return SAF_SUCCESS;
    }

    while (pool_->GetCurTaskNum() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SAF_GUI_CHECK_INTERVAL_MS));
    }

    pool_->Stop();
    pool_.reset();
    LOGI("SafGuiThreadPool stopped successfully");
    return SAF_SUCCESS;
}

int32_t SafGuiThreadPool::AddTask(std::function<void()> &&task)
{
    std::lock_guard<std::mutex> lock(poolMutex_);
    if (!isRunning_ || pool_ == nullptr) {
        LOGE("SafGuiThreadPool is not running, cannot add task");
        return SAF_ERR_THREAD_POOL_NOT_RUNNING;
    }

    if (pool_->GetCurTaskNum() >= pool_->GetMaxTaskNum()) {
        LOGE("SafGuiThreadPool cannot add task: queue is full");
        return SAF_ERR_THREAD_POOL_QUEUE_FULL;
    }

    pool_->AddTask(std::move(task));
    return SAF_SUCCESS;
}

bool SafGuiThreadPool::IsRunning() const
{
    std::lock_guard<std::mutex> lock(poolMutex_);
    return isRunning_;
}

} // namespace OHOS::Security::SAF