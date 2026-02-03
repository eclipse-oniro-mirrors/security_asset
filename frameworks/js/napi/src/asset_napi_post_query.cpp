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

#include "asset_napi_post_query.h"

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

const std::vector<uint32_t> REQUIRED_TAGS = {
    SEC_ASSET_TAG_AUTH_CHALLENGE
};

const std::vector<uint32_t> OPTIONAL_TAGS = {
    SEC_ASSET_TAG_GROUP_ID,
    SEC_ASSET_TAG_USER_ID
};

napi_status CheckPostQueryArgs(const napi_env env, const std::vector<AssetAttr> &attrs)
{
    IF_ERROR_THROW_RETURN(env, CheckAssetRequiredTag(env, attrs, REQUIRED_TAGS, SEC_ASSET_INVALID_ARGUMENT));
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), REQUIRED_TAGS.begin(), REQUIRED_TAGS.end());
    validTags.insert(validTags.end(), OPTIONAL_TAGS.begin(), OPTIONAL_TAGS.end());
    IF_ERROR_THROW_RETURN(env, CheckAssetValueValidity(env, attrs, SEC_ASSET_INVALID_ARGUMENT));
    return napi_ok;
}

napi_status ParseAttrMap(napi_env env, napi_callback_info info, BaseContext *context)
{
    napi_value argv[MAX_ARGS_NUM] = { 0 };
    IF_ERR_RETURN(ParseJsArgs(env, info, argv, QUERY_ARG_COUNT));
    IF_ERR_RETURN(ParseJsMap(env, argv[0], context->attrs));
    IF_ERR_RETURN(CheckPostQueryArgs(env, context->attrs));
    return napi_ok;
}

napi_status ParseAttrMapAsUser(napi_env env, napi_callback_info info, BaseContext *context)
{
    napi_value argv[MAX_ARGS_NUM] = { 0 };
    IF_ERR_RETURN(ParseJsArgs(env, info, argv, QUERY_ARG_COUNT_AS_USER));
    uint32_t index = 0;
    IF_ERR_RETURN(ParseJsUserId(env, argv[index++], context->attrs));
    IF_ERR_RETURN(ParseJsMap(env, argv[index++], context->attrs));
    IF_ERR_RETURN(CheckPostQueryArgs(env, context->attrs));
    return napi_ok;
}
} // anonymous namespace

napi_value NapiPostQuery(const napi_env env, napi_callback_info info, bool asUser, bool async)
{
    auto context = std::unique_ptr<BaseContext>(new (std::nothrow)BaseContext());
    NAPI_THROW(env, context == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for Context.");

    context->parse = asUser ? ParseAttrMapAsUser : ParseAttrMap;
    context->execute = [](napi_env env, void *data) {
        if (data == nullptr) {
            LOGE("data is nullptr.");
            return;
        }
        BaseContext *context = static_cast<BaseContext *>(data);
        if (context->attrs.empty()) {
            context->result = AssetPostQuery(nullptr, context->attrs.size());
            return;
        }
        context->result = AssetPostQuery(&context->attrs[0], context->attrs.size());
    };

    context->resolve = [](napi_env env, BaseContext *context) -> napi_value {
        return CreateJsUndefined(env);
    };

    if (async) {
        return CreateAsyncWork(env, info, std::move(context), __func__);
    } else {
        return CreateSyncWork(env, info, context.get());
    }
}

napi_value NapiPostQuery(const napi_env env, napi_callback_info info)
{
    return NapiPostQuery(env, info, false, true);
}

napi_value NapiPostQueryAsUser(const napi_env env, napi_callback_info info)
{
    return NapiPostQuery(env, info, true, true);
}

napi_value NapiPostQuerySync(const napi_env env, napi_callback_info info)
{
    return NapiPostQuery(env, info, false, false);
}

} // Asset
} // Security
} // OHOS
