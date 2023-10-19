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
    int32_t delete_hap_asset(int32_t user_id, const char* owner); // todo 切换命名
    void delete_user_asset(int32_t user_id);
}

namespace {
class SystemEventHandler : public OHOS::EventFwk::CommonEventSubscriber {
public:
    SystemEventHandler(const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo) :
        OHOS::EventFwk::CommonEventSubscriber(subscribeInfo) {
            LOGE("SystemEventHandler constructor"); // todo: delete
        }
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override
    {
        auto want = data.GetWant();
        std::string action = want.GetAction();
        LOGE("receive event!!!!!");  // todo 要删掉
        if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED ||
            action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
            // get userId
            int uid = want.GetIntParam(OHOS::AppExecFwk::Constants::UID, -1);
            int userId = -1;
            OHOS::AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userId); // todo: 从want中获取
            LOGE("userId %{public}i", userId);  // todo 要删掉
            // get appId
            const char *APP_ID = "appId";
            std::string appId = want.GetStringParam(APP_ID);
            LOGE("appId %{public}s", appId.c_str());  // todo 要删掉

            int appIndex = 0;
            if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
                appIndex = want.GetIntParam(OHOS::AppExecFwk::Constants::SANDBOX_APP_INDEX, -1);
                if (appIndex < 0) {
                    LOGE("sandbox package appIndex = %{public}d is invalid.", appIndex);
                    return;
                }
            }
            if (appId.empty() || userId == -1) {
                LOGE("get wrong appId/userId");
                return;
            }

            LOGE("appIndex %{public}i", appIndex);  // todo 要删掉
            std::string owner = appId + '_' + std::to_string(appIndex);
            int totalDeleteNum = delete_hap_asset(userId, owner.c_str());
            LOGI("delete finish! total delete line: %{public}i", totalDeleteNum);  // todo 要删掉
            // TODO: 增加判断os_account
            // do DeleteByAppID
        } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            int userId = data.GetCode();
            LOGE("AssetService user removed: userId is %{public}i", userId);  // todo 要删掉
            // delete data
            delete_user_asset(userId);  // todo 这里直接把user下对应的文件夹删除了 谨慎使用
        } // todo: 监听锁屏广播，中止session
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

    g_eventHandler = std::make_shared<SystemEventHandler>(subscriberInfo);
    if (g_eventHandler == nullptr) {
        LOGE("[FATAL]Asset system event handler is nullptr.");
        return false;
    }

    LOGE("register sub system event!");  // todo 要删掉
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
