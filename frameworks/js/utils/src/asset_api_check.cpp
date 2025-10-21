/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <vector>

#include "securec.h"

#include "asset_log.h"
#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_api_check.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {

#define MIN_ARRAY_SIZE 0
#define MAX_SECRET_SIZE 1024
#define MAX_ALIAS_SIZE 256
#define MIN_NUMBER_VALUE 0
#define MAX_AUTH_VALID_PERIOD 600
#define CHALLENGE_SIZE 32
#define AUTH_TOKEN_SIZE 280
#define MAX_LABEL_SIZE 2048
#define MAX_RETURN_LIMIT 0x10000
#define SYNC_TYPE_MIN_BITS 0
#define SYNC_TYPE_MAX_BITS 3
#define ROOT_USER_UPPERBOUND 99
#define MAX_TIME_SIZE 1024
#define SYSTEM_USER_ID_MAX 99
#define BINARY_BASE 2
#define MIN_GROUP_ID_SIZE 7
#define MAX_GROUP_ID_SIZE 127

const std::vector<uint32_t> ADD_REQUIRED_TAGS = {
    SEC_ASSET_TAG_SECRET,
    SEC_ASSET_TAG_ALIAS
};

const std::vector<uint32_t> ADD_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_SECRET,
    SEC_ASSET_TAG_CONFLICT_RESOLUTION
};

const std::vector<uint32_t> POST_QUERY_REQUIRED_TAGS = {
    SEC_ASSET_TAG_AUTH_CHALLENGE
};

const std::vector<uint32_t> POST_QUERY_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_GROUP_ID,
    SEC_ASSET_TAG_USER_ID
};

const std::vector<uint32_t> PRE_QUERY_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD
};

const std::vector<uint32_t> QUERY_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_RETURN_LIMIT,
    SEC_ASSET_TAG_RETURN_OFFSET,
    SEC_ASSET_TAG_RETURN_ORDERED_BY,
    SEC_ASSET_TAG_RETURN_TYPE,
    SEC_ASSET_TAG_AUTH_TOKEN,
    SEC_ASSET_TAG_AUTH_CHALLENGE,
};

const std::vector<uint32_t> QUERY_REQUIRED_TAGS = {
    SEC_ASSET_TAG_ALIAS
};

const std::vector<uint32_t> UPDATE_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_SECRET
};

const std::vector<uint32_t> QUERY_SYNC_RESULT_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_GROUP_ID,
    SEC_ASSET_TAG_REQUIRE_ATTR_ENCRYPTED,
};

const std::vector<uint32_t> CRITICAL_LABEL_TAGS = {
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4
};

const std::vector<uint32_t> NORMAL_LABEL_TAGS = {
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_4
};

const std::vector<uint32_t> NORMAL_LOCAL_LABEL_TAGS = {
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4
};

const std::vector<uint32_t> ACCESS_CONTROL_TAGS = {
    SEC_ASSET_TAG_ALIAS,
    SEC_ASSET_TAG_ACCESSIBILITY,
    SEC_ASSET_TAG_AUTH_TYPE,
    SEC_ASSET_TAG_IS_PERSISTENT,
    SEC_ASSET_TAG_SYNC_TYPE,
    SEC_ASSET_TAG_REQUIRE_PASSWORD_SET,
    SEC_ASSET_TAG_REQUIRE_ATTR_ENCRYPTED,
    SEC_ASSET_TAG_GROUP_ID,
    SEC_ASSET_TAG_WRAP_TYPE,
    SEC_ASSET_TAG_USER_ID
};

const std::vector<uint32_t> ASSET_RETURN_ORDER_BY_TAGS = {
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_4,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4
};

const std::vector<uint32_t> ASSET_SYNC_TAGS = {
    SEC_ASSET_TAG_OPERATION_TYPE
};

const std::vector<uint32_t> ASSET_ACCESSIBILITY_VEC = {
    SEC_ASSET_ACCESSIBILITY_DEVICE_POWERED_ON,
    SEC_ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED,
    SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED
};

const std::vector<uint32_t> ASSET_AUTH_TYPE_VEC = {
    SEC_ASSET_AUTH_TYPE_NONE,
    SEC_ASSET_AUTH_TYPE_ANY
};

const std::vector<uint32_t> ASSET_CONFLICT_RESOLUTION_VEC = {
    SEC_ASSET_CONFLICT_OVERWRITE,
    SEC_ASSET_CONFLICT_THROW_ERROR
};

const std::vector<uint32_t> ASSET_RETURN_TYPE_VEC = {
    SEC_ASSET_RETURN_ALL,
    SEC_ASSET_RETURN_ATTRIBUTES
};

const std::vector<uint32_t> ASSET_WRAP_TYPE_VEC = {
    SEC_ASSET_WRAP_TYPE_NEVER,
    SEC_ASSET_WRAP_TYPE_TRUSTED_ACCOUNT
};

bool CheckArraySize(const AssetAttr &attr, uint32_t min, uint32_t max, std::function<void(char *)> throwPtr)
{
    if (attr.value.blob.size > max || attr.value.blob.size <= min) {
        API_THROW_INVALID_ARGUMENT(throwPtr, "Value byte length[%u] of tag[asset.Tag.%s] is out of range[%u, %u].",
            attr.value.blob.size, TAG_MAP.at(attr.tag),  min + 1, max);
        return false;
    }
    return true;
}

bool CheckEnumVariant(const AssetAttr &attr, const std::vector<uint32_t> &enumVec, std::function<void(char *)> throwPtr)
{
    auto it = std::find(enumVec.begin(), enumVec.end(), attr.value.u32);
    if (it == enumVec.end()) {
        API_THROW_INVALID_ARGUMENT(throwPtr, "Value[%u] of tag[asset.Tag.%s] is an illegal enumeration variant.",
            attr.value.u32, TAG_MAP.at(attr.tag));
        return false;
    }
    return true;
}

bool CheckNumberRange(const AssetAttr &attr, uint32_t min, uint32_t max, std::function<void(char *)> throwPtr)
{
    if (attr.value.u32 > max || attr.value.u32 <= min) {
        API_THROW_INVALID_ARGUMENT(throwPtr, "Value[%u] of tag[asset.Tag.%s] is out of range[%u, %u].",
            attr.value.u32, TAG_MAP.at(attr.tag), min, max);
        return false;
    }
    return true;
}

bool CheckValidBits(const AssetAttr &attr, uint32_t minBits, uint32_t maxBits, std::function<void(char *)> throwPtr)
{
    if (attr.value.u32 >= pow(BINARY_BASE, maxBits) || attr.value.u32 < pow(BINARY_BASE, minBits) - 1) {
        API_THROW_INVALID_ARGUMENT(throwPtr, "Value[%u] of tag[asset.Tag.%s] has bit count out of range[%u, %u].",
            attr.value.u32, TAG_MAP.at(attr.tag), minBits + 1, maxBits);
        return false;
    }
    return true;
}

bool CheckTagRange(const AssetAttr &attr, const std::vector<uint32_t> &tags, std::function<void(char *)> throwPtr)
{
    auto it = std::find(tags.begin(), tags.end(), attr.value.u32);
    if (it == tags.end()) {
        API_THROW_INVALID_ARGUMENT(throwPtr, "Value[0x%X] of tag[asset.Tag.(%s)] is not tags allowed for sorting, "
            "which should start with \"DATA_LABEL\".", attr.value.u32, TAG_MAP.at(attr.tag));
        return false;
    }
    return true;
}

struct CheckContinuousRange {
    std::function<bool(const AssetAttr &, uint32_t, uint32_t, std::function<void(char *)>)> funcPtr;
    uint32_t min;
    uint32_t max;
};

const std::unordered_map<uint32_t, CheckContinuousRange> CHECK_CONTINOUS_RANGE_FUNC_MAP = {
    { SEC_ASSET_TAG_SECRET, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_SECRET_SIZE } },
    { SEC_ASSET_TAG_ALIAS, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_ALIAS_SIZE } },
    { SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, { &CheckNumberRange, MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD } },
    { SEC_ASSET_TAG_AUTH_CHALLENGE, { &CheckArraySize, CHALLENGE_SIZE - 1, CHALLENGE_SIZE } },
    { SEC_ASSET_TAG_AUTH_TOKEN, { &CheckArraySize, AUTH_TOKEN_SIZE, AUTH_TOKEN_SIZE } },
    { SEC_ASSET_TAG_SYNC_TYPE, { &CheckValidBits, SYNC_TYPE_MIN_BITS, SYNC_TYPE_MAX_BITS } },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_LABEL_SIZE } },
    { SEC_ASSET_TAG_RETURN_LIMIT, { &CheckNumberRange, MIN_NUMBER_VALUE, MAX_RETURN_LIMIT } },
    { SEC_ASSET_TAG_GROUP_ID, { &CheckArraySize, MIN_GROUP_ID_SIZE, MAX_GROUP_ID_SIZE } },
    { SEC_ASSET_TAG_USER_ID, { &CheckNumberRange, ROOT_USER_UPPERBOUND, INT32_MAX } },
    { SEC_ASSET_TAG_UPDATE_TIME, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_TIME_SIZE } }
};

struct CheckDiscreteRange {
    std::function<bool(const AssetAttr &, const std::vector<uint32_t> &, std::function<void(char *)>)> funcPtr;
    const std::vector<uint32_t> validRange;
};

const std::unordered_map<uint32_t, CheckDiscreteRange> CHECK_DISCRETE_RANGE_FUNC_MAP = {
    { SEC_ASSET_TAG_ACCESSIBILITY, { &CheckEnumVariant, ASSET_ACCESSIBILITY_VEC } },
    { SEC_ASSET_TAG_AUTH_TYPE, { &CheckEnumVariant, ASSET_AUTH_TYPE_VEC } },
    { SEC_ASSET_TAG_CONFLICT_RESOLUTION, { &CheckEnumVariant, ASSET_CONFLICT_RESOLUTION_VEC } },
    { SEC_ASSET_TAG_RETURN_TYPE, { &CheckEnumVariant, ASSET_RETURN_TYPE_VEC } },
    { SEC_ASSET_TAG_RETURN_ORDERED_BY, { &CheckTagRange, ASSET_RETURN_ORDER_BY_TAGS } },
    { SEC_ASSET_TAG_WRAP_TYPE, { &CheckEnumVariant, ASSET_WRAP_TYPE_VEC } }
};

} // anonymous namespace

bool CheckAssetRequiredTag(const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &requiredTags, std::function<void(char *)> throwPtr)
{
    for (uint32_t requiredTag : requiredTags) {
        auto it = std::find_if(attrs.begin(), attrs.end(), [requiredTag](const AssetAttr &attr) {
            return attr.tag == requiredTag;
        });
        if (it == attrs.end()) {
            API_THROW_INVALID_ARGUMENT(throwPtr, "Missing required tag[asset.Tag.%s].", TAG_MAP.at(requiredTag));
            return false;
        }
    }
    return true;
}

bool CheckAssetTagValidity(const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &validTags, std::function<void(char *)> throwPtr)
{
    for (AssetAttr attr : attrs) {
        if (std::count(validTags.begin(), validTags.end(), attr.tag) == 0) {
            API_THROW_INVALID_ARGUMENT(throwPtr, "Unsupported tag[asset.Tag.%s] for the function.",
                TAG_MAP.at(attr.tag));
            return false;
        }
    }
    return true;
}

bool CheckAssetValueValidity(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    return std::all_of(attrs.begin(), attrs.end(), [throwPtr](const AssetAttr &attr) {
        if (CHECK_CONTINOUS_RANGE_FUNC_MAP.find(attr.tag) != CHECK_CONTINOUS_RANGE_FUNC_MAP.end()) {
            auto funcPtr = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag).funcPtr;
            uint32_t min = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag).min;
            uint32_t max = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag).max;
            return funcPtr(attr, min, max, throwPtr);
        }
        if (CHECK_DISCRETE_RANGE_FUNC_MAP.find(attr.tag) != CHECK_DISCRETE_RANGE_FUNC_MAP.end()) {
            auto funcPtr = CHECK_DISCRETE_RANGE_FUNC_MAP.at(attr.tag).funcPtr;
            auto validRangePtr = CHECK_DISCRETE_RANGE_FUNC_MAP.at(attr.tag).validRange;
            return funcPtr(attr, validRangePtr, throwPtr);
        }
        return true;
        });
}

bool CheckAssetPresence(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    if (attrs.empty()) {
        API_THROW_INVALID_ARGUMENT(throwPtr, "Argument[attributesToUpdate] is empty.");
        return false;
    }
    return true;
}

int32_t CheckAddArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    IF_FALSE_RETURN(CheckAssetRequiredTag(attrs, ADD_REQUIRED_TAGS, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    validTags.insert(validTags.end(), ADD_OPTIONAL_TAGS.begin(), ADD_OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(attrs, validTags, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    return SEC_ASSET_SUCCESS;
}

int32_t CheckPostQueryArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    IF_FALSE_RETURN(CheckAssetRequiredTag(attrs, POST_QUERY_REQUIRED_TAGS, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), POST_QUERY_REQUIRED_TAGS.begin(), POST_QUERY_REQUIRED_TAGS.end());
    validTags.insert(validTags.end(), POST_QUERY_OPTIONAL_TAGS.begin(), POST_QUERY_OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    return SEC_ASSET_SUCCESS;
}

int32_t CheckPreQueryArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), PRE_QUERY_OPTIONAL_TAGS.begin(), PRE_QUERY_OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(attrs, validTags, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    return SEC_ASSET_SUCCESS;
}

int32_t CheckQueryArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    validTags.insert(validTags.end(), QUERY_OPTIONAL_TAGS.begin(), QUERY_OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(attrs, validTags, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    return SEC_ASSET_SUCCESS;
}

int32_t CheckRemoveArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(attrs, validTags, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    return SEC_ASSET_SUCCESS;
}

int32_t CheckUpdateArgs(const std::vector<AssetAttr> &attrs, const std::vector<AssetAttr> &updateAttrs,
    std::function<void(char *)> throwPtr)
{
    IF_FALSE_RETURN(CheckAssetRequiredTag(attrs, QUERY_REQUIRED_TAGS, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    std::vector<uint32_t> queryValidTags;
    queryValidTags.insert(queryValidTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(attrs, queryValidTags, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);

    IF_FALSE_RETURN(CheckAssetPresence(updateAttrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);
    std::vector<uint32_t> updateValidTags;
    updateValidTags.insert(updateValidTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), UPDATE_OPTIONAL_TAGS.begin(), UPDATE_OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(updateAttrs, updateValidTags, throwPtr),
        SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(updateAttrs, throwPtr), SEC_ASSET_INVALID_ARGUMENT);

    return SEC_ASSET_SUCCESS;
}

int32_t CheckQuerySyncResultArgs(const std::vector<AssetAttr> &attrs, std::function<void(char *)> throwPtr)
{
    IF_FALSE_RETURN(CheckAssetTagValidity(attrs, QUERY_SYNC_RESULT_OPTIONAL_TAGS, throwPtr),
        SEC_ASSET_INVALID_ARGUMENT);
    IF_FALSE_RETURN(CheckAssetValueValidity(attrs, throwPtr), SEC_ASSET_PARAM_VERIFICATION_FAILED);

    return SEC_ASSET_SUCCESS;
}

} // Asset
} // Security
} // OHOS
