#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_log.h"
#include "asset_napi_common.h"
#include "argument_check.h"
#include "asset_napi_add.h"
#include "asset_system_api.h"
#include "asset_system_type.h"
#include <vector>
#include <cstdint>

namespace OHOS {
namespace Security {
namespace Asset {

std::vector<uint32_t> g_requiredTags = {
    SEC_ASSET_TAG_SECRET,
    SEC_ASSET_TAG_ALIAS
};
std::vector<uint32_t> g_optionalTags = {
    SEC_ASSET_TAG_SECRET,
    SEC_ASSET_TAG_CONFLICT_RESOLUTION,
    SEC_ASSET_TAG_IS_PERSISTENT,
    SEC_ASSET_TAG_USER_ID
};
std::vector<uint32_t> g_validTags;

napi_status CheckAddArgs(napi_env env, const std::vector<AssetAttr> &attrs)
{
    g_validTags.insert(g_validTags.end(), g_criticalLabelTags.begin(), g_criticalLabelTags.end());
    g_validTags.insert(g_validTags.end(), g_normalLabelTags.begin(), g_normalLabelTags.end());
    g_validTags.insert(g_validTags.end(), g_normalLocalLabelTags.begin(), g_normalLocalLabelTags.end());
    g_validTags.insert(g_validTags.end(), g_accessControlTags.begin(), g_accessControlTags.end());
    g_validTags.insert(g_validTags.end(), g_optionalTags.begin(), g_optionalTags.end());
    if (!CheckAssetRequiredTag(env, attrs, g_requiredTags)) {
        return napi_invalid_arg;
    }
    if (!CheckAssetTagValidity(env, attrs, g_validTags)) {
        return napi_invalid_arg;
    }
    if (!CheckAssetValueValidity(env, attrs)) {
        return napi_invalid_arg;
    }
    return napi_ok;
}

} // Asset
} // Security
} // OHOS
