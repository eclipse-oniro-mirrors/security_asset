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

#ifndef SYSTEM_EVENT_WRAPPER
#define SYSTEM_EVENT_WRAPPER

#include "common_event_manager.h"
#include "common_event_subscriber.h"

namespace OHOS {
namespace Security {
namespace Asset {
class SystemEventHandler : public OHOS::EventFwk::CommonEventSubscriber {
public:
    explicit SystemEventHandler(const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo);
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override;
};

class SystemEventManager {
public:
    SystemEventManager() = default;
    ~~SystemEventManager();
    static bool SubscribeSystemEvent();
    static bool UnSubscribeSystemEvent();

private:
    static std::shared_ptr<SystemEventHandler> systemEventHandler_;
};
} // namespace Asset
} // namespace Security
} // namespace OHOS

#endif // SYSTEM_EVENT_WRAPPER_H
