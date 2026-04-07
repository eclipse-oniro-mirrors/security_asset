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
#include <string>
#include <unordered_map>

#include "system_event_wrapper.h"

#include "bundle_constants.h"
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"

#include "saf_log.h"

namespace {
using namespace OHOS::AppExecFwk::Constants;
using namespace OHOS::EventFwk;

const std::unordered_map<std::string, CommonEventType> EVENT_NAME_2_TYPE_MAPPING = {
    { CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED, CommonEventType::PACKAGE_REMOVED },
    { CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED, CommonEventType::PACKAGE_ADDED },
    { CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED, CommonEventType::PACKAGE_CHANGED },
    { CommonEventSupport::COMMON_EVENT_RESTORE_START, CommonEventType::RESTORE_START },
};

const std::string COMMON_EVENT_PARAM_UID = "uid";
const std::string COMMON_EVENT_PARAM_APPINDEX = "appIndex";
const std::string COMMON_EVENT_PARAM_BUNDLE_NAME = "bundleName";
const std::string COMMON_EVENT_PARAM_USER_ID = "userId";
const int32_t DEFAULT_COMMON_EVENT_PARAM_VAL = -1;

CommonEventType GetCommonEventType(const std::string& eventName)
{
    auto pair = EVENT_NAME_2_TYPE_MAPPING.find(eventName);
    if (pair != EVENT_NAME_2_TYPE_MAPPING.end()) {
        return pair->second;
    }
    return CommonEventType::UNKNOWN;
}

class SystemEventHandler : public CommonEventSubscriber {
public:
    explicit SystemEventHandler(const CommonEventSubscribeInfo &subscribeInfo, const EventCallBack eventCallBack)
        : CommonEventSubscriber(subscribeInfo), eventCallBack(eventCallBack) {}
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const CommonEventData &data) override
    {
        auto want = data.GetWant();
        std::string eventName = want.GetAction();

        auto CommonEventType = GetCommonEventType(eventName);

        int32_t intUid = want.GetIntParam(COMMON_EVENT_PARAM_UID, DEFAULT_COMMON_EVENT_PARAM_VAL);
        std::string uid = std::to_string(intUid);
        ConstSAFBlob uidBlob = {
            .size = uid.size(), .data = reinterpret_cast<const uint8_t *>(uid.c_str())
        };

        std::string bundleName = want.GetStringParam(COMMON_EVENT_PARAM_BUNDLE_NAME);
        ConstSAFBlob bundleNameBlob = {
            .size = bundleName.size(), .data = reinterpret_cast<const uint8_t *>(bundleName.c_str())
        };

        int32_t intAppIndex = want.GetIntParam(COMMON_EVENT_PARAM_APPINDEX, DEFAULT_COMMON_EVENT_PARAM_VAL);
        std::string appIndex = std::to_string(intAppIndex);
        ConstSAFBlob appIndexBlob = {
            .size = appIndex.size(), .data = reinterpret_cast<const uint8_t *>(appIndex.c_str())
        };

        int32_t intUserId = want.GetIntParam(COMMON_EVENT_PARAM_USER_ID, DEFAULT_COMMON_EVENT_PARAM_VAL);
        std::string userId = std::to_string(intUserId);
        ConstSAFBlob userIdBlob = {
            .size = userId.size(), .data = reinterpret_cast<const uint8_t *>(userId.c_str())
        };

        if (this->eventCallBack.onCommonEvent != nullptr) {
            this->eventCallBack.onCommonEvent({ CommonEventType, uidBlob, appIndexBlob, bundleNameBlob, userIdBlob });
        }
        LOGI("[INFO]Receive event: %{public}s", eventName.c_str());
    }
private:
    const EventCallBack eventCallBack;
};

std::shared_ptr<SystemEventHandler> g_eventHandler = nullptr;
}

bool SubscribeSystemEvent(const EventCallBack eventCallBack)
{
    MatchingSkills matchingSkills;
    for (const auto& pair : EVENT_NAME_2_TYPE_MAPPING) {
        matchingSkills.AddEvent(pair.first);
    }

    CommonEventSubscribeInfo info(matchingSkills);
    if (g_eventHandler == nullptr) {
        g_eventHandler = std::shared_ptr<SystemEventHandler>(
            new (std::nothrow) SystemEventHandler(info, eventCallBack));
        if (g_eventHandler == nullptr) {
            LOGE("[FATAL]SAF system event handler is nullptr.");
            return false;
        }
    }

    return CommonEventManager::SubscribeCommonEvent(g_eventHandler);
}

bool UnSubscribeSystemEvent(void)
{
    if (g_eventHandler == nullptr) {
        LOGW("SAF system event handler is nullptr, no need to unsubscribe.");
        return false;
    }

    bool res = CommonEventManager::UnSubscribeCommonEvent(g_eventHandler);
    g_eventHandler = nullptr;
    return res;
}
