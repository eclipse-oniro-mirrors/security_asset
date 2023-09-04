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

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

#define DEFAULT_MAX_ARGS_NUM 2
#define DEFAULT_MIN_ARGS_NUM 1
#define UPDATE_MAX_ARGS_NUM 3
#define UPDATE_MIN_ARGS_NUM 2
#define NAPI_THROW_BASE(env, assertion, ret, code, message)            \
if ((assertion)) {                                                     \
    napi_throw_error((env), std::to_string((code)).c_str(), message);  \
    return (ret);                                                      \
}

#define NAPI_THROW(env, assertion, code, message) \
    NAPI_THROW_BASE(env, assertion, nullptr, code, message)

#define NAPI_THROW_RETURN(env, assertion, code, message) \
    NAPI_THROW_BASE(env, assertion, napi_generic_failure, code, message)

#define NAPI_THROW_BREAK(env, assertion, code, message) \
if ((assertion)) { \
    napi_throw_error((env), std::to_string((code)).c_str(), message); \
    break; \
}

#define NAPI_CALL_BREAK(env, theCall)   \
if ((theCall) != napi_ok) {             \
    GET_AND_THROW_LAST_ERROR((env));    \
    break;                              \
}

#define NAPI_CALL_RETURN(env, theCall)  \
if ((theCall) != napi_ok) {             \
    GET_AND_THROW_LAST_ERROR((env));    \
    return napi_generic_failure;        \
}

typedef struct AsyncContext {
    napi_async_work asyncWork = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callback = nullptr;

    AssetParam *params = nullptr;
    uint32_t paramCnt = 0;
    int32_t result = 0;
} AsyncContext;

AsyncContext *CreateAsyncContext();

void DestroyAsyncContext(napi_env env, AsyncContext *context);

napi_status ParseMapParam(napi_env env, napi_value arg, AssetParam **params, uint32_t *paramCnt);

napi_status ParseCallbackParam(napi_env env, napi_value arg, napi_ref *callback);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_COMMON_H