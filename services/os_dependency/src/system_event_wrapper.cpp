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

#include "asset_log.h"

namespace {
class SystemEventHandler : public OHOS::EventFwk::CommonEventSubscriber {
public:
    SystemEventHandler(const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo) :
        OHOS::EventFwk::CommonEventSubscriber(subscribeInfo) {}
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override
    {
        auto want = data.GetWant();
        std::string action = want.GetAction();
        if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED ||
            action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
            int uid = want.GetIntParam(OHOS::AppExecFwk::Constants::UID, -1); // TODO: replace with APPID
            LOGE("uid = %d", uid);
            // TODO: 增加判断os_account
            // do DeleteByAppID
            // do delete key by uid, call HUKS API
        } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            // int userId = data.GetCode();
            // do deleteByUserID
            // do delete key by userID, call HUKS API
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
    OHOS::EventFwk::CommonEventSubscribeInfo subscriberInfo(matchingSkills);

    if (g_eventHandler != nullptr) { // todo: 是否需要考虑并发
        g_eventHandler = std::make_shared<SystemEventHandler>(subscriberInfo);
        // g_eventHandler = new(std::nothrow) SystemEventHandler(subscriberInfo);
    }

    if (g_eventHandler == nullptr) {
        LOGE("[JIN] asset system event handler is nullptr");
        return false;
    }

    return OHOS::EventFwk::CommonEventManager::SubscribeCommonEvent(g_eventHandler);
}

bool UnSubscribeSystemEvent(void)
{
    if (g_eventHandler == nullptr) {
        LOGE("[JIN] asset system event handler is nullptr");
        return false;
    }

    bool res = OHOS::EventFwk::CommonEventManager::UnSubscribeCommonEvent(g_eventHandler);

    g_eventHandler = nullptr;
    // delete g_eventHandler; // todo: 是否需要考虑并发

    return res;
}
