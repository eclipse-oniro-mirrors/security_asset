/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <mutex>

#include "iservice_registry.h"

#include "asset_log.h"

namespace {
    const int32_t LOAD_TIMEOUT_IN_SECONDS = 2;
    std::mutex g_serviceLock;
}

extern "C" bool LoadService(int32_t saId);

bool LoadService(int32_t saId)
{
    auto samgrProxy = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        LOGE("[FATAL][SA]Get system ability manager proxy failed.");
        return false;
    }
    auto object = samgrProxy->CheckSystemAbility(saId);
    if (object != nullptr) {
        return true;
    }

    std::lock_guard<std::mutex> lock(g_serviceLock);
    object = samgrProxy->CheckSystemAbility(saId);
    if (object != nullptr) {
        return true;
    }
    return samgrProxy->LoadSystemAbility(saId, LOAD_TIMEOUT_IN_SECONDS) != nullptr;
}
