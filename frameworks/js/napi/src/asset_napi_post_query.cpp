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
#include "asset_napi_post_query.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {

const std::vector<uint32_t> REQUIRED_TAGS = {
    SEC_ASSET_TAG_AUTH_CHALLENGE
};

const std::vector<uint32_t> OPTIONAL_TAGS = {
    SEC_ASSET_TAG_USER_ID
};

napi_status CheckPostQueryArgs(napi_env env, const std::vector<AssetAttr> &attrs)
{
    IF_FALSE_RETURN(CheckAssetRequiredTag(env, attrs, REQUIRED_TAGS), napi_invalid_arg);
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), REQUIRED_TAGS.begin(), REQUIRED_TAGS.end());
    validTags.insert(validTags.end(), OPTIONAL_TAGS.begin(), OPTIONAL_TAGS.end());
    IF_FALSE_RETURN(CheckAssetValueValidity(env, attrs), napi_invalid_arg);
    return napi_ok;
}

} // anonymous namespace

napi_value NapiPostQuery(napi_env env, napi_callback_info info, const NapiCallerArgs &args)
{
    napi_async_execute_callback execute =
        [](napi_env env, void *data) {
            AsyncContext *context = static_cast<AsyncContext *>(data);
            context->result = AssetPostQuery(&context->attrs[0], context->attrs.size());
        };
    return NapiAsync(env, info, __func__, execute, args);
}

napi_value NapiPostQuery(napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = NORMAL_ARGS_NUM, .isUpdate = false, .isAsUser = false,
        .checkFuncPtr = &CheckPostQueryArgs };
    return NapiPostQuery(env, info, args);
}

napi_value NapiPostQueryAsUser(napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = AS_USER_ARGS_NUM, .isUpdate = false, .isAsUser = true,
        .checkFuncPtr = &CheckPostQueryArgs };
    return NapiPostQuery(env, info, args);
}

napi_value NapiPostQuerySync(napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    do {
        if (ParseParam(env, info, attrs) != napi_ok) {
            break;
        }

        if (CheckPostQueryArgs(env, attrs) != napi_ok) {
            break;
        }

        int32_t result = AssetPostQuery(&attrs[0], attrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    return nullptr;
}

} // Asset
} // Security
} // OHOS
