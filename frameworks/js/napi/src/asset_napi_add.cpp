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

#include "asset_napi_add.h"
#include "asset_napi_check.h"
#include "asset_napi_common.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {

const std::vector<uint32_t> REQUIRED_TAGS = {
    SEC_ASSET_TAG_SECRET,
    SEC_ASSET_TAG_ALIAS
};

const std::vector<uint32_t> OPTIONAL_TAGS = {
    SEC_ASSET_TAG_SECRET,
    SEC_ASSET_TAG_CONFLICT_RESOLUTION
};

napi_status CheckAddArgs(const napi_env env, const std::vector<AssetAttr> &attrs)
{
    IF_FALSE_RETURN(CheckAssetRequiredTag(env, attrs, REQUIRED_TAGS), napi_invalid_arg);
    std::vector<uint32_t> validTags;
    validTags.insert(validTags.end(), CRITICAL_LABEL_TAGS.begin(), CRITICAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LABEL_TAGS.begin(), NORMAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), NORMAL_LOCAL_LABEL_TAGS.begin(), NORMAL_LOCAL_LABEL_TAGS.end());
    validTags.insert(validTags.end(), ACCESS_CONTROL_TAGS.begin(), ACCESS_CONTROL_TAGS.end());
    validTags.insert(validTags.end(), ASSET_SYNC_TAGS.begin(), ASSET_SYNC_TAGS.end());
    validTags.insert(validTags.end(), OPTIONAL_TAGS.begin(), OPTIONAL_TAGS.end());
    validTags.insert(validTags.end(), ENCRYPTION_TAGS.begin(), ENCRYPTION_TAGS.end());
    IF_FALSE_RETURN(CheckAssetTagValidity(env, attrs, validTags), napi_invalid_arg);
    IF_FALSE_RETURN(CheckAssetValueValidity(env, attrs), napi_invalid_arg);
    return napi_ok;
}

} // anonymous namespace

napi_value NapiAdd(const napi_env env, napi_callback_info info, const NapiCallerArgs &args)
{
    napi_async_execute_callback execute = [](napi_env env, void *data) {
        AsyncContext *context = static_cast<AsyncContext *>(data);
        context->result = AssetAdd(&context->attrs[0], context->attrs.size());
    };
    return NapiAsync(env, info, execute, args, &CheckAddArgs);
}

napi_value NapiAdd(const napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = NORMAL_ARGS_NUM, .isUpdate = false, .isAsUser = false };
    return NapiAdd(env, info, args);
}

napi_value NapiAddAsUser(const napi_env env, napi_callback_info info)
{
    NapiCallerArgs args = { .expectArgNum = AS_USER_ARGS_NUM, .isUpdate = false, .isAsUser = true };
    return NapiAdd(env, info, args);
}

napi_value NapiAddSync(const napi_env env, napi_callback_info info)
{
    std::vector<AssetAttr> attrs;
    NapiCallerArgs args = { .expectArgNum = NORMAL_ARGS_NUM, .isUpdate = false, .isAsUser = false };
    do {
        if (ParseParam(env, info, args, attrs) != napi_ok) {
            break;
        }

        if (CheckAddArgs(env, attrs) != napi_ok) {
            break;
        }

        int32_t result = AssetAdd(&attrs[0], attrs.size());
        CHECK_RESULT_BREAK(env, result);
    } while (false);
    FreeAssetAttrs(attrs);
    return nullptr;
}

} // Asset
} // Security
} // OHOS
