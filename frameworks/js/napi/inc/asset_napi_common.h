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

#include <functional>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_system_api.h"
#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

#define NORMAL_ARGS_NUM 1
#define AS_USER_ARGS_NUM 2

#define MAX_MESSAGE_LEN 256
#define MAX_ARGS_NUM 5

#define NAPI_THROW_BASE(env, condition, ret, code, message)             \
if ((condition)) {                                                      \
    LOGE("[FATAL][NAPI]%{public}s", (message));                         \
    napi_throw((env), CreateJsError((env), (code), (message)));         \
    return (ret);                                                       \
}

#define NAPI_THROW(env, condition, code, message)                       \
    NAPI_THROW_BASE(env, condition, nullptr, code, message)

#define CHECK_RESULT_BREAK(env, ret)                        \
if ((ret) != SEC_ASSET_SUCCESS) {                           \
    napi_throw((env), CreateJsError((env), (ret)));         \
    break;                                                  \
}

#define IF_FALSE_RETURN(result, returnValue)    \
if (!(result)) {                                \
    return (returnValue);                       \
}

using AsyncContext = struct AsyncContext {
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
};

using CheckFuncPtr = std::function<napi_status(const napi_env, const std::vector<AssetAttr> &)>;

using NapiCallerArgs = struct NapiCallerArgs {
    size_t expectArgNum;
    bool isUpdate;
    bool isAsUser;
};

AsyncContext *CreateAsyncContext();

void DestroyAsyncContext(const napi_env env, AsyncContext *context);

napi_value CreateAsyncWork(const napi_env env, AsyncContext *context, const char *funcName,
    napi_async_execute_callback execute);

void FreeAssetAttrs(std::vector<AssetAttr> &attrs);

napi_value CreateJsError(const napi_env env, int32_t errCode);

napi_value CreateJsError(const napi_env env, int32_t errCode, const char *errorMsg);

napi_value CreateJsUint8Array(const napi_env env, const AssetBlob &blob);

napi_value CreateJsMapArray(const napi_env env, const AssetResultSet &resultSet);

napi_status ParseParam(const napi_env env, napi_callback_info info, const NapiCallerArgs &args,
    std::vector<AssetAttr> &attrs);

napi_status ParseParam(const napi_env env, napi_callback_info info, const NapiCallerArgs &args,
    std::vector<AssetAttr> &attrs, std::vector<AssetAttr> &updateAttrs);

napi_value NapiAsync(const napi_env env, napi_callback_info info, napi_async_execute_callback execute,
    const NapiCallerArgs &args, CheckFuncPtr checkFunc);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_COMMON_H