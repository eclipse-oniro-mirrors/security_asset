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

// TODO: 增加判断os_account

namespace OHOS {
namespace Security {
namespace Asset {
std::shared_ptr<SystemEventHandler> SystemEventManager::systemEventHandler_ = nullptr;

SystemEventHandler::SystemEventHandler(const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : OHOS::EventFwk::CommonEventSubscriber(subscribeInfo)
{}

void SystemEventHandler::OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED ||
        action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
        int uid = want.GetIntParam(AppExecFwk::Constants::UID, -1); // TODO: replace with APPID
        // do DeleteByAppID
        // do delete key by uid, call HUKS API
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
        int userId = data.GetCode();
        // do deleteByUserID
        // do delete key by userID, call HUKS API
    }
}

bool SystemEventManager::SubscribeSystemEvent()
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED);
    OHOS::EventFwk::CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    systemEventHandler_ = std::make_shared<SystemEventHandler>(subscriberInfo);

    if (systemEventHandler_ == nullptr)
    {
        LOGE("[JIN] asset system event handler is nullptr")
        return false;
    } else {
        return OHOS::EventFwk::CommonEventManager::SubscribeCommonEvent(systemEventHandler_);
    }
}

bool SystemEventManager::UnSubscribeSystemEvent()
{
    if (systemEventHandler_ == nullptr)
    {
        LOGE("[JIN] asset system event handler is nullptr")
        return false;
    } else {
        return OHOS::EventFwk::CommonEventManager::UnSubscribeCommonEvent(systemEventHandler_);
    }
}
} // namespace Asset
} // namespace Security
} // namespace OHOS
