#include "asset_napi_common.h"
#include "argument_check.h"

#include <vector>
#include <algorithm>
#include <math.h>
#include <unistd.h>
#include <unordered_map>

#include "securec.h"

#include "asset_system_api.h"
#include "asset_system_type.h"
#include "os_account_wrapper.h"
#include "access_token_wrapper.h"
#include "bms_wrapper.h"

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

bool CheckAssetDataType(napi_env env, const AssetAttr &attr)
{
    if (((attr.tag & SEC_ASSET_TAG_TYPE_MASK) == SEC_ASSET_TYPE_BOOL && typeid(attr.value) != typeid(bool))
        || ((attr.tag & SEC_ASSET_TAG_TYPE_MASK) == SEC_ASSET_TYPE_NUMBER && typeid(attr.value) != typeid(uint32_t))
        || ((attr.tag & SEC_ASSET_TAG_TYPE_MASK) == SEC_ASSET_TYPE_BYTES && typeid(attr.value) != typeid(AssetBlob)))
    {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN, "Incompatible value type to the tag[AssetTag(%s)].", g_tagMap[attr.tag]);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckArraySize(napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max)
{
    if (attr.value.blob.size > max || attr.value.blob.size <= min) {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN,
            "The value[AssetValue(%s)] of tag[AssetTag(%s)] has byte length out of range[%u, %u].",
            attr.value.blob.data, attr.tag, min, max);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckEnumVariant(napi_env env, const AssetAttr &attr, std::vector<uint32_t> &enum_vec)
{
    auto it = std::find(enum_vec.begin(), enum_vec.end(), attr.value.u32);
    if (it == enum_vec.end()) {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is an illegal enumeration variant.",
            attr.value.u32, attr.tag);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckNumberRange(napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max)
{
    if (attr.value.u32 > max || attr.value.u32 <= min) {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is out of range[%u, %u].",
            attr.value.u32, attr.tag, min, max);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckValidBits(napi_env env, const AssetAttr &attr, uint32_t min_bits, uint32_t max_bits)
{
    if (attr.value.u32 >= pow(static_cast<uint32_t>(2), max_bits)
        || attr.value.u32 < pow(static_cast<uint32_t>(2), min_bits) - 1) {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is an invalid bit number.", attr.value.u32, attr.tag);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckTagRange(napi_env env, const AssetAttr &attr, std::vector<uint32_t> &tags)
{
    auto it = std::find(tags.begin(), tags.end(), attr.value.u32);
    if (it == tags.end()) {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is out of the valid tag range[%s, %s].",
            attr.value.u32, attr.tag, g_tagMap[*(tags.begin())], g_tagMap[*(tags.end())]);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckUserId(napi_env env, const AssetAttr &attr)
{
    if (!CheckNumberRange(env, attr, ROOT_USER_UPPERBOUND, I32_MAX)) {
        return false;
    }
    bool exist = false;
    if (!IsUserIdExist(static_cast<int32_t>(attr.value.u32), &exist)) {
        char msg[MAX_MESSAGE_LEN] = { 0 };
        (void)sprintf_s(msg, MAX_MESSAGE_LEN,
            "The value[AssetValue(%u)] of tag[AssetTag(%s)] is a nonexistent user id.",
            attr.value.u32, attr.tag);
        LOGE("[FATAL][NAPI]%{public}s", (msg));
        napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
        return false;
    }
    return true;
}

bool CheckAssetDataValue(napi_env env, const AssetAttr &attr)
{
    switch (attr.tag) {
        case SEC_ASSET_TAG_SECRET: {
            if (!CheckArraySize(env, attr, MIN_ARRAY_SIZE, MAX_SECRET_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_ALIAS: {
            if (!CheckArraySize(env, attr, MIN_ARRAY_SIZE, MAX_ALIAS_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_ACCESSIBILITY: {
            std::vector<uint32_t> enum_vec = {
                SEC_ASSET_ACCESSIBILITY_DEVICE_POWERED_ON,
                SEC_ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED,
                SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED
            };
            if (!CheckEnumVariant(env, attr, enum_vec)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_REQUIRE_PASSWORD_SET
            | SEC_ASSET_TAG_IS_PERSISTENT:
            break;
        case SEC_ASSET_TAG_AUTH_TYPE: {
            std::vector<uint32_t> enum_vec = {
                SEC_ASSET_AUTH_TYPE_NONE,
                SEC_ASSET_AUTH_TYPE_ANY
            };
            if (!CheckEnumVariant(env, attr, enum_vec)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD: {
            if (!CheckNumberRange(env, attr, MIN_NUMBER_VALUE, MAX_AUTH_VALID_PERIOD)){
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_AUTH_CHALLENGE: {
            if (!CheckArraySize(env, attr, CHALLENGE_SIZE - 1, CHALLENGE_SIZE)){
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_AUTH_TOKEN: {
            if (!CheckArraySize(env, attr, AUTH_TOKEN_SIZE - 1, AUTH_TOKEN_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_SYNC_TYPE: {
            if (!CheckValidBits(env, attr, SYNC_TYPE_MIN_BITS, SYNC_TYPE_MAX_BITS)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_CONFLICT_RESOLUTION: {
            std::vector<uint32_t> enum_vec = {
                SEC_ASSET_CONFLICT_OVERWRITE,
                SEC_ASSET_CONFLICT_THROW_ERROR
            };
            if (!CheckEnumVariant(env, attr, enum_vec)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1
            | SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2
            | SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3
            | SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4: {
            if (!CheckArraySize(env, attr, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_DATA_LABEL_NORMAL_1
            | SEC_ASSET_TAG_DATA_LABEL_NORMAL_2
            | SEC_ASSET_TAG_DATA_LABEL_NORMAL_3
            | SEC_ASSET_TAG_DATA_LABEL_NORMAL_4: {
            if (!CheckArraySize(env, attr, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1
            | SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2
            | SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3
            | SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4: {
            if (!CheckArraySize(env, attr, MIN_ARRAY_SIZE, MAX_LABEL_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_RETURN_TYPE: {
            std::vector<uint32_t> enum_vec = {
                SEC_ASSET_RETURN_ALL,
                SEC_ASSET_RETURN_ATTRIBUTES
            };
            if (!CheckEnumVariant(env, attr, enum_vec)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_RETURN_LIMIT: {
            if (!CheckNumberRange(env, attr, MIN_NUMBER_VALUE, MAX_RETURN_LIMIT)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_RETURN_OFFSET:
            break;
        case SEC_ASSET_TAG_RETURN_ORDERED_BY: {
            std::vector<uint32_t> tags;
            std::copy(critical_label_tags.begin(), critical_label_tags.end(), std::back_inserter(tags));
            std::copy(normal_label_tags.begin(), normal_label_tags.end(), std::back_inserter(tags));
            std::copy(normal_local_label_tags.begin(), normal_local_label_tags.end(), std::back_inserter(tags));
            if (!CheckTagRange(env, attr, tags)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_USER_ID: {
            if (!CheckUserId(env, attr)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_UPDATE_TIME: {
            if (!CheckArraySize(env, attr, MIN_ARRAY_SIZE, MAX_TIME_SIZE)) {
                return false;
            }
            break;
        }
        case SEC_ASSET_TAG_OPERATION_TYPE: {
            std::vector<uint32_t> enum_vec = {
                SEC_ASSET_NEED_SYNC,
                SEC_ASSET_NEED_LOGOUT
            };
            if (!CheckEnumVariant(env, attr, enum_vec)) {
                return false;
            }
            break;
        }
    }
    return true;
}

} // anonymous namespace

bool CheckAssetRequiredTag(napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &required_tags)
{
    for (uint32_t required_tag : required_tags) {
        auto it = std::find_if(attrs.begin(), attrs.end(), [required_tag](AssetAttr &attr) {
            return attr.tag == required_tag;
        });
        if (it == attrs.end()) {
            char msg[MAX_MESSAGE_LEN] = { 0 };
            (void)sprintf_s(msg, MAX_MESSAGE_LEN, "Missing required tag[AssetTag(%s)].", g_tagMap[required_tag]);
            LOGE("[FATAL][NAPI]%{public}s", (msg));
            napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
            return false;
        }
    }
    return true;
}

bool CheckAssetTagValidity(napi_env env, const std::vector<AssetAttr> &attrs, const std::vector<uint32_t> &valid_tags)
{
    for (AssetAttr attr : attrs) {
        if (std::count(valid_tags.begin(), valid_tags.end(), attr.tag) == 0) {
            char msg[MAX_MESSAGE_LEN] = { 0 };
            (void)sprintf_s(msg, MAX_MESSAGE_LEN, "Illegal tag[AssetTag(%s)].", g_tagMap[attr.tag]);
            LOGE("[FATAL][NAPI]%{public}s", (msg));
            napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));
            return false;
        }
    }
    return true;
}

bool CheckAssetValueValidity(napi_env env, const std::vector<AssetAttr> &attrs)
{
    for (AssetAttr attr : attrs) {
        if (!CheckAssetDataType(env, attr)) {
            return false;
        }
        if (!CheckAssetDataValue(env, attr)) {
            return false;
        }
    }
    return true;
}