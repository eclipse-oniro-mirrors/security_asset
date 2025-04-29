/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ASSET_NAPI_CONTEXT_H
#define ASSET_NAPI_CONTEXT_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_system_type.h"

namespace OHOS {
namespace Security {
namespace Asset {

class BaseContext {
public:
    virtual ~BaseContext();

    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_value error = nullptr;
    std::vector<AssetAttr> attrs;
    int32_t result = SEC_ASSET_INVALID_ARGUMENT;

    std::function<napi_status(const napi_env, napi_callback_info, BaseContext *)> parse;
    napi_async_execute_callback execute;
    std::function<napi_value(napi_env, BaseContext *)> resolve;
};

class PreQueryContext : public BaseContext {
public:
    ~PreQueryContext();
    AssetBlob challenge = { 0 };
};

class QueryContext : public BaseContext {
public:
    ~QueryContext();
    AssetResultSet resultSet = { 0 };
};

class UpdateContext : public BaseContext {
public:
    ~UpdateContext();
    std::vector<AssetAttr> updateAttrs;
};

class QuerySyncResultContext : public BaseContext {
public:
    // output
    AssetSyncResult syncResult;
};

} // Asset
} // Security
} // OHOS
#endif // ASSET_NAPI_CONTEXT_H
