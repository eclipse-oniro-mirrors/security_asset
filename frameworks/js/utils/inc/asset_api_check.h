/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ASSET_API_CHECK_H
#define ASSET_API_CHECK_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

#define MAX_MESSAGE_LEN 256

#define IF_FALSE_RETURN(result, returnValue)    \
if (!(result)) {                                \
    return (returnValue);                       \
}

#define API_THROW_INVALID_ARGUMENT(throwPtr, format, arg...)                                        \
do {                                                                                                \
    char msg[MAX_MESSAGE_LEN] = { 0 };                                                              \
    if ((sprintf_s(msg, MAX_MESSAGE_LEN, format, ##arg)) == -1) {                                   \
        LOGE("[FATAL][API]Failed to create message string, truncation occurred when sprintf_s.");   \
        break;                                                                                      \
    }                                                                                               \
    LOGE("[FATAL][API]%{public}s", (msg));                                                          \
    if (throwPtr != nullptr) {                                                                      \
        throwPtr(msg);                                                                              \
    }                                                                                               \
} while (0)

const std::unordered_map<uint32_t, const char *> TAG_MAP = {
    { SEC_ASSET_TAG_SECRET, "SECRET" },
    { SEC_ASSET_TAG_ALIAS, "ALIAS" },
    { SEC_ASSET_TAG_ACCESSIBILITY, "ACCESSIBILITY" },
    { SEC_ASSET_TAG_REQUIRE_PASSWORD_SET, "REQUIRE_PASSWORD_SET" },
    { SEC_ASSET_TAG_AUTH_TYPE, "AUTH_TYPE" },
    { SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, "AUTH_VALIDITY_PERIOD" },
    { SEC_ASSET_TAG_AUTH_CHALLENGE, "AUTH_CHALLENGE" },
    { SEC_ASSET_TAG_AUTH_TOKEN, "AUTH_TOKEN" },
    { SEC_ASSET_TAG_SYNC_TYPE, "SYNC_TYPE" },
    { SEC_ASSET_TAG_IS_PERSISTENT, "IS_PERSISTENT" },
    { SEC_ASSET_TAG_CONFLICT_RESOLUTION, "CONFLICT_RESOLUTION" },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, "DATA_LABEL_CRITICAL_1" },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, "DATA_LABEL_CRITICAL_2" },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, "DATA_LABEL_CRITICAL_3" },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, "DATA_LABEL_CRITICAL_4" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, "DATA_LABEL_NORMAL_1" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, "DATA_LABEL_NORMAL_2" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, "DATA_LABEL_NORMAL_3" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, "DATA_LABEL_NORMAL_4" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1, "DATA_LABEL_NORMAL_LOCAL_1" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2, "DATA_LABEL_NORMAL_LOCAL_2" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3, "DATA_LABEL_NORMAL_LOCAL_3" },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4, "DATA_LABEL_NORMAL_LOCAL_4" },
    { SEC_ASSET_TAG_RETURN_TYPE, "RETURN_TYPE" },
    { SEC_ASSET_TAG_RETURN_LIMIT, "RETURN_LIMIT" },
    { SEC_ASSET_TAG_RETURN_OFFSET, "RETURN_OFFSET" },
    { SEC_ASSET_TAG_RETURN_ORDERED_BY, "RETURN_ORDERED_BY" },
    { SEC_ASSET_TAG_UPDATE_TIME, "UPDATE_TIME" },
    { SEC_ASSET_TAG_OPERATION_TYPE, "OPERATION_TYPE" },
    { SEC_ASSET_TAG_REQUIRE_ATTR_ENCRYPTED, "REQUIRE_ATTR_ENCRYPTED" },
    { SEC_ASSET_TAG_GROUP_ID, "GROUP_ID" },
    { SEC_ASSET_TAG_WRAP_TYPE, "WRAP_TYPE" },
    { SEC_ASSET_TAG_USER_ID, "USER_ID" },
};

bool CheckAssetRequiredTag(const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &requiredTags, std::function<void(char *)> throwPtr);

bool CheckAssetTagValidity(const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &validTags, std::function<void(char *)> throwPtr);

bool CheckAssetValueValidity(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

bool CheckAssetPresence(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

int32_t CheckAddArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

int32_t CheckPostQueryArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

int32_t CheckPreQueryArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

int32_t CheckQueryArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

int32_t CheckRemoveArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

int32_t CheckUpdateArgs(const std::vector<AssetAttr> &attrs, const std::vector<AssetAttr> &updateAttrs,
    std::function<void(char *)> throwPtr);

int32_t CheckQuerySyncResultArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr);

} // Asset
} // Security
} // OHOS

#endif // ASSET_API_CHECK_H