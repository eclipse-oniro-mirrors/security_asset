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

#include "asset_napi_query_sync_result.h"

#include "asset_log.h"
#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_napi_check.h"
#include "asset_napi_common.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
const uint32_t ARG_COUNT = 1;

const std::vector<uint32_t> OPTIONAL_TAGS = {
    SEC_ASSET_TAG_GROUP_ID,
    SEC_ASSET_TAG_REQUIRE_ATTR_ENCRYPTED,
};

napi_value CheckQuerySyncResultArgs(napi_env env, const std::vector<AssetAttr> &attrs)
{
    napi_value error = CheckAssetTagValidity(env, attrs, OPTIONAL_TAGS, SEC_ASSET_PARAM_VERIFICATION_FAILED);
    if (error != nullptr) {
        return error;
    }

    return CheckAssetValueValidity(env, attrs, SEC_ASSET_PARAM_VERIFICATION_FAILED);
}
} // anonymous namespace

napi_value NapiQuerySyncResult(const napi_env env, napi_callback_info info)
{
    auto context = std::unique_ptr<QuerySyncResultContext>(new (std::nothrow)QuerySyncResultContext());
    NAPI_THROW(env, context == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for Context.");

    context->parse = [](napi_env env, napi_callback_info info, BaseContext *baseContext) -> napi_status {
        QuerySyncResultContext *context = reinterpret_cast<QuerySyncResultContext *>(baseContext);
        napi_value argv[MAX_ARGS_NUM] = { 0 };
        IF_ERR_RETURN(ParseJsArgs(env, info, argv, ARG_COUNT));
        IF_ERR_RETURN(ParseJsMap(env, argv[0], context->attrs));
        return napi_ok;
    };

    context->execute = [](napi_env env, void *data) {
        QuerySyncResultContext *context = static_cast<QuerySyncResultContext *>(data);
        context->error = CheckQuerySyncResultArgs(env, context->attrs);
        if (context->error != nullptr) {
            return;
        }
        context->result = AssetQuerySyncResult(&context->attrs[0], context->attrs.size(), &context->syncResult);
    };

    context->resolve = [](napi_env env, BaseContext *baseContext) -> napi_value {
        QuerySyncResultContext *context = static_cast<QuerySyncResultContext *>(baseContext);
        napi_value syncResult = nullptr;
        NAPI_CALL(env, napi_create_object(env, &syncResult));
        NAPI_CALL(env, NapiSetProperty(env, syncResult, "resultCode", context->syncResult.resultCode));
        NAPI_CALL(env, NapiSetProperty(env, syncResult, "totalCount", context->syncResult.totalCount));
        NAPI_CALL(env, NapiSetProperty(env, syncResult, "failedCount", context->syncResult.failedCount));
        return syncResult;
    };
    return CreateAsyncWork(env, info, std::move(context), __func__);
}

} // Asset
} // Security
} // OHOS
