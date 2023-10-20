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

#include "system_event_wrapper.h"

#include "bundle_constants.h"
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "os_account_manager.h"

#include "asset_log.h"

extern "C" {
    int32_t delete_data_by_owner(int32_t user_id, const char* owner);
    bool delete_dir_by_user(int32_t user_id);
}

namespace {
const char *APP_ID = "appId";
void OnPackageRemoved(const OHOS::AAFwk::Want &want, bool isSandBoxApp)
{
    int userId = want.GetIntParam(OHOS::AppExecFwk::Constants::USER_ID, -1);
    std::string appId = want.GetStringParam(APP_ID);
    int appIndex = isSandBoxApp ? want.GetIntParam(OHOS::AppExecFwk::Constants::SANDBOX_APP_INDEX, -1) : 0;

    if (appId.empty() || userId == -1 || appIndex == -1) {
        LOGE("[FATAL]Get removed owner info failed, userId=%{public}i, appId=%{public}s, appIndex=%{public}d",
            userId, appId.c_str(), appIndex);
        return;
    }

    std::string owner = appId + '_' + std::to_string(appIndex);
    int totalDeleteNum = delete_data_by_owner(userId, owner.c_str());
    LOGI("[INFO] Receive event: PACKAGE_REMOVED, userId=%{public}i, appId=%{public}s, appIndex=%{public}d, "
        "deleteDataNum=%{public}d", userId, appId.c_str(), appIndex, totalDeleteNum);
}

class SystemEventHandler : public OHOS::EventFwk::CommonEventSubscriber {
public:
    SystemEventHandler(const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo) :
        OHOS::EventFwk::CommonEventSubscriber(subscribeInfo) {}
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override
    {
        auto want = data.GetWant();
        std::string action = want.GetAction();
        if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
            OnPackageRemoved(want, false);
        } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
            OnPackageRemoved(want, true);
        } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            int userId = data.GetCode();
            bool ret = delete_dir_by_user(userId);
            LOGI("[INFO] Receive event: USER_REMOVED, userId=%{public}i, deleteDirRet=%{public}d", userId, ret);
        } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
            LOGE("AssetService screen off");  // // todo: 监听锁屏广播，中止session
        }
    }
};
}

static std::shared_ptr<SystemEventHandler> g_eventHandler = nullptr;

bool SubscribeSystemEvent(void)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    OHOS::EventFwk::CommonEventSubscribeInfo subscriberInfo(matchingSkills);

    g_eventHandler = std::make_shared<SystemEventHandler>(subscriberInfo);
    if (g_eventHandler == nullptr) {
        LOGE("[FATAL]Asset system event handler is nullptr.");
        return false;
    }

    return OHOS::EventFwk::CommonEventManager::SubscribeCommonEvent(g_eventHandler);
}

bool UnSubscribeSystemEvent(void)
{
    if (g_eventHandler == nullptr) {
        LOGW("Asset system event handler is nullptr, no need to unsubscribe.");
        return false;
    }

    bool res = OHOS::EventFwk::CommonEventManager::UnSubscribeCommonEvent(g_eventHandler);
    g_eventHandler = nullptr;
    return res;
}
