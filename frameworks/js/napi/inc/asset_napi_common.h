/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "asset_napi_context.h"

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

#define IF_ERROR_THROW_RETURN(env, result)                 \
if ((result) != nullptr) {                                 \
    napi_throw((env), (result));                           \
    return napi_invalid_arg;                               \
}

#define IF_FALSE_RETURN(result, returnValue)    \
if (!(result)) {                                \
    return (returnValue);                       \
}

#define IF_ERR_RETURN(result)                   \
if ((result) != napi_ok) {                      \
    return (result);                            \
}

napi_value CreateJsError(const napi_env env, int32_t errCode);

napi_value CreateJsError(const napi_env env, int32_t errCode, const char *errorMsg);

napi_value CreateJsUint8Array(const napi_env env, const AssetBlob &blob);

napi_value CreateJsMapArray(const napi_env env, const AssetResultSet &resultSet);

napi_value CreateJsUndefined(const napi_env env);

napi_status ParseJsArgs(const napi_env env, napi_callback_info info, napi_value *value, size_t valueSize);

napi_status ParseJsMap(const napi_env env, napi_value arg, std::vector<AssetAttr> &attrs);

napi_status ParseJsUserId(const napi_env env, napi_value arg, std::vector<AssetAttr> &attrs);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, uint32_t propertyValue);

napi_value CreateAsyncWork(const napi_env env, napi_callback_info info, std::unique_ptr<BaseContext> context,
    const char *resourceName);

napi_value CreateSyncWork(const napi_env env, napi_callback_info info, BaseContext *context);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_COMMON_H