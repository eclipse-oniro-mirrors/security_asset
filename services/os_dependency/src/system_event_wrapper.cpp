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

#include <ctime>

#include "system_event_wrapper.h"

#include "bundle_constants.h"
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"

#include "asset_log.h"

namespace {
using namespace OHOS::AppExecFwk::Constants;
using namespace OHOS::EventFwk;

const char * const APP_ID = "appId";

void HandlePackageRemoved(const OHOS::AAFwk::Want &want, bool isSandBoxApp, OnPackageRemoved onPackageRemoved)
{
    int userId = want.GetIntParam(USER_ID, INVALID_USERID);
    std::string appId = want.GetStringParam(APP_ID);
    int appIndex = isSandBoxApp ? want.GetIntParam(SANDBOX_APP_INDEX, -1) : 0;
    if (appId.empty() || userId == INVALID_USERID || appIndex == -1) {
        LOGE("[FATAL]Get removed owner info failed, userId=%{public}i, appId=%{public}s, appIndex=%{public}d",
            userId, appId.c_str(), appIndex);
        return;
    }

    std::string owner = appId + '_' + std::to_string(appIndex);
    if (onPackageRemoved != nullptr) {
        onPackageRemoved(userId, reinterpret_cast<const uint8_t *>(owner.c_str()), owner.size());
    }
    LOGI("[INFO]Receive event: PACKAGE_REMOVED, userId=%{public}i, appId=%{public}s, appIndex=%{public}d, ",
        userId, appId.c_str(), appIndex);
}

class SystemEventHandler : public CommonEventSubscriber {
public:
    explicit SystemEventHandler(const CommonEventSubscribeInfo &subscribeInfo, OnPackageRemoved onPackageRemoved,
        OnUserRemoved onUserRemoved, OnScreenOff onScreenOff): CommonEventSubscriber(subscribeInfo)
    {
        this->onPackageRemoved = onPackageRemoved;
        this->onUserRemoved = onUserRemoved;
        this->onScreenOff = onScreenOff;
    }
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const CommonEventData &data) override
    {
        long startTime = std::clock();
        auto want = data.GetWant();
        std::string action = want.GetAction();
        if (action == CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
            HandlePackageRemoved(want, false, this->onPackageRemoved);
        } else if (action == CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
            HandlePackageRemoved(want, true, this->onPackageRemoved);
        } else if (action == CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            int userId = data.GetCode();
            if (this->onUserRemoved != nullptr) {
                this->onUserRemoved(userId);
            }
            LOGI("[INFO] Receive event: USER_REMOVED, userId=%{public}i", userId);
        } else if (action == CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
            if (this->onScreenOff != nullptr) {
                this->onScreenOff();
            }
            LOGI("[INFO]Receive event: SCREEN_OFF, start_time: %{public}ld", startTime);
        } else {
            LOGW("[WARNING]Receive unknown event: %{public}s", action.c_str());
        }
    }
private:
    OnPackageRemoved onPackageRemoved;
    OnUserRemoved onUserRemoved;
    OnScreenOff onScreenOff;
};

std::shared_ptr<SystemEventHandler> g_eventHandler = nullptr;
}

bool SubscribeSystemEvent(OnPackageRemoved onPackageRemoved, OnUserRemoved onUserRemoved, OnScreenOff onScreenOff)
{
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    CommonEventSubscribeInfo info(matchingSkills);
    if (g_eventHandler == nullptr) {
        g_eventHandler = std::shared_ptr<SystemEventHandler>(
            new (std::nothrow) SystemEventHandler(info, onPackageRemoved, onUserRemoved, onScreenOff));
        if (g_eventHandler == nullptr) {
            LOGE("[FATAL]Asset system event handler is nullptr.");
            return false;
        }
    }

    return CommonEventManager::SubscribeCommonEvent(g_eventHandler);
}

bool UnSubscribeSystemEvent(void)
{
    if (g_eventHandler == nullptr) {
        LOGW("Asset system event handler is nullptr, no need to unsubscribe.");
        return false;
    }

    bool res = CommonEventManager::UnSubscribeCommonEvent(g_eventHandler);
    g_eventHandler = nullptr;
    return res;
}
