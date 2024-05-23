#ifndef ARGUMENT_CHECK_H
#define ARGUMENT_CHECK_H

#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

#define NAPI_THROW_RETURN_INVALID_ARGUMENT(env, format, arg...)                     \
    char msg[MAX_MESSAGE_LEN] = { 0 };                                              \
    (void)sprintf_s(msg, MAX_MESSAGE_LEN, format, ##arg);                           \
    LOGE("[FATAL][NAPI]%{public}s", (msg));                                         \
    napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));     \

const std::vector<uint32_t> g_criticalLabelTags = {
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4
};
const std::vector<uint32_t> g_normalLabelTags = {
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_4
};
const std::vector<uint32_t> g_normalLocalLabelTags = {
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4
};
const std::vector<uint32_t> g_accessControlTags = {
    SEC_ASSET_TAG_ALIAS,
    SEC_ASSET_TAG_ACCESSIBILITY,
    SEC_ASSET_TAG_AUTH_TYPE,
    SEC_ASSET_TAG_IS_PERSISTENT,
    SEC_ASSET_TAG_SYNC_TYPE,
    SEC_ASSET_TAG_REQUIRE_PASSWORD_SET
};

const std::unordered_map<uint32_t, const char *> g_tagMap = {
    {SEC_ASSET_TAG_SECRET, "SECRET"},
    {SEC_ASSET_TAG_ALIAS, "ALIAS"},
    {SEC_ASSET_TAG_ACCESSIBILITY, "ACCESSIBILITY"},
    {SEC_ASSET_TAG_REQUIRE_PASSWORD_SET, "REQUIRE_PASSWORD_SET"},
    {SEC_ASSET_TAG_AUTH_TYPE, "AUTH_TYPE"},
    {SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD, "AUTH_VALIDITY_PERIOD"},
    {SEC_ASSET_TAG_AUTH_CHALLENGE, "AUTH_CHALLENGE"},
    {SEC_ASSET_TAG_AUTH_TOKEN, "AUTH_TOKEN"},
    {SEC_ASSET_TAG_SYNC_TYPE, "SYNC_TYPE"},
    {SEC_ASSET_TAG_IS_PERSISTENT, "IS_PERSISTENT"},
    {SEC_ASSET_TAG_CONFLICT_RESOLUTION, "CONFLICT_RESOLUTION"},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, "DATA_LABEL_CRITICAL_1"},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, "DATA_LABEL_CRITICAL_2"},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, "DATA_LABEL_CRITICAL_3"},
    {SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, "DATA_LABEL_CRITICAL_4"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, "DATA_LABEL_NORMAL_1"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, "DATA_LABEL_NORMAL_2"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, "DATA_LABEL_NORMAL_3"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, "DATA_LABEL_NORMAL_4"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1, "DATA_LABEL_NORMAL_LOCAL_1"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2, "DATA_LABEL_NORMAL_LOCAL_2"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3, "DATA_LABEL_NORMAL_LOCAL_3"},
    {SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4, "DATA_LABEL_NORMAL_LOCAL_4"},
    {SEC_ASSET_TAG_RETURN_TYPE, "RETURN_TYPE"},
    {SEC_ASSET_TAG_RETURN_LIMIT, "RETURN_LIMIT"},
    {SEC_ASSET_TAG_RETURN_OFFSET, "RETURN_OFFSET"},
    {SEC_ASSET_TAG_RETURN_ORDERED_BY, "RETURN_ORDERED_BY"},
    {SEC_ASSET_TAG_UPDATE_TIME, "UPDATE_TIME"},
    {SEC_ASSET_TAG_OPERATION_TYPE, "OPERATION_TYPE"}
};

bool CheckAssetRequiredTag(napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<uint32_t> &requiredTags);

bool CheckAssetTagValidity(napi_env env, const std::vector<AssetAttr> &attrs, const std::vector<uint32_t> &validTags);

bool CheckAssetValueValidity(napi_env env, const std::vector<AssetAttr> &attrs);

} // Asset
} // Security
} // OHOS

#endif // ARGUMENT_CHECK_H
