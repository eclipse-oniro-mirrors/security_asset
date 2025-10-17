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

#include "asset_ani_update.h"

#include "asset_system_api.h"

#include "asset_api_check.h"
#include "asset_api_error_code.h"
#include "asset_ani_common.h"

using namespace OHOS::Security::Asset;

namespace OHOS {
namespace Security {
namespace Asset {

int32_t AniUpdateContext::Parse(ani_env *env, const ani_object &attributes, const ani_object &attributesToUpdate)
{
    int32_t ret = ParseAssetAttributeFromAni(env, attributes, attrs_);
    if (ret != SEC_ASSET_SUCCESS) {
        LOGE("Parse first map from ani failed.");
        return ret;
    }
    ret = ParseAssetAttributeFromAni(env, attributesToUpdate, updateAttrs_);
    if (ret != SEC_ASSET_SUCCESS) {
        LOGE("Parse second map from ani failed.");
    }
    return ret;
}

ani_object AniUpdateContext::Process(ani_env *env, const ani_object &attributes, const ani_object &attributesToUpdate)
{
    int32_t ret = Parse(env, attributes, attributesToUpdate);
    if (ret != SEC_ASSET_SUCCESS) {
        return GetResult(env, ret, GetErrorMessage(ret));
    }
    ret = Check(env);
    if (ret != SEC_ASSET_SUCCESS) {
        return GetResult(env, ret, errMsg_);
    }
    ret = Execute(env);
    return GetResult(env, ret, GetErrorMessage(ret));
}

ani_object AniUpdateContext::ProcessAsUser(ani_env *env, const ani_object &attributes,
    const ani_object &attributesToUpdate, int32_t userId)
{
    attrs_.emplace_back(AssetAttr{ .tag = SEC_ASSET_TAG_USER_ID, .value = { .u32 = userId }});
    return Process(env, attributes, attributesToUpdate);
}

int32_t AniUpdateContext::Check(ani_env *env)
{
    return CheckUpdateArgs(attrs_, updateAttrs_, AniGetError());
}

int32_t AniUpdateContext::Execute(ani_env *env)
{
    return AssetUpdate(&attrs_[0], attrs_.size(), &updateAttrs_[0], updateAttrs_.size());
}

ani_object AniUpdateContext::GetResult(ani_env *env, int32_t result, const char *errMsg)
{
    return CreateAniResult(env, result, errMsg, nullptr);
}

AniUpdateContext::~AniUpdateContext()
{
    FreeAssetAttrs(updateAttrs_);
}
} // Asset
} // Security
} // OHOS