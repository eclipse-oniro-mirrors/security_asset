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

#include "asset_napi_update.h"

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
const uint32_t UPDATE_ARG_COUNT = 2;
const uint32_t UPDATE_ARG_COUNT_AS_USER = 3;

const std::vector<uint32_t> QUERY_REQUIRED_TAGS = {
    SEC_ASSET_TAG_ALIAS
};

const std::vector<uint32_t> UPDATE_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_SECRET
};

napi_value CheckAssetPresence(const napi_env env, const std::vector<AssetAttr> &attrs)
{
    if (attrs.empty()) {
        RETURN_JS_ERROR(env, SEC_ASSET_INVALID_ARGUMENT, "Argument[attributesToUpdate] is empty.");
    }
    return nullptr;
}

napi_status CheckUpdateArgs(const napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<AssetAttr> &updateAttrs)
{
    IF_ERROR_THROW_RETURN(env, CheckAssetRequiredTag(env, attrs, QUERY_REQUIRED_TAGS, SEC_ASSET_INVALID_ARGUMENT));
    std::vector<uint32_t> queryValidTags;
    queryValidTags.insert(queryValidTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    IF_ERROR_THROW_RETURN(env, CheckAssetTagValidity(env, attrs, queryValidTags, SEC_ASSET_INVALID_ARGUMENT));
    IF_ERROR_THROW_RETURN(env, CheckAssetValueValidity(env, attrs, SEC_ASSET_INVALID_ARGUMENT));

    IF_ERROR_THROW_RETURN(env, CheckAssetPresence(env, updateAttrs));
    std::vector<uint32_t> updateValidTags;
    updateValidTags.insert(updateValidTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), UPDATE_OPTIONAL_TAGS.begin(), UPDATE_OPTIONAL_TAGS.end());
    IF_ERROR_THROW_RETURN(env, CheckAssetTagValidity(env, updateAttrs, updateValidTags, SEC_ASSET_INVALID_ARGUMENT));
    IF_ERROR_THROW_RETURN(env, CheckAssetValueValidity(env, updateAttrs, SEC_ASSET_INVALID_ARGUMENT));

    return napi_ok;
}

napi_status ParseAttrMap(napi_env env, napi_callback_info info, BaseContext *baseContext)
{
    UpdateContext *context = reinterpret_cast<UpdateContext *>(baseContext);
    napi_value argv[MAX_ARGS_NUM] = { 0 };
    IF_ERR_RETURN(ParseJsArgs(env, info, argv, UPDATE_ARG_COUNT));
    uint32_t index = 0;
    IF_ERR_RETURN(ParseJsMap(env, argv[index++], context->attrs));
    IF_ERR_RETURN(ParseJsMap(env, argv[index++], context->updateAttrs));
    IF_ERR_RETURN(CheckUpdateArgs(env, context->attrs, context->updateAttrs));
    return napi_ok;
}

napi_status ParseAttrMapAsUser(napi_env env, napi_callback_info info, BaseContext *baseContext)
{
    UpdateContext *context = reinterpret_cast<UpdateContext *>(baseContext);
    napi_value argv[MAX_ARGS_NUM] = { 0 };
    IF_ERR_RETURN(ParseJsArgs(env, info, argv, UPDATE_ARG_COUNT_AS_USER));
    uint32_t index = 0;
    IF_ERR_RETURN(ParseJsUserId(env, argv[index++], context->attrs));
    IF_ERR_RETURN(ParseJsMap(env, argv[index++], context->attrs));
    IF_ERR_RETURN(ParseJsMap(env, argv[index++], context->updateAttrs));
    IF_ERR_RETURN(CheckUpdateArgs(env, context->attrs, context->updateAttrs));
    return napi_ok;
}
} // anonymous namespace

napi_value NapiUpdate(const napi_env env, napi_callback_info info, bool asUser, bool async)
{
    auto context = std::unique_ptr<UpdateContext>(new (std::nothrow)UpdateContext());
    NAPI_THROW(env, context == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for Context.");

    context->parse = asUser ? ParseAttrMapAsUser : ParseAttrMap;
    context->execute = [](napi_env env, void *data) {
        if (data == nullptr) {
            LOGE("data is nullptr.");
            return;
        }
        UpdateContext *context = static_cast<UpdateContext *>(data);
        context->result = AssetUpdate(&context->attrs[0], context->attrs.size(),
            &context->updateAttrs[0], context->updateAttrs.size());
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

napi_value NapiUpdate(const napi_env env, napi_callback_info info)
{
    return NapiUpdate(env, info, false, true);
}

napi_value NapiUpdateAsUser(const napi_env env, napi_callback_info info)
{
    return NapiUpdate(env, info, true, true);
}

napi_value NapiUpdateSync(const napi_env env, napi_callback_info info)
{
    return NapiUpdate(env, info, false, false);
}

} // Asset
} // Security
} // OHOS
