/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <vector>

#include "securec.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_log.h"
#include "asset_mem.h"
#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_napi_check.h"
#include "asset_napi_common.h"
#include "asset_napi_update.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {

const std::vector<uint32_t> QUERY_REQUIRED_TAGS = {
    SEC_ASSET_TAG_ALIAS
};

const std::vector<uint32_t> UPDATE_OPTIONAL_TAGS = {
    SEC_ASSET_TAG_SECRET
};

bool CheckAssetPresence(const napi_env env, const std::vector<AssetAttr> &attrs)
{
    if (attrs.empty()) {
        NAPI_THROW_INVALID_ARGUMENT(env, "Argument[attributesToUpdate] is empty.");
        return false;
    }
    return true;
}

napi_status CheckUpdateArgs(const napi_env env, const std::vector<AssetAttr> &attrs,
    const std::vector<AssetAttr> &updateAttrs)
{
    IF_FALSE_RETURN(CheckAssetRequiredTag(env, attrs, QUERY_REQUIRED_TAGS), napi_invalid_arg);
    std::vector<uint32_t> queryValidTags;
    queryValidTags.insert(queryValidTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    queryValidTags.insert(queryValidTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(env, attrs, queryValidTags), napi_invalid_arg);
    IF_FALSE_RETURN(CheckAssetValueValidity(env, attrs), napi_invalid_arg);

    IF_FALSE_RETURN(CheckAssetPresence(env, updateAttrs), napi_invalid_arg);
    std::vector<uint32_t> updateValidTags;
    updateValidTags.insert(updateValidTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    updateValidTags.insert(updateValidTags.end(), UPDATE_OPTIONAL_TAGS.begin(), UPDATE_OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(env, updateAttrs, updateValidTags), napi_invalid_arg);
    IF_FALSE_RETURN(CheckAssetValueValidity(env, updateAttrs), napi_invalid_arg);

    return napi_ok;
}

napi_value NapiUpdateAsync(const napi_env env, napi_callback_info info, const char *funcName,
    napi_async_execute_callback execute, const NapiCallerArgs &args)
{
    AsyncContext *context = CreateAsyncContext();
    NAPI_THROW(env, context == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for AsyncContext.");

    do {
        if (ParseParam(env, info, args, context->attrs, context->updateAttrs) != napi_ok) {
            break;
        }

        if (CheckUpdateArgs(env, context->attrs, context->updateAttrs) != napi_ok) {
            break;
        }

        napi_value promise = CreateAsyncWork(env, context, funcName, execute);
        if (promise == nullptr) {
            LOGE("Create async work failed.");
            break;
        }
        return promise;
    } while (0);
    DestroyAsyncContext(env, context);
    return nullptr;
}

} // anonymous namespace

napi_value NapiUpdate(const napi_env env, napi_callback_info info, const NapiCallerArgs &args)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetUpdate(&context->attrs[0], context->attrs.size(),
                &context->updateAttrs[0], context->updateAttrs.size());
        };
    return NapiUpdateAsync(env, info, __func__, execute, args);
}

napi_value NapiUpdate(const napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = UPDATE_ARGS_NUM, .isUpdate = true, .isAsUser = false };
    return NapiUpdate(env, info, args);
}

napi_value NapiUpdateAsUser(const napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = AS_USER_UPDATE_ARGS_NUM, .isUpdate = true, .isAsUser = true };
    return NapiUpdate(env, info, args);
}

napi_value NapiUpdateSync(const napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    std::vector<AssetAttr> updateAttrs;
    NapiCallerArgs args = { .expectArgNum = UPDATE_ARGS_NUM, .isUpdate = true, .isAsUser = false };
    do {
        if (ParseParam(env, info, args, attrs, updateAttrs) != napi_ok) {
            break;
        }

        if (CheckUpdateArgs(env, attrs, updateAttrs) != napi_ok) {
            break;
        }

        int32_t result = AssetUpdate(&attrs[0], attrs.size(), &updateAttrs[0], updateAttrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    FreeAssetAttrs(updateAttrs);
    return nullptr;
}

} // Asset
} // Security
} // OHOS
