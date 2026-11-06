/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "settings_observer.h"

#include "datashare_helper.h"
#include "iservice_registry.h"
#include "uri.h"

#include "saf_log.h"

namespace OHOS::Security::SAF {

namespace {
constexpr int32_t SAF_SERVICE_ID = 66532;
const constexpr char *SETTING_URI_PROXY_PREFIX =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_";
const constexpr char *SETTING_URI_PROXY_SUFFIX = "?Proxy=true";
const constexpr char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";

std::string BuildSettingUri(int32_t userId, const std::string &key)
{
    return std::string(SETTING_URI_PROXY_PREFIX) + std::to_string(userId) +
        std::string(SETTING_URI_PROXY_SUFFIX) + "&key=" + key;
}

std::shared_ptr<OHOS::DataShare::DataShareHelper> CreateDataShareHelper(int32_t userId)
{
    std::string baseUri = std::string(SETTING_URI_PROXY_PREFIX) + std::to_string(userId) +
        std::string(SETTING_URI_PROXY_SUFFIX);
    auto samgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOGE("CreateDataShareHelper: GetSystemAbilityManager failed");
        return nullptr;
    }
    auto remoteObj = samgr->GetSystemAbility(SAF_SERVICE_ID);
    if (remoteObj == nullptr) {
        LOGE("CreateDataShareHelper: GetSystemAbility(%{public}d) failed", SAF_SERVICE_ID);
        return nullptr;
    }
    auto [ret, helper] = OHOS::DataShare::DataShareHelper::Create(remoteObj, baseUri, SETTINGS_DATA_EXT_URI);
    if (helper == nullptr || ret != OHOS::ERR_OK) {
        LOGE("CreateDataShareHelper: DataShareHelper::Create failed, ret=%{public}d", ret);
        return nullptr;
    }
    return helper;
}
} // namespace

void SettingsDataObserver::OnChange()
{
    if (callback_) {
        callback_();
    }
}

bool RegisterSettingsObserver(const std::string &key, int32_t userId,
    const sptr<SettingsDataObserver> &observer)
{
    if (observer == nullptr) {
        LOGE("RegisterSettingsObserver: observer is null");
        return false;
    }

    auto helper = CreateDataShareHelper(userId);
    if (helper == nullptr) {
        LOGE("RegisterSettingsObserver: CreateDataShareHelper failed for userId=%{public}d", userId);
        return false;
    }
    std::string uriStr = BuildSettingUri(userId, key);
    OHOS::Uri settingsUri(uriStr);
    LOGI("RegisterSettingsObserver: uri=%{public}s", uriStr.c_str());
    int32_t ret = helper->RegisterObserver(settingsUri, observer);
    helper->Release();
    if (ret != OHOS::ERR_OK) {
        LOGE("RegisterSettingsObserver: RegisterObserver failed, ret=%{public}d", ret);
        return false;
    }
    LOGI("RegisterSettingsObserver: success, key=%{public}s", key.c_str());
    return true;
}

bool UnregisterSettingsObserver(const std::string &key, int32_t userId,
    const sptr<SettingsDataObserver> &observer)
{
    if (observer == nullptr) {
        LOGE("UnregisterSettingsObserver: observer is null");
        return false;
    }

    auto helper = CreateDataShareHelper(userId);
    if (helper == nullptr) {
        LOGE("UnregisterSettingsObserver: CreateDataShareHelper failed for userId=%{public}d", userId);
        return false;
    }
    std::string uriStr = BuildSettingUri(userId, key);
    OHOS::Uri settingsUri(uriStr);
    int32_t ret = helper->UnregisterObserver(settingsUri, observer);
    helper->Release();
    if (ret != OHOS::ERR_OK) {
        LOGE("UnregisterSettingsObserver: UnregisterObserver failed, ret=%{public}d", ret);
        return false;
    }
    LOGI("UnregisterSettingsObserver: success, key=%{public}s", key.c_str());
    return true;
}

} // namespace OHOS::Security::SAF
