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

#include "asset_napi_check.h"
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
#define AUTH_TOKEN_SIZE 280
#define MAX_LABEL_SIZE 2048
#define MAX_RETURN_LIMIT 0x10000
#define SYNC_TYPE_MIN_BITS 0
#define SYNC_TYPE_MAX_BITS 3
#define ROOT_USER_UPPERBOUND 99
#define MAX_TIME_SIZE 1024
#define SYSTEM_USER_ID_MAX 99
#define BINARY_BASE 2

bool CheckArraySize(const napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max)
{
    if (attr.value.blob.size > max || attr.value.blob.size <= min) {
        NAPI_THROW_INVALID_ARGUMENT(env,
            "Value byte length[%u] of tag[asset.Tag.%s] is out of range[%u, %u].",
            attr.value.blob.size, TAG_MAP.at(attr.tag),  min + 1, max);
        return false;
    }
    return true;
}

bool CheckEnumVariant(const napi_env env, const AssetAttr &attr, const std::vector<uint32_t> &enumVec)
{
    auto it = std::find(enumVec.begin(), enumVec.end(), attr.value.u32);
    if (it == enumVec.end()) {
        NAPI_THROW_INVALID_ARGUMENT(env,
            "Value[%u] of tag[asset.Tag.%s] is an illegal enumeration variant.",
            attr.value.u32, TAG_MAP.at(attr.tag));
        return false;
    }
    return true;
}

bool CheckNumberRange(const napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max)
{
    if (attr.value.u32 > max || attr.value.u32 <= min) {
        NAPI_THROW_INVALID_ARGUMENT(env,
            "Value[%u] of tag[asset.Tag.%s] is out of range[%u, %u].",
            attr.value.u32, TAG_MAP.at(attr.tag), min, max);
        return false;
    }
    return true;
}

bool CheckValidBits(const napi_env env, const AssetAttr &attr, uint32_t minBits, uint32_t maxBits)
{
    if (attr.value.u32 >= pow(BINARY_BASE, maxBits) || attr.value.u32 < pow(BINARY_BASE, minBits) - 1) {
        NAPI_THROW_INVALID_ARGUMENT(env,
            "Value[%u] of tag[asset.Tag.%s] has bit count out of range[%u, %u].",
            attr.value.u32, TAG_MAP.at(attr.tag), minBits + 1, maxBits);
        return false;
    }
    return true;
}

bool CheckTagRange(const napi_env env, const AssetAttr &attr, const std::vector<uint32_t> &tags)
{
    auto it = std::find(tags.begin(), tags.end(), attr.value.u32);
    if (it == tags.end()) {
        NAPI_THROW_INVALID_ARGUMENT(env,
            "Value[0x%X] of tag[asset.Tag.(%s)] is not tags allowed for sorting, "
            "which should start with \"DATA_LABEL\".", attr.value.u32, TAG_MAP.at(attr.tag));
        return false;
    }
    return true;
}

struct CheckContinuousRange {
    std::function<bool(const napi_env, const AssetAttr &, uint32_t, uint32_t)> funcPtr;
    uint32_t min;
    uint32_t max;
};

const std::unordered_map<uint32_t, CheckContinuousRange> CHECK_CONTINOUS_RANGE_FUNC_MAP = {
    { SEC_ASSET_TAG_SECRET, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_SECRET_SIZE } },
    { SEC_ASSET_TAG_ALIAS, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_ALIAS_SIZE } },
    { SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, { &CheckNumberRange, MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD } },
    { SEC_ASSET_TAG_AUTH_CHALLENGE, { &CheckArraySize, CHALLENGE_SIZE - 1, CHALLENGE_SIZE } },
    { SEC_ASSET_TAG_AUTH_TOKEN, { &CheckArraySize, AUTH_TOKEN_SIZE - 1, AUTH_TOKEN_SIZE } },
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
    { SEC_ASSET_TAG_USER_ID, { &CheckNumberRange, ROOT_USER_UPPERBOUND, INT32_MAX } },
    { SEC_ASSET_TAG_UPDATE_TIME, { &CheckArraySize, MIN_ARRAY_SIZE, MAX_TIME_SIZE } }
};

struct CheckDiscreteRange {
    std::function<bool(const napi_env, const AssetAttr &, const std::vector<uint32_t> &)> funcPtr;
    const std::vector<uint32_t> validRange;
};

const std::unordered_map<uint32_t, CheckDiscreteRange> CHECK_DISCRETE_RANGE_FUNC_MAP = {
    { SEC_ASSET_TAG_ACCESSIBILITY, { &CheckEnumVariant, ASSET_ACCESSIBILITY_VEC } },
    { SEC_ASSET_TAG_AUTH_TYPE, { &CheckEnumVariant, ASSET_AUTH_TYPE_VEC } },
    { SEC_ASSET_TAG_CONFLICT_RESOLUTION, { &CheckEnumVariant, ASSET_CONFLICT_RESOLUTION_VEC } },
    { SEC_ASSET_TAG_RETURN_TYPE, { &CheckEnumVariant, ASSET_RETURN_TYPE_VEC } },
    { SEC_ASSET_TAG_RETURN_ORDERED_BY, { &CheckTagRange, ASSET_RETURN_ORDER_BY_TAGS } },
    { SEC_ASSET_TAG_OPERATION_TYPE, { &CheckEnumVariant, ASSET_OPERATION_TYPE_VEC } }
};

} // anonymous namespace

bool CheckAssetRequiredTag(const napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &requiredTags)
{
    for (uint32_t requiredTag : requiredTags) {
        auto it = std::find_if(attrs.begin(), attrs.end(), [requiredTag](const AssetAttr &attr) {
            return attr.tag == requiredTag;
        });
        if (it == attrs.end()) {
            NAPI_THROW_INVALID_ARGUMENT(env, "Missing required tag[asset.Tag.%s].", TAG_MAP.at(requiredTag));
            return false;
        }
    }
    return true;
}

bool CheckAssetTagValidity(const napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &validTags)
{
    for (AssetAttr attr : attrs) {
        if (std::count(validTags.begin(), validTags.end(), attr.tag) == 0) {
            NAPI_THROW_INVALID_ARGUMENT(env, "Unsupported tag[asset.Tag.%s] for the function.",
                TAG_MAP.at(attr.tag));
            return false;
        }
    }
    return true;
}

bool CheckAssetValueValidity(const napi_env env, const std::vector<AssetAttr> &attrs)
{
    return std::all_of(attrs.begin(), attrs.end(), [env](const AssetAttr &attr) {
        if (CHECK_CONTINOUS_RANGE_FUNC_MAP.find(attr.tag) != CHECK_CONTINOUS_RANGE_FUNC_MAP.end()) {
            auto funcPtr = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag).funcPtr;
            uint32_t min = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag).min;
            uint32_t max = CHECK_CONTINOUS_RANGE_FUNC_MAP.at(attr.tag).max;
            return funcPtr(env, attr, min, max);
        }
        if (CHECK_DISCRETE_RANGE_FUNC_MAP.find(attr.tag) != CHECK_DISCRETE_RANGE_FUNC_MAP.end()) {
            auto funcPtr = CHECK_DISCRETE_RANGE_FUNC_MAP.at(attr.tag).funcPtr;
            auto validRangePtr = CHECK_DISCRETE_RANGE_FUNC_MAP.at(attr.tag).validRange;
            return funcPtr(env, attr, validRangePtr);
        }
        return true;
        });
}

} // Asset
} // Security
} // OHOS
