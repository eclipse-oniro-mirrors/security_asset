/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "asset_napi_check.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <vector>

#include "securec.h"

#include "asset_log.h"
#include "asset_system_type.h"

#include "asset_napi_common.h"

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
#define MAX_AUTH_TOKEN_SIZE 1024
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

napi_value CheckArraySize(const napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max, uint32_t errorCode)
{
    if (attr.value.blob.size > max || attr.value.blob.size <= min) {
        RETURN_JS_ERROR(env, errorCode, "Value byte length[%u] of tag[asset.Tag.%s] is out of range[%u, %u].",
            attr.value.blob.size, TAG_MAP.at(attr.tag),  min + 1, max);
    }
    return nullptr;
}

napi_value CheckEnumVariant(const napi_env env, const AssetAttr &attr, const std::vector<uint32_t> &enumVec,
    uint32_t errorCode)
{
    auto it = std::find(enumVec.begin(), enumVec.end(), attr.value.u32);
    if (it == enumVec.end()) {
        RETURN_JS_ERROR(env, errorCode, "Value[%u] of tag[asset.Tag.%s] is an illegal enumeration variant.",
            attr.value.u32, TAG_MAP.at(attr.tag));
    }
    return nullptr;
}

napi_value CheckNumberRange(const napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max, uint32_t errorCode)
{
    if (attr.value.u32 > max || attr.value.u32 <= min) {
        RETURN_JS_ERROR(env, errorCode, "Value[%u] of tag[asset.Tag.%s] is out of range[%u, %u].",
            attr.value.u32, TAG_MAP.at(attr.tag), min, max);
    }
    return nullptr;
}

napi_value CheckValidBits(const napi_env env, const AssetAttr &attr, uint32_t minBits, uint32_t maxBits,
    uint32_t errorCode)
{
    if (attr.value.u32 >= pow(BINARY_BASE, maxBits) || attr.value.u32 < pow(BINARY_BASE, minBits) - 1) {
        RETURN_JS_ERROR(env, errorCode, "Value[%u] of tag[asset.Tag.%s] has bit count out of range[%u, %u].",
            attr.value.u32, TAG_MAP.at(attr.tag), minBits + 1, maxBits);
    }
    return nullptr;
}

napi_value CheckTagRange(const napi_env env, const AssetAttr &attr, const std::vector<uint32_t> &tags,
    uint32_t errorCode)
{
    auto it = std::find(tags.begin(), tags.end(), attr.value.u32);
    if (it == tags.end()) {
        RETURN_JS_ERROR(env, errorCode, "Value[0x%X] of tag[asset.Tag.(%s)] is not tags allowed for sorting, "
            "which should start with \"DATA_LABEL\".", attr.value.u32, TAG_MAP.at(attr.tag));
    }
    return nullptr;
}

struct CheckContinuousRange {
    std::function<napi_value(const napi_env, const AssetAttr &, uint32_t, uint32_t, uint32_t)> funcPtr;
    uint32_t min;
    uint32_t max;
};

const std::unordered_map<uint32_t, CheckContinuousRange> CHECK_CONTINOUS_RANGE_FUNC_MAP = {
    { SEC_ASSET_TAG_SECRET, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_SECRET_SIZE } },
    { SEC_ASSET_TAG_ALIAS, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_ALIAS_SIZE } },
    { SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, { &CheckNumberRange, MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD } },
    { SEC_ASSET_TAG_AUTH_CHALLENGE, { &CheckArraySize, CHALLENGE_SIZE - 1, CHALLENGE_SIZE } },
    { SEC_ASSET_TAG_AUTH_TOKEN, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_AUTH_TOKEN_SIZE } },
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
    std::function<napi_value(const napi_env, const AssetAttr &, const std::vector<uint32_t> &, uint32_t)> funcPtr;
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

napi_value CheckAssetRequiredTag(const napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &requiredTags, uint32_t errorCode)
{
    for (uint32_t requiredTag : requiredTags) {
        auto it = std::find_if(attrs.begin(), attrs.end(), [requiredTag](const AssetAttr &attr) {
            return attr.tag == requiredTag;
        });
        if (it == attrs.end()) {
            RETURN_JS_ERROR(env, errorCode, "Missing required tag[asset.Tag.%s].", TAG_MAP.at(requiredTag));
        }
    }
    return nullptr;
}

napi_value CheckAssetTagValidity(const napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &validTags, uint32_t errorCode)
{
    for (AssetAttr attr : attrs) {
        if (std::count(validTags.begin(), validTags.end(), attr.tag) == 0) {
            RETURN_JS_ERROR(env, errorCode, "Unsupported tag[asset.Tag.%s] for the function.", TAG_MAP.at(attr.tag));
        }
    }
    return nullptr;
}

napi_value CheckAssetValueValidity(const napi_env env, const std::vector<AssetAttr> &attrs, uint32_t errorCode)
{
    napi_value error = nullptr;
    for (auto attr : attrs) {
        if (CHECK_CONTINOUS_RANGE_FUNC_MAP.find(attr.tag) != CHECK_CONTINOUS_RANGE_FUNC_MAP.end()) {
            auto checkRange = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag);
            error = checkRange.funcPtr(env, attr, checkRange.min, checkRange.max, errorCode);
            if (error != nullptr) {
                return error;
            }
        }
        if (CHECK_DISCRETE_RANGE_FUNC_MAP.find(attr.tag) != CHECK_DISCRETE_RANGE_FUNC_MAP.end()) {
            auto checkRange = CHECK_DISCRETE_RANGE_FUNC_MAP.at(attr.tag);
            error = checkRange.funcPtr(env, attr, checkRange.validRange, errorCode);
            if (error != nullptr) {
                return error;
            }
        }
    }
    return error;
}

} // Asset
} // Security
} // OHOS
