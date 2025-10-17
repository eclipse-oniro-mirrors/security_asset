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

#include "asset_ani_base.h"

#include "securec.h"

#include "asset_system_api.h"

#include "asset_ani_common.h"
#include "asset_api_check.h"
#include "asset_api_error_code.h"
#include "asset_system_api.h"

namespace OHOS {
namespace Security {
namespace Asset {

int32_t AniBaseContext::Parse(ani_env *env, const ani_object &attributes)
{
    return ParseAssetAttributeFromAni(env, attributes, attrs_);
}

ani_object AniBaseContext::Process(ani_env *env, const ani_object &attributes)
{
    int32_t ret = Parse(env, attributes);
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

ani_object AniBaseContext::ProcessAsUser(ani_env *env, const ani_object &attributes, int32_t userId)
{
    attrs_.emplace_back(AssetAttr{ .tag = SEC_ASSET_TAG_USER_ID, .value = { .u32 = userId }});
    return Process(env, attributes);
}

std::function<void(char *)> AniBaseContext::AniGetError()
{
    return [this](char *errMsg) {
        (void)strcpy_s(errMsg_, MAX_MESSAGE_LEN, errMsg);
    };
}

AniBaseContext::~AniBaseContext()
{
    FreeAssetAttrs(attrs_);
}
} // Asset
} // Security
} // OHOS