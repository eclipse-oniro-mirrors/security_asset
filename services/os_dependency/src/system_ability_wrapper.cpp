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

#include "system_ability_wrapper.h"

#include <unistd.h>

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_status_change_stub.h"

#include "asset_log.h"
#include "system_event_wrapper.h"

using namespace std;
using namespace OHOS;

static sptr<SystemAbilityHandler> abilityListener;

static constexpr int32_t RETRY_TIMES_FOR_SAMGR = 50;
static constexpr int32_t RETRY_DURATION_US = 200 * 1000;

bool SystemAbilityManager::RegisterCommonEventListener(void)
{
    sptr<ISystemAbilityManager> samgrProxy = GetSystemAbility();
    if (samgrProxy == nullptr) {
        LOGE("wait for samgr time out (10s)");
        return false;
    }
    abilityListener = new (std::nothrow) SystemAbilityHandler();
    if (abilityListener == nullptr) {
        LOGE("New ability listener failed.");
        return false;
    }
    int32_t ret = samgrProxy->SubscribeSystemAbility(LIBCESFWK_SERVICES_ID, abilityListener);
    if (ret != ERR_OK) {
        LOGE("Subscribe common event systemAbility fail.");
        return false;
    }
    return true;
}

bool SystemAbilityManager::DeregisterCommonEventListener(void)
{
    sptr<ISystemAbilityManager> samgrProxy = GetSystemAbility();
    if (samgrProxy == nullptr || abilityListener == nullptr) {
        LOGE("Params is invalid.");
        return false;
    }
    if (samgrProxy->UnSubscribeSystemAbility(LIBCESFWK_SERVICES_ID, abilityListener) != ERR_OK ||
        !UnSubscribeSystemEvent()) {
        LOGE("UnSubscribe common event systemAbility fail.");
        return false;
    }
    return true;
}

OHOS::sptr<OHOS::ISystemAbilityManager> SystemAbilityManager::GetSystemAbility(void)
{
    int32_t retryCount = RETRY_TIMES_FOR_SAMGR;
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    while (samgrProxy == nullptr) {
        LOGE("waiting for samgr...");
        if (retryCount > 0) {
            usleep(RETRY_DURATION_US);
            samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        } else {
            LOGE("wait for samgr time out (10s)");
            return nullptr;
        }
        retryCount--;
    }
    return samgrProxy;
}

SystemAbilityHandler::SystemAbilityHandler() {}

void SystemAbilityHandler::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    if (systemAbilityId != SystemAbilityManager::LIBCESFWK_SERVICES_ID) {
        LOGE("Current sa is invalid.");
        return;
    }
    if (!SubscribeSystemEvent()) {
        LOGE("Init comment event fail.");
    }
}
void SystemAbilityHandler::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    LOGI("start OnRemoveSystemAbility.");
    if (systemAbilityId != SystemAbilityManager::LIBCESFWK_SERVICES_ID) {
        LOGE("Current sa is invalid.");
        return;
    }
    if (!UnSubscribeSystemEvent()) {
        LOGE("Destroy comment event fail.");
    }
}

bool RegisterCommonEventListener() {
    return SystemAbilityManager::RegisterCommonEventListener();
}

bool DeregisterCommonEventListener() {
    return SystemAbilityManager::DeregisterCommonEventListener();
}