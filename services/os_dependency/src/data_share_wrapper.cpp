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
const constexpr char *SETTING_COLUMN_KEYWORD = "KEYWORD";
const constexpr char *SETTING_COLUMN_VALUE = "VALUE";
const constexpr char *SETTING_URI_PROXY_PREFIX = "datashare:///com.ohos.settingsdata/entry/settingsdata/"
                                          "USER_SETTINGSDATA_SECURE_";
const constexpr char *SETTING_URI_PROXY_SUFFIX = "?Proxy=true";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
constexpr const int32_t ASSET_SA_ID = 8100;
constexpr const int32_t ZERO = 0;
constexpr const int32_t ONE = 1;
constexpr const int32_t DATASHARE_SUCCESS = 0;
constexpr const int32_t DATASHARE_FAIL = -1;

std::string getUriStr(int32_t userId)
{
    return std::string(SETTING_URI_PROXY_PREFIX) + std::to_string(userId) + std::string(SETTING_URI_PROXY_SUFFIX);
}

std::shared_ptr<OHOS::DataShare::DataShareHelper> CreateDataShareHelper(int32_t userId)
{
    auto SETTING_URI_PROXY = getUriStr(userId);
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

OHOS::DataShare::DataSharePredicates getPredicates(std::string keyword)
{
    OHOS::DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_COLUMN_KEYWORD, keyword);
    return predicates;
}
} // namespace

bool StoreKeyValue(int32_t userId, char const *inKey, int32_t inValue)
{
    auto helper = CreateDataShareHelper(userId);
    if (helper == nullptr) {
        LOGE("helper is nullptr");
        return false;
    }

    auto uri = Uri(getUriStr(userId));
    std::string keyword = std::string(inKey);
    auto predicates = getPredicates(keyword);
    std::vector<std::string> columns;
    auto resultSet = helper->Query(uri, predicates, columns);
    if (resultSet == nullptr) {
        LOGE("[FATAL]Datashare query failed.");
        helper->Release();
        return false;
    }

    int32_t result = DATASHARE_FAIL;
    int32_t query_count = ZERO;
    resultSet->GetRowCount(query_count);
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SETTING_COLUMN_VALUE, inValue);
    switch (query_count) {
        case ZERO:
            valuesBucket.Put(SETTING_COLUMN_KEYWORD, keyword);
            result = helper->Insert(uri, valuesBucket);
            break;
        case ONE:
            result = helper->Update(uri, predicates, valuesBucket);
            break;
        default:
            LOGE("[FATAL]Datashare query over expected.");
            break;
    }

    if (result < ZERO) {
        LOGE("[FATAL]Datashare insert/update failed, ret=%{public}d", result);
    }
    resultSet->Close();
    helper->Release();
    return result >= ZERO;
}

int32_t QueryValue(int32_t userId, const char *inKey, int32_t *outValue)
{
    auto helper = CreateDataShareHelper(userId);
    if (helper == nullptr) {
        LOGE("helper is nullptr");
        return DATASHARE_FAIL;
    }

    auto uri = Uri(getUriStr(userId));
    auto predicates = getPredicates(std::string(inKey));
    std::vector<std::string> columns;
    auto resultSet = helper->Query(uri, predicates, columns);
    if (resultSet == nullptr) {
        LOGE("[FATAL]Datashare query failed.");
        helper->Release();
        return DATASHARE_FAIL;
    }

    int32_t result = DATASHARE_FAIL;
    int32_t query_count = ZERO;
    int value = ZERO;
    resultSet->GetRowCount(query_count);
    switch (query_count) {
        case ZERO:
            LOGE("[FATAL]Datashare query not exist.");
            break;
        case ONE:
            resultSet->GetInt(ZERO, value);
            *outValue = value;
            result = DATASHARE_SUCCESS;
            break;
        default:
            LOGE("[FATAL]Datashare query over expected.");
            break;
    }

    resultSet->Close();
    helper->Release();
    return result;
}