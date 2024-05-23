/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ASSET_NAPI_COMMON_H
#define ASSET_NAPI_COMMON_H

#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

#define NORMAL_ARGS_NUM 1
#define AS_USER_ARGS_NUM 2
#define AS_USER_UPDATE_ARGS_NUM 3
#define UPDATE_ARGS_NUM 2

#define CHECK_RESULT_BREAK(env, ret)                        \
if ((ret) != SEC_ASSET_SUCCESS) {                           \
    napi_throw((env), CreateJsError((env), (ret)));         \
    break;                                                  \
}

typedef struct AsyncContext {
    // common
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;

    // input
    std::vector<AssetAttr> attrs;
    std::vector<AssetAttr> updateAttrs;

    // output
    int32_t result = 0;
    AssetBlob challenge = { 0 };
    AssetResultSet resultSet = { 0 };
} AsyncContext;

void FreeAssetAttrs(std::vector<AssetAttr> &attrs);

napi_value CreateJsError(napi_env env, int32_t errCode);

napi_value CreateJsError(napi_env env, int32_t errCode, const char *errorMsg);

napi_value CreateJsUint8Array(napi_env env, const AssetBlob &blob);

napi_value CreateJsMapArray(napi_env env, const AssetResultSet &resultSet);

napi_status ParseParam(napi_env env, napi_callback_info info, std::vector<AssetAttr> &attrs);

napi_status ParseParam(napi_env env, napi_callback_info info, size_t expectArgNum, std::vector<AssetAttr> &attrs,
    std::vector<AssetAttr> &updateAttrs);

napi_value NapiEntry(napi_env env, napi_callback_info info, const char *funcName, napi_async_execute_callback execute,
    size_t expectArgNum = 1);

napi_value NapiEntryAsUser(napi_env env, napi_callback_info info, const char *funcName,
    napi_async_execute_callback execute, size_t expectArgNum = AS_USER_ARGS_NUM);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_COMMON_H