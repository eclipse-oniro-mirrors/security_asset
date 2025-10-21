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

#include "asset_ani_query_sync_result.h"

#include "asset_system_api.h"

#include "asset_api_check.h"
#include "asset_api_error_code.h"
#include "asset_ani_common.h"

using namespace OHOS::Security::Asset;

namespace OHOS {
namespace Security {
namespace Asset {

int32_t AniQuerySyncResultContext::Check(ani_env *env)
{
    return CheckQuerySyncResultArgs(attrs_, AniGetError());
}

int32_t AniQuerySyncResultContext::Execute(ani_env *env)
{
    return AssetQuerySyncResult(&attrs_[0], attrs_.size(), &syncResult_);
}

ani_object AniQuerySyncResultContext::GetResult(ani_env *env, int32_t result, const char *errMsg)
{
    if (result != SEC_ASSET_SUCCESS) {
        return CreateAniError(env, result, errMsg);
    }

    ani_object syncResultObj = nullptr;
    if (!CreateAniSyncResult(env, syncResult_, syncResultObj)) {
        result = SEC_ASSET_INVALID_ARGUMENT;
        LOGE("CreateAniSyncResult failed, ret = %{public}d", result);
        return CreateAniError(env, result, GetErrorMessage(result));
    }
    return CreateAniResult(env, result, errMsg, syncResultObj);
}

AniQuerySyncResultContext::~AniQuerySyncResultContext() {}
} // Asset
} // Security
} // OHOS