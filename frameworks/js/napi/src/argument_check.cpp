#include "asset_napi_common.h"
#include "argument_check.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <unistd.h>
#include <unordered_map>
#include <functional>

#include "securec.h"

#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_log.h"

namespace OHOS {
namespace Security {
namespace Asset {

namespace {

#define MAX_MESSAGE_LEN 128

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
#define I32_MAX 0x7FFFFFFF
#define MAX_TIME_SIZE 1024
#define SYSTEM_USER_ID_MAX 99
#define BINARY_BASE 2


bool CheckArraySize(napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max)
{
    if (attr.value.blob.size > max || attr.value.blob.size <= min) {
        NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
            "The value[AssetValue(%s)] of tag[AssetTag(%s)] has byte length out of range[%u, %u].",
            attr.value.blob.data, g_tagMap.at(attr.tag), min, max);
    }
    return true;
}

bool CheckEnumVariant(napi_env env, const AssetAttr &attr, std::vector<uint32_t> &enumVec)
{
    auto it = std::find(enumVec.begin(), enumVec.end(), attr.value.u32);
    if (it == enumVec.end()) {
        NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is an illegal enumeration variant.",
            attr.value.u32, g_tagMap.at(attr.tag));
        return false;
    }
    return true;
}

bool CheckNumberRange(napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max)
{
    if (attr.value.u32 > max || attr.value.u32 <= min) {
        NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is out of range[%u, %u].",
            attr.value.u32, g_tagMap.at(attr.tag), min, max);
        return false;
    }
    return true;
}

bool CheckValidBits(napi_env env, const AssetAttr &attr, uint32_t minBits, uint32_t maxBits)
{
    LOGE("111111");
    if (attr.value.u32 >= pow(static_cast<uint32_t>(BINARY_BASE), maxBits) ||
        attr.value.u32 < pow(static_cast<uint32_t>(BINARY_BASE), minBits) - 1) {
        LOGE("22222");
        NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is an invalid bit number.",
            attr.value.u32, g_tagMap.at(attr.tag));
        LOGE("33333");
        return false;
    }
    return true;
}

bool CheckTagRange(napi_env env, const AssetAttr &attr, std::vector<uint32_t> &tags)
{
    auto it = std::find(tags.begin(), tags.end(), attr.value.u32);
    if (it == tags.end()) {
        NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is out of the valid tag range[%s, %s].",
            attr.value.u32, g_tagMap.at(attr.tag), g_tagMap.at(*(tags.begin())), g_tagMap.at(*(tags.end())));
        return false;
    }
    return true;
}

const std::unordered_map<uint32_t, std::function<bool(napi_env, const AssetAttr &, uint32_t, uint32_t)>>
    g_firstFuncMap = {
    {SEC_ASSET_TAG_SECRET, &CheckArraySize},
    {SEC_ASSET_TAG_ALIAS, &CheckArraySize},
    {SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, &CheckNumberRange},
    {SEC_ASSET_TAG_AUTH_CHALLENGE, &CheckArraySize},
    {SEC_ASSET_TAG_AUTH_TOKEN, &CheckArraySize},
    {SEC_ASSET_TAG_SYNC_TYPE, &CheckValidBits},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3, &CheckArraySize},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4, &CheckArraySize},
    {SEC_ASSET_TAG_RETURN_LIMIT, &CheckNumberRange},
    {SEC_ASSET_TAG_USER_ID, &CheckNumberRange},
    {SEC_ASSET_TAG_UPDATE_TIME, &CheckArraySize}
};

const std::unordered_map<uint32_t, std::vector<uint32_t>> g_firstParamMap = {
    {SEC_ASSET_TAG_SECRET, {MIN_ARRAY_SIZE, MAX_SECRET_SIZE}},
    {SEC_ASSET_TAG_ALIAS, {MIN_ARRAY_SIZE, MAX_ALIAS_SIZE}},
    {SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, {MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD}},
    {SEC_ASSET_TAG_AUTH_CHALLENGE, {CHALLENGE_SIZE - 1, CHALLENGE_SIZE}},
    {SEC_ASSET_TAG_AUTH_TOKEN, {AUTH_TOKEN_SIZE - 1, AUTH_TOKEN_SIZE}},
    {SEC_ASSET_TAG_SYNC_TYPE, {SYNC_TYPE_MIN_BITS, SYNC_TYPE_MAX_BITS}},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4, {MIN_ARRAY_SIZE, MAX_LABEL_SIZE}},
    {SEC_ASSET_TAG_RETURN_LIMIT, {MIN_NUMBER_VALUE, MAX_RETURN_LIMIT}},
    {SEC_ASSET_TAG_USER_ID, {ROOT_USER_UPPERBOUND, I32_MAX}},
    {SEC_ASSET_TAG_UPDATE_TIME, {MIN_ARRAY_SIZE, MAX_TIME_SIZE}}
};

const std::unordered_map<uint32_t, std::function<bool(napi_env, const AssetAttr &, std::vector<uint32_t> &)>>
    g_secondFuncMap = {
        {SEC_ASSET_TAG_ACCESSIBILITY, &CheckEnumVariant},
        {SEC_ASSET_TAG_AUTH_TYPE, &CheckEnumVariant},
        {SEC_ASSET_TAG_CONFLICT_RESOLUTION, &CheckEnumVariant},
        {SEC_ASSET_TAG_RETURN_TYPE, &CheckEnumVariant},
        {SEC_ASSET_TAG_RETURN_ORDERED_BY, &CheckTagRange},
        {SEC_ASSET_TAG_OPERATION_TYPE, &CheckEnumVariant}
};

const std::unordered_map<uint32_t, std::vector<uint32_t>> g_secondParamMap = {
        {SEC_ASSET_TAG_ACCESSIBILITY, {
            SEC_ASSET_ACCESSIBILITY_DEVICE_POWERED_ON,
            SEC_ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED,
            SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED
        }},
        {SEC_ASSET_TAG_AUTH_TYPE, {
            SEC_ASSET_AUTH_TYPE_NONE,
            SEC_ASSET_AUTH_TYPE_ANY
        }},
        {SEC_ASSET_TAG_CONFLICT_RESOLUTION, {
            SEC_ASSET_CONFLICT_OVERWRITE,
            SEC_ASSET_CONFLICT_THROW_ERROR
        }},
        {SEC_ASSET_TAG_RETURN_TYPE, {
            SEC_ASSET_RETURN_ALL,
            SEC_ASSET_RETURN_ATTRIBUTES
        }},
        {SEC_ASSET_TAG_RETURN_ORDERED_BY, {
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
        }},
        {SEC_ASSET_TAG_OPERATION_TYPE, {
            SEC_ASSET_NEED_SYNC,
            SEC_ASSET_NEED_LOGOUT
        }}
};

bool CheckAssetDataValue(napi_env env, const AssetAttr &attr)
{
    if(g_firstFuncMap.find(attr.tag) != g_firstFuncMap.end()) {
        auto funcPtr = g_firstFuncMap.at(attr.tag);
        auto paramPtr = g_firstParamMap.at(attr.tag);
        uint32_t min = paramPtr[0];
        uint32_t max = paramPtr[1];
        if(!funcPtr(env, attr, min, max)) {
            return false;
        }
    }
    if(g_secondFuncMap.find(attr.tag) != g_secondFuncMap.end()) {
        auto funcPtr = g_secondFuncMap.at(attr.tag);
        auto paramPtr = g_secondParamMap.at(attr.tag);
        if(!funcPtr(env, attr, paramPtr)) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

bool CheckAssetRequiredTag(napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &requiredTags)
{
    for (uint32_t requiredTag : requiredTags) {
        auto it = std::find_if(attrs.begin(), attrs.end(), [requiredTag](const AssetAttr &attr) {
            return attr.tag == requiredTag;
        });
        if (it == attrs.end()) {
            NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
                "Missing required tag[AssetTag(%s)].",
                g_tagMap.at(requiredTag));
            return false;
        }
    }
    return true;
}

bool CheckAssetTagValidity(napi_env env, const std::vector<AssetAttr> &attrs, const std::vector<uint32_t> &validTags)
{
    for (AssetAttr attr : attrs) {
        if (std::count(validTags.begin(), validTags.end(), attr.tag) == 0) {
            NAPI_THROW_RETURN_INVALID_ARGUMENT(env,
                "Illegal tag[AssetTag(%s)].",
                g_tagMap.at(attr.tag));
            return false;
        }
    }
    return true;
}

bool CheckAssetValueValidity(napi_env env, const std::vector<AssetAttr> &attrs)
{
    for (AssetAttr attr : attrs) {
        LOGE("00000");
        if (!CheckAssetDataValue(env, attr)) {
            return false;
        }
    }
    return true;
}

} // Asset
} // Security
} // OHOS
