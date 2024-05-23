#ifndef ASSET_NAPI_ADD_H
#define ASSET_NAPI_ADD_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_napi_common.h"

namespace OHOS {
namespace Security {
namespace Asset {

napi_status CheckAddArgs(napi_env env, const std::vector<AssetAttr> &attrs);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_ADD_H
