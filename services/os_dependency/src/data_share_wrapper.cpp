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

#include "data_share_wrapper.h"

#include <cstring>
#include "securec.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_interface.h"
#include "iservice_registry.h"

#include "asset_type.h"
#include "asset_log.h"

namespace {
const constexpr char *ASSET_CE_UPGRADE = "ASSET_CE_UPGRADE";
const constexpr char *SETTING_COLUMN_KEYWORD = "KEYWORD";
const constexpr char *SETTING_COLUMN_VALUE = "VALUE";
const constexpr char *SETTING_URI_PROXY_PREFIX = "datashare:///com.ohos.settingsdata/entry/settingsdata/"
                                          "USER_SETTINGSDATA_SECURE_";
const constexpr char *SETTING_URI_PROXY_SUFFIX = "?Proxy=true";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
constexpr const int32_t ASSET_SA_ID = 8100;

std::shared_ptr<OHOS::DataShare::DataShareHelper> CreateDataShareHelper(int32_t userId)
{
    auto SETTING_URI_PROXY = std::string(SETTING_URI_PROXY_PREFIX) + std::to_string(userId) 
        + std::string(SETTING_URI_PROXY_SUFFIX);
    auto systemAbilityManager = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        LOGE("[FATAL]systemAbilityManager is nullptr, please check.");
        return nullptr;
    }
    auto remoteObj = systemAbilityManager->GetSystemAbility(ASSET_SA_ID);
    if (!remoteObj) {
        LOGE("get sa manager return nullptr");
        return nullptr;
    }

    auto [ret, helper] = OHOS::DataShare::DataShareHelper::Create(remoteObj, SETTING_URI_PROXY, SETTINGS_DATA_EXT_URI);
    remoteObj = nullptr;
    return helper;
}

} // namespace

bool StoreUpgradeInSetting(int32_t userId)
{
    auto helper = CreateDataShareHelper(userId);
    if (helper == nullptr) {
        LOGE("helper is nullptr");
        return false;
    }
    std::string ce_upgrade = std::string(ASSET_CE_UPGRADE);
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SETTING_COLUMN_KEYWORD, ce_upgrade);
    valuesBucket.Put(SETTING_COLUMN_VALUE, true);
    auto uri = Uri(std::string(SETTING_URI_PROXY_PREFIX) + std::to_string(userId) 
        + std::string(SETTING_URI_PROXY_SUFFIX));
    int32_t result = helper->Insert(uri, valuesBucket);
    if (result != 0) {
        LOGE("[FATAL]Datashare insert failed, ret=%{public}d", result);
        helper->Release();
        return false;
    }

    helper->Release();
    return true;
}
