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
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"

#include "asset_log.h"
#include "system_event_wrapper.h"

using namespace std;
using namespace OHOS;

static constexpr int32_t RETRY_TIMES_FOR_SAMGR = 50;
static constexpr int32_t RETRY_DURATION_US = 200 * 1000;

class SystemAbilityHandler : public OHOS::SystemAbilityStatusChangeStub {
public:
    SystemAbilityHandler() {};
    ~SystemAbilityHandler() = default;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override
    {
        if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
            return;
        }

        if (!SubscribeSystemEvent()) {
            LOGE("Subscribe system event failed.");
        }
    }
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override
    {
        if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
            return;
        }
        if (!UnSubscribeSystemEvent()) {
            LOGE("UnSubscribe system event failed.");
        }
    }
};

static sptr<OHOS::ISystemAbilityManager> GetSystemAbility(void)
{
    int32_t retryCount = RETRY_TIMES_FOR_SAMGR;
    OHOS::sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    while (samgrProxy == nullptr && retryCount > 0) {
        usleep(RETRY_DURATION_US);
        samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        retryCount--;
    }
    return samgrProxy;
}

static OHOS::sptr<SystemAbilityHandler> abilityHandler;

bool SubscribeSystemAbility()
{
    OHOS::sptr<ISystemAbilityManager> samgrProxy = GetSystemAbility();
    if (samgrProxy == nullptr) {
        LOGE("Get system ability failed");
        return false;
    }

    abilityHandler = new (std::nothrow) SystemAbilityHandler();
    if (abilityHandler == nullptr) {
        LOGE("Create system ability handler failed.");
        return false;
    }

    int32_t ret = samgrProxy->SubscribeSystemAbility(COMMON_EVENT_SERVICE_ID, abilityHandler);
    if (ret != ERR_OK) {
        LOGE("Subscribe system ability failed.");
        return false;
    }
    return true;
}

bool UnSubscribeSystemAbility()
{
    OHOS::sptr<ISystemAbilityManager> samgrProxy = GetSystemAbility();
    if (samgrProxy == nullptr || abilityHandler == nullptr) {
        return false;
    }

    if (samgrProxy->UnSubscribeSystemAbility(COMMON_EVENT_SERVICE_ID, abilityHandler) != ERR_OK ||
        !UnSubscribeSystemEvent()) {
        LOGE("UnSubscribe system ability or system event failed.");
        return false;
    }

    return true;
}