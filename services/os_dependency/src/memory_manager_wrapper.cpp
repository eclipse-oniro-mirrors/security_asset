/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "memory_manager_wrapper.h"

#include "accesstoken_kit.h"
#include "app_provision_info.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"

#include "mem_mgr_client.h"

#include "asset_log.h"

namespace {
constexpr const int32_t SA_TYPE = 1;
constexpr const int32_t ASSET_SA_ID = 8100;
constexpr const int32_t MEMORY_MANAGER_SA_ID = 1909;
}

bool CheckMemoryMgr()
{
    auto systemAbilityManager = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        LOGE("[FATAL]systemAbilityManager is nullptr, please check.");
        return false;
    }
    auto memoryMgrRemoteObj = systemAbilityManager->CheckSystemAbility(MEMORY_MANAGER_SA_ID);
    if (memoryMgrRemoteObj == nullptr) {
        LOGE("[FATAL]memoryMgrRemoteObj is nullptr, please check.");
        return false;
    }
    return true;
}

int32_t NotifyStatus(int32_t status)
{
    int32_t res = OHOS::Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), SA_TYPE, status, ASSET_SA_ID);
    if (res != OHOS::ERR_OK) {
        LOGE("[FATAL]set NotifyStatus failed. ret: [%{public}d] statud: [%{public}d]", res, status);
    }
    return res;
}

int32_t SetCritical(bool critical)
{
    int32_t res = OHOS::Memory::MemMgrClient::GetInstance().SetCritical(getpid(), critical, ASSET_SA_ID);
    if (res != OHOS::ERR_OK) {
        LOGE("[FATAL]set SetCritical failed. ret: [%{public}d] critical: [%{public}d]", res, critical);
    }
    return res;
}