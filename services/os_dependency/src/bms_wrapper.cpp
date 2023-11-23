/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "bms_wrapper.h"

#include <cstring>
#include "securec.h"

#include "accesstoken_kit.h"
#include "bundle_mgr_client.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"

#include "asset_log.h"

using namespace OHOS;
using namespace AppExecFwk;
using namespace Security::AccessToken;

namespace {
ResultCode GetHapInfo(int32_t userId, uint32_t tokenId, std::string &info)
{
    HapTokenInfo tokenInfo;
    int32_t ret = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get hap token info failed, ret = %{public}d", ret);
        return ACCESS_TOKEN_ERROR;
    }

    AppExecFwk::BundleMgrClient bmsClient;
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsClient.GetBundleInfo(tokenInfo.bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return BMS_ERROR;
    }

    info = bundleInfo.appId + "_" + std::to_string(bundleInfo.appIndex);
    return SUCCESS;
}

ResultCode GetProcessInfo(uint32_t tokenId, uint64_t uid, std::string &info)
{
    NativeTokenInfo tokenInfo;
    int32_t ret = AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get native token info failed, ret = %{public}d", ret);
        return ACCESS_TOKEN_ERROR;
    }

    info = tokenInfo.processName + "_" + std::to_string(uid);
    return SUCCESS;
}
} // namespace

ResultCode GetOwnerInfo(int32_t userId, uint64_t uid, OwnerType *ownerType, uint8_t *ownerInfo, uint32_t *infoLen)
{
    if (ownerType == NULL || ownerInfo == NULL || infoLen == NULL) {
        return INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    std::string info;
    ResultCode code = SUCCESS;
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP:
            *ownerType = HAP;
            code = GetHapInfo(userId, tokenId, info);
            break;
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL:
            *ownerType = NATIVE;
            code = GetProcessInfo(tokenId, uid, info);
            break;
        default:
            LOGE("[FATAL]Unsupported calling type: %{public}d", tokenType);
            code = UNSUPPORTED;
    }

    if (code != SUCCESS) {
        return code;
    }

    if (memcpy_s(ownerInfo, *infoLen, info.c_str(), info.size()) != EOK) {
        LOGE("The owner buffer is too small. Expect size: %{public}zu, actual size: %{public}u", info.size(), *infoLen);
        return INVALID_ARGUMENT;
    }

    *infoLen = info.size();
    return SUCCESS;
}

bool PermissionCheck(void)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenId,
        "ohos.permission.STORE_PERSISTENT_DATA");
    if (result == PERMISSION_GRANTED) {
        LOGI("[INFO]Check Permission success!");
        return true;
    } else {
        LOGI("[INFO]Check Permission failed!");
        return false;
    }
}
