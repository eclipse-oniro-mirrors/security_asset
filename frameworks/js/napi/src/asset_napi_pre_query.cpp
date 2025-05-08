/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "asset_napi_pre_query.h"

#include <cstdint>
#include <vector>

#include "securec.h"

#include "asset_log.h"
#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_napi_check.h"
#include "asset_napi_common.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
const uint32_t QUERY_ARG_COUNT = 1;
const uint32_t QUERY_ARG_COUNT_AS_USER = 2;

const std::vector<uint32_t> OPTIONAL_TAGS = {
    SEC_ASSET_TAG_AUTH_VALIDITY_PERIOD
};

napi_status CheckPreQueryArgs(const napi_env env, const std::vector<AssetAttr> &attrs)
{
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), OPTIONAL_TAGS.begin(), OPTIONAL_TAGS.end());
    IF_ERROR_THROW_RETURN(env, CheckAssetTagValidity(env, attrs, validTags, SEC_ASSET_INVALID_ARGUMENT));
    IF_ERROR_THROW_RETURN(env, CheckAssetValueValidity(env, attrs, SEC_ASSET_INVALID_ARGUMENT));
    return napi_ok;
}

napi_status ParseAttrMap(napi_env env, napi_callback_info info, BaseContext *context)
{
    napi_value argv[QUERY_ARG_COUNT] = { 0 };
    IF_ERR_RETURN(ParseJsArgs(env, info, argv, QUERY_ARG_COUNT));
    IF_ERR_RETURN(ParseJsMap(env, argv[0], context->attrs));
    IF_ERR_RETURN(CheckPreQueryArgs(env, context->attrs));
    return napi_ok;
}

napi_status ParseAttrMapAsUser(napi_env env, napi_callback_info info, BaseContext *context)
{
    napi_value argv[QUERY_ARG_COUNT_AS_USER] = { 0 };
    IF_ERR_RETURN(ParseJsArgs(env, info, argv, QUERY_ARG_COUNT_AS_USER));
    uint32_t index = 0;
    IF_ERR_RETURN(ParseJsMap(env, argv[index++], context->attrs));
    IF_ERR_RETURN(ParseJsUserId(env, argv[index++], context->attrs));
    IF_ERR_RETURN(CheckPreQueryArgs(env, context->attrs));
    return napi_ok;
}
} // anonymous namespace

napi_value NapiPreQuery(const napi_env env, napi_callback_info info, bool asUser, bool async)
{
    auto context = std::unique_ptr<PreQueryContext>(new (std::nothrow)PreQueryContext());
    NAPI_THROW(env, context == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for Context.");

    context->parse = asUser ? ParseAttrMapAsUser : ParseAttrMap;
    context->execute = [](napi_env env, void *data) {
        PreQueryContext *context = static_cast<PreQueryContext *>(data);
        context->result = AssetPreQuery(&context->attrs[0], context->attrs.size(), &context->challenge);
    };

    context->resolve = [](napi_env env, BaseContext *baseContext) -> napi_value {
        PreQueryContext *context = static_cast<PreQueryContext *>(baseContext);
        return CreateJsUint8Array(env, context->challenge);
    };

    if (async) {
        return CreateAsyncWork(env, info, std::move(context), __func__);
    } else {
        return CreateSyncWork(env, info, context.get());
    }
}

napi_value NapiPreQuery(const napi_env env, napi_callback_info info)
{
    return NapiPreQuery(env, info, false, true);
}

napi_value NapiPreQueryAsUser(const napi_env env, napi_callback_info info)
{
    return NapiPreQuery(env, info, true, true);
}

napi_value NapiPreQuerySync(const napi_env env, napi_callback_info info)
{
    return NapiPreQuery(env, info, false, false);
}

} // Asset
} // Security
} // OHOS
