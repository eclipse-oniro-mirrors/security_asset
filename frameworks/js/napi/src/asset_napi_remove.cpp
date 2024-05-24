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

#include <vector>
#include <cstdint>

#include "securec.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "asset_log.h"
#include "asset_mem.h"
#include "asset_system_api.h"
#include "asset_system_type.h"

#include "asset_napi_check.h"
#include "asset_napi_common.h"
#include "asset_napi_remove.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {

const std::vector<uint32_t> OPTIONAL_TAGS = {
    SEC_ASSET_TAG_USER_ID
};

napi_status CheckRemoveArgs(napi_env env, const std::vector<AssetAttr> &attrs)
{
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    validTags.insert(validTags.end(), OPTIONAL_TAGS.begin(), OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(env, attrs, validTags), napi_invalid_arg);
    IF_FALSE_RETURN(CheckAssetValueValidity(env, attrs), napi_invalid_arg);
    return napi_ok;
}

} // anonymous namespace

napi_value NapiRemove(napi_env env, napi_callback_info info, const NapiCallerArgs &args)
{
    AsyncContext *context = CreateAsyncContext();
    NAPI_THROW(env, context == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for AsyncContext.");

    do {
        if (ParseParam(env, info, args, context->attrs, context->updateAttrs) != napi_ok) {
            break;
        }

        if (CheckRemoveArgs(env, context->attrs) != napi_ok) {
            break;
        }

        napi_async_execute_callback execute = [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetRemove(&context->attrs[0], context->attrs.size());
        };

        napi_value promise = CreateAsyncWork(env, context, __func__, execute);
        if (promise == nullptr) {
            LOGE("Create async work failed.");
            break;
        }
        return promise;
    } while (0);
    DestroyAsyncContext(env, context);
    return nullptr;
}

napi_value NapiRemove(napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = NORMAL_ARGS_NUM, .isUpdate = false, .isAsUser = false };
    return NapiRemove(env, info, args);
}

napi_value NapiRemoveAsUser(napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = NORMAL_ARGS_NUM, .isUpdate = false, .isAsUser = true };
    return NapiRemove(env, info, args);
}

napi_value NapiRemoveSync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    std::vector<AssetAttr> updateAttrs;
    do {
        NapiCallerArgs args = { .expectArgNum = NORMAL_ARGS_NUM, .isUpdate = false, .isAsUser = false };
        if (ParseParam(env, info, args, attrs, updateAttrs) != napi_ok) {
            break;
        }

        if (CheckRemoveArgs(env, attrs) != napi_ok) {
            break;
        }

        int32_t result = AssetRemove(&attrs[0], attrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    return nullptr;
}

} // Asset
} // Security
} // OHOS
