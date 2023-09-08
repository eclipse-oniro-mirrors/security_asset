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

typedef struct AsyncContext {
    // common
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callback = nullptr;

    // input
    AssetParam *params = nullptr;
    uint32_t paramCnt = 0;
    AssetParam *updateParams = nullptr;
    uint32_t updateParamCnt = 0;

    // output
    int32_t result = 0;
    AssetBlob challenge = { 0 };
    AssetResultSet resultSet = { 0 };
} AsyncContext;


napi_value NapiEntry(napi_env env, napi_callback_info info, const char *funcName, napi_async_execute_callback execute,
    size_t expectArgNum = 2);

} // Asset
} // Security
} // OHOS

#endif // ASSET_NAPI_COMMON_H