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
using namespace OHOS::AAFwk;

const std::vector<std::string> SYSTEM_EVENT_LIST = {
    CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED,
    CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED,
    CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED,
    CommonEventSupport::COMMON_EVENT_RESTORE_START,
};

const char** ToConstCharArray(const std::vector<std::string>& vec)
{
    std::vector<const char*> result;
    for (const auto& str : vec) {
        result.push_back(str.c_str());
    }
    static std::vector<const char*> static_result = result;
    return static_result.data();
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

        std::vector<std::string> rustWant;
        const WantParams &wantParams = want.GetParams();

        const std::map<std::string, OHOS::sptr<IInterface>> &params = wantParams.GetParams();

        for (const auto &param : params) {
            const std::string &key = param.first;
            OHOS::sptr<IInterface> value = param.second;

            int typeId = WantParams::GetDataType(value);
            std::string stringValue = WantParams::GetStringByType(value, typeId);
            rustWant.push_back(key);
            rustWant.push_back(stringValue);
        }

        if (this->eventCallBack.onCommonEvent != nullptr) {
            auto cArray = ToConstCharArray(rustWant);
            StringArray wantArray =  {
                .size = static_cast<uint32_t>(rustWant.size()),
                .data = cArray
            };
            this->eventCallBack.onCommonEvent({
                eventName.c_str(),
                wantArray
            });
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
    for (const auto& eventName : SYSTEM_EVENT_LIST) {
        matchingSkills.AddEvent(eventName);
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
