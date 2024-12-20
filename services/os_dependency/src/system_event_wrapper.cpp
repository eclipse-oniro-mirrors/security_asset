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
const char * const COMMON_EVENT_RESTORE_START = "usual.event.RESTORE_START";
const char * const COMMON_EVENT_USER_PIN_CREATED = "USER_PIN_CREATED_EVENT";
const char * const BUNDLE_NAME = "bundleName";
const char * const PERMISSION_MANAGE_USER_IDM = "ohos.permission.MANAGE_USER_IDM";
const char * const DEVELOPER_ID = "developerId";
const char * const GROUP_IDS = "assetAccessGroups";
const char * const OWNER_INFO_SEPARATOR = "_";
const char * const GROUP_SEPARATOR = ",";

void HandlePackageRemoved(const OHOS::AAFwk::Want &want, bool isSandBoxApp, OnPackageRemoved onPackageRemoved)
{
    // parse user id from want
    int userId = want.GetIntParam(USER_ID, INVALID_USERID);
    // parse app id from want
    std::string appId = want.GetStringParam(APP_ID);
    // parse app index from want
    int appIndex = isSandBoxApp ? want.GetIntParam(SANDBOX_APP_INDEX, -1) : 0;
    if (appId.empty() || userId == INVALID_USERID || appIndex == -1) {
        LOGE("[FATAL]Get removed owner info failed, userId=%{public}d, appId=%{public}s, appIndex=%{public}d",
            userId, appId.c_str(), appIndex);
        return;
    }
    std::string owner = appId + OWNER_INFO_SEPARATOR + std::to_string(appIndex);
    ConstAssetBlob ownerBlob = { .size = owner.size(), .data = reinterpret_cast<const uint8_t *>(owner.c_str()) };

    // parse bundle name from want
    std::string bundleName = want.GetBundle();
    // parse groups from want
    std::string developerId = want.GetStringParam(DEVELOPER_ID);
    ConstAssetBlob developerIdBlob = { .size = developerId.size(),
        .data = reinterpret_cast<const uint8_t *>(developerId.c_str()) };
    std::string groupIdsStr = want.GetStringParam(GROUP_IDS);
    std::vector<ConstAssetBlob> groupIds;
    if (!developerId.empty() && !groupIds.empty()) {
        if (appIndex != 0) {
            LOGE("[FATAL]App with non-zero app index is not allowed to access groups, appIndex=%{public}d", appIndex);
            return;
        }
        size_t start = 0, end;
        while ((end = groupIdsStr.find(GROUP_SEPARATOR, start)) != std::string::npos) {
            std::string groupId = groupIdsStr.substr(start, end - start);
            groupIds.push_back({ .size = groupId.size(), .data = reinterpret_cast<const uint8_t *>(groupId.c_str()) });
            start = end;
        }
        std::string groupId = groupIdsStr.substr(start, end);
        groupIds.push_back({ .size = groupId.size(), .data = reinterpret_cast<const uint8_t *>(groupId.c_str()) });
    }
    ConstAssetBlobArray groupIdsBlobArray = { .size = groupIds.size(),
        .blob = reinterpret_cast<const ConstAssetBlob *>(&groupIds[0]) };

    if (onPackageRemoved != nullptr) {
        onPackageRemoved({ userId, appIndex, ownerBlob, developerIdBlob, groupIdsBlobArray,
            reinterpret_cast<const uint8_t *>(bundleName.c_str()) });
    }

    LOGI("[INFO]Receive event: PACKAGE_REMOVED, userId=%{public}d, appId=%{public}s, appIndex=%{public}d, ", userId,
        appId.c_str(), appIndex);
}

void HandleAppRestore(const OHOS::AAFwk::Want &want, OnAppRestore onAppRestore)
{
    if (onAppRestore != nullptr) {
        int userId = want.GetIntParam(USER_ID, INVALID_USERID);
        std::string bundleName = want.GetStringParam(BUNDLE_NAME);

        int appIndex = want.GetIntParam(SANDBOX_APP_INDEX, -1);
        if (appIndex == -1) {
            LOGI("[INFO]Get app restore info failed, default as index 0.");
            appIndex = 0;
        }

        onAppRestore(userId, reinterpret_cast<const uint8_t *>(bundleName.c_str()), appIndex);
        LOGI("[INFO]Receive event: RESTORE_START.");
    }
}

class SystemEventHandler : public CommonEventSubscriber {
public:
    explicit SystemEventHandler(const CommonEventSubscribeInfo &subscribeInfo, const EventCallBack eventCallBack)
        : CommonEventSubscriber(subscribeInfo), eventCallBack(eventCallBack) {}
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const CommonEventData &data) override
    {
        long startTime = std::clock();
        auto want = data.GetWant();
        std::string action = want.GetAction();
        if (action == CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
            HandlePackageRemoved(want, false, this->eventCallBack.onPackageRemoved);
        } else if (action == CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
            HandlePackageRemoved(want, true, this->eventCallBack.onPackageRemoved);
        } else if (action == CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            int userId = data.GetCode();
            if (this->eventCallBack.onUserRemoved != nullptr) {
                this->eventCallBack.onUserRemoved(userId);
            }
            LOGI("[INFO] Receive event: USER_REMOVED, userId=%{public}d", userId);
        } else if (action == CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
            if (this->eventCallBack.onScreenOff != nullptr) {
                this->eventCallBack.onScreenOff();
            }
            LOGI("[INFO]Receive event: SCREEN_OFF, start_time: %{public}ld", startTime);
        } else if (action == CommonEventSupport::COMMON_EVENT_CHARGING) {
            if (this->eventCallBack.onCharging != nullptr) {
                this->eventCallBack.onCharging();
            }
            LOGI("[INFO]Receive event: CHARGING, start_time: %{public}ld", startTime);
        } else if (action == COMMON_EVENT_RESTORE_START) {
            HandleAppRestore(want, this->eventCallBack.onAppRestore);
        } else if (action == CommonEventSupport::COMMON_EVENT_USER_UNLOCKED) {
            if (this->eventCallBack.onUserUnlocked != nullptr) {
                int userId = data.GetCode();
                this->eventCallBack.onUserUnlocked(userId);
            }
            LOGI("[INFO]Receive event: USER_UNLOCKED, start_time: %{public}ld", startTime);
        } else if (action == COMMON_EVENT_USER_PIN_CREATED) {
            if (this->eventCallBack.onUserUnlocked != nullptr) {
                int userId = data.GetCode();
                this->eventCallBack.onUserUnlocked(userId);
            }
            LOGI("[INFO]Receive event: USER_PIN_CREATED_EVENT, start_time: %{public}ld", startTime);
        } else {
            LOGW("[WARNING]Receive unknown event: %{public}s", action.c_str());
        }
    }
private:
    const EventCallBack eventCallBack;
};

std::shared_ptr<SystemEventHandler> g_eventHandler = nullptr;
std::shared_ptr<SystemEventHandler> g_pinEventHandler = nullptr;
bool SubscribePinEvent(const EventCallBack eventCallBack)
{
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(COMMON_EVENT_USER_PIN_CREATED);
    CommonEventSubscribeInfo info(matchingSkills);
    info.SetPermission(PERMISSION_MANAGE_USER_IDM);
    if (g_pinEventHandler == nullptr) {
        g_pinEventHandler = std::shared_ptr<SystemEventHandler>(
            new (std::nothrow) SystemEventHandler(info, eventCallBack));
        if (g_pinEventHandler == nullptr) {
            LOGE("[FATAL]Asset pin event handler is nullptr.");
            return false;
        }
    }

    return CommonEventManager::SubscribeCommonEvent(g_pinEventHandler);
}

bool UnSubscribePinEvent(void)
{
    if (g_pinEventHandler == nullptr) {
        LOGW("Asset pin event handler is nullptr, no need to unsubscribe.");
        return false;
    }

    bool res = CommonEventManager::UnSubscribeCommonEvent(g_pinEventHandler);
    g_pinEventHandler = nullptr;
    return res;
}

}

bool SubscribeSystemEvent(const EventCallBack eventCallBack)
{
    bool ret = SubscribePinEvent(eventCallBack);
    LOGI("Subscribe pin event result: %d", ret);

    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_CHARGING);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_UNLOCKED);
    matchingSkills.AddEvent(COMMON_EVENT_RESTORE_START);
    CommonEventSubscribeInfo info(matchingSkills);
    if (g_eventHandler == nullptr) {
        g_eventHandler = std::shared_ptr<SystemEventHandler>(
            new (std::nothrow) SystemEventHandler(info, eventCallBack));
        if (g_eventHandler == nullptr) {
            LOGE("[FATAL]Asset system event handler is nullptr.");
            return false;
        }
    }

    return CommonEventManager::SubscribeCommonEvent(g_eventHandler);
}

bool UnSubscribeSystemEvent(void)
{
    bool ret = UnSubscribePinEvent();
    LOGI("UnSubscribe pin event result: %d", ret);

    if (g_eventHandler == nullptr) {
        LOGW("Asset system event handler is nullptr, no need to unsubscribe.");
        return false;
    }

    bool res = CommonEventManager::UnSubscribeCommonEvent(g_eventHandler);
    g_eventHandler = nullptr;
    return res;
}
