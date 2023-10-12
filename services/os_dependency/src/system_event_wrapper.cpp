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
int32_t delete_hap_asset(int32_t user_id, const char* owner, uint32_t auth_type, uint32_t access_type);
void delete_user_asset(int32_t user_id);
}



namespace {
class SystemEventHandler : public OHOS::EventFwk::CommonEventSubscriber {
public:
    SystemEventHandler(const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo) :
        OHOS::EventFwk::CommonEventSubscriber(subscribeInfo) {
            LOGE("SystemEventHandler constructor");
        }
    ~SystemEventHandler() = default;
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override
    {
        auto want = data.GetWant();
        std::string action = want.GetAction();
        LOGE("receive event!!!!!");  // todo 要删掉
        if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED ||
            action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
            // 1. 测试主应用卸载时，能否获取到userID, appId（最新开发分支版本）, appIndex（默认为0）
            // int userId = want.GetIntParam(OHOS::AppExecFwk::Constants::USER_ID, -1);
            // get userId(use front userId)
            std::vector<int> ids;
            int ret = OHOS::AccountSA::OsAccountManager::QueryActiveOsAccountIds(ids);
            if (ret != 0 || ids.empty()) {
                LOGE("QueryActiveOsAccountIds Failed!! ret = %" LOG_PUBLIC "d", ret);
                return;
            }
            int userId = ids[0];
            LOGE("userId %{public}i", userId);  // todo 要删掉

            // get APPID todo 这里需要等到能获取到appId的时候再操作
            const char *APP_ID = "appId";
            std::string appId = want.GetStringParam(APP_ID);
            LOGE("appId %{public}s", appId.c_str());  // todo 要删掉

            if (appId.empty() || userId == -1) {
                LOGE("get wrong appId/userId");
                return;
            }
            // todo 获取 appIndex 只有在COMMON_EVENT_SANDBOX_PACKAGE_REMOVED的时候获取 在COMMON_EVENT_PACKAGE_REMOVED的时候默认为0
            int appIndex = 0;
            LOGE("appId %{public}i", appIndex);  // todo 要删掉
            // 2. 调用数据库提供的接口，删除userID+owner(appId+appIndex)对应的数据； 删除密钥 userId+owner+accessType+authType
            // 现在已经有userId和owner accessType和authType直接遍历
            std::string owner = appId + '_' + std::to_string(appIndex);
            int totalDeleteNum = 0;
            for (int accessType = 0; accessType < 2; accessType++) {
                for (int authType = 0; authType < 2; authType++) {
                    totalDeleteNum += delete_hap_asset(userId, owner.c_str(), authType, accessType);
                }
            }
            LOGI("delete finish! total delete line: %{public}i", totalDeleteNum);  // todo 要删掉
            // 5. 测试沙箱应用卸载时，能否获取到userID, appId（最新开发分支版本可能没有）, appIndex

            // TODO: 增加判断os_account
            // do DeleteByAppID
        } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            // 3. 获取到userID, 删除数据库userId，删除密钥-（huks)
            // get userId
            std::vector<int> ids;
            int ret = OHOS::AccountSA::OsAccountManager::QueryActiveOsAccountIds(ids);
            if (ret != 0 || ids.empty()) {
                LOGE("QueryActiveOsAccountIds Failed!! ret = %" LOG_PUBLIC "d", ret);
                return;
            }
            int userId = ids[0];
            LOGE("userId %{public}i", userId);  // todo 要删掉
            // delete data
            delete_user_asset(userId);  // todo 这里直接把user下对应的文件夹删除了 谨慎使用
            // 4. 属主访问控制获取到的userID，应该是前台userID，而不是调用方的userID
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
