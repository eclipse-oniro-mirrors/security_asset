#ifndef CHECK_ARGUMENT_H
#define CHECK_ARGUMENT_H

#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {
std::vector<uint32_t> critical_label_tags = {
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3,
    SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4
};
std::vector<uint32_t> normal_label_tags = {
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_4
};
std::vector<uint32_t> normal_local_label_tags = {
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_1,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_2,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_3,
    SEC_ASSET_TAG_DATA_LABEL_NORMAL_LOCAL_4
};
std::vector<uint32_t> access_control_tags = {
    SEC_ASSET_TAG_ALIAS,
    SEC_ASSET_TAG_ACCESSIBILITY,
    SEC_ASSET_TAG_AUTH_TYPE,
    SEC_ASSET_TAG_IS_PERSISTENT,
    SEC_ASSET_TAG_SYNC_TYPE,
    SEC_ASSET_TAG_REQUIRE_PASSWORD_SET
};

std::unordered_map<uint32_t, const char *> g_tagMap = {
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
    const std::vector<uint32_t> &required_tags);

bool CheckAssetTagValidity(napi_env env, const std::vector<AssetAttr> &attrs, const std::vector<uint32_t> &valid_tags);

bool CheckAssetDataType(napi_env env, const AssetAttr &attr);

bool CheckArraySize(napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max);

bool CheckEnumVariant(napi_env env, const AssetAttr &attr, std::vector<uint32_t> &enum_vec);

bool CheckNumberRange(napi_env env, const AssetAttr &attr, uint32_t min, uint32_t max);

bool CheckValidBits(napi_env env, const AssetAttr &attr, uint32_t min_bits, uint32_t max_bits);

bool CheckTagRange(napi_env env, const AssetAttr &attr, std::vector<uint32_t> &tags);

bool CheckUserId(napi_env env, const AssetAttr &attr);

bool CheckAssetDataValue(napi_env env, const AssetAttr &attr);

bool CheckAssetValueValidity(napi_env env, const std::vector<AssetAttr> &attrs);

} // Asset
} // Security
} // OHOS

#endif // CHECK_ARGUMENT_H