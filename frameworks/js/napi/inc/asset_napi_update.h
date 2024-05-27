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

#ifndef ASSET_NAPI_UPDATE_H
#define ASSET_NAPI_UPDATE_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_napi_common.h"

namespace OHOS {
namespace Security {
namespace Asset {

typedef std::function<napi_status(napi_env, const std::vector<AssetAttr> &, const std::vector<AssetAttr> &)>
    CheckUpdateFuncPtr;

typedef struct NapiUpdateCallerArgs {
    size_t expectArgNum;
    bool isUpdate;
    bool isAsUser;
    CheckUpdateFuncPtr checkUpdateFuncPtr;
} NapiUpdateCallerArgs;

napi_status ParseUpdateParam(napi_env env, napi_callback_info info, const NapiUpdateCallerArgs &args,
    std::vector<AssetAttr> &attrs, std::vector<AssetAttr> &updateAttrs);

napi_value NapiUpdateAsync(napi_env env, napi_callback_info info, const char *funcName,
    napi_async_execute_callback execute, const NapiUpdateCallerArgs &args);

napi_value NapiUpdate(napi_env env, napi_callback_info info);

napi_value NapiUpdateSync(napi_env env, napi_callback_info info);

napi_value NapiUpdateAsUser(napi_env env, napi_callback_info info);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_UPDATE_H