/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "asset_type.h"
#include "asset_log.h"

using namespace OHOS;
using namespace AppExecFwk;
using namespace Security::AccessToken;

namespace {
int32_t GetHapInfo(int32_t userId, uint32_t tokenId, std::string &info)
{
    HapTokenInfo tokenInfo;
    int32_t ret = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get hap token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }

    AppExecFwk::BundleMgrClient bmsClient;
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsClient.GetBundleInfo(tokenInfo.bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return ASSET_BMS_ERROR;
    }

    info = bundleInfo.appId + "_" + std::to_string(bundleInfo.appIndex);
    return ASSET_SUCCESS;
}

int32_t GetProcessInfo(uint32_t tokenId, uint64_t uid, std::string &info)
{
    NativeTokenInfo tokenInfo;
    int32_t ret = AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get native token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }

    info = tokenInfo.processName + "_" + std::to_string(uid);
    return ASSET_SUCCESS;
}

int32_t GetHapBundleName(int32_t userId, uint32_t tokenId, uint8_t *name, uint32_t *nameLen, int32_t *appIndex)
{
    HapTokenInfo hapTokenInfo;
    int32_t ret = AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get hap token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }
    if (memcpy_s(name, *nameLen, hapTokenInfo.bundleName.c_str(), hapTokenInfo.bundleName.size()) != EOK) {
        LOGE("[FATAL]The name buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            hapTokenInfo.bundleName.size(), *nameLen);
        return ASSET_ACCESS_TOKEN_ERROR;
    }
    *nameLen = hapTokenInfo.bundleName.size();
    AppExecFwk::BundleMgrClient bmsClient;
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsClient.GetBundleInfo(hapTokenInfo.bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE,
        bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return ASSET_BMS_ERROR;
    }
    *appIndex = bundleInfo.appIndex;
    return ASSET_SUCCESS;
}

int32_t GetNativePackageName(uint32_t tokenId, uint8_t *name, uint32_t *nameLen)
{
    NativeTokenInfo nativeTokenInfo;
    int32_t ret = AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get native token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }
    if (memcpy_s(name, *nameLen, nativeTokenInfo.processName.c_str(),
        nativeTokenInfo.processName.size()) != EOK) {
        LOGE("[FATAL]The name buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            nativeTokenInfo.processName.size(), *nameLen);
        return ASSET_ACCESS_TOKEN_ERROR;
    }
    *nameLen = nativeTokenInfo.processName.size();
    return ASSET_SUCCESS;
}
} // namespace

int32_t GetOwnerInfo(int32_t userId, uint64_t uid, OwnerType *ownerType, uint8_t *ownerInfo, uint32_t *infoLen)
{
    if (ownerType == NULL || ownerInfo == NULL || infoLen == NULL) {
        return ASSET_INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    std::string info;
    int32_t code = ASSET_SUCCESS;
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
            LOGE("[FATAL]Invalid calling type: %{public}d", tokenType);
            code = ASSET_INVALID_ARGUMENT;
    }

    if (code != ASSET_SUCCESS) {
        return code;
    }

    if (memcpy_s(ownerInfo, *infoLen, info.c_str(), info.size()) != EOK) {
        LOGE("The owner buffer is too small. Expect size: %{public}zu, actual size: %{public}u", info.size(), *infoLen);
        return ASSET_INVALID_ARGUMENT;
    }

    *infoLen = info.size();
    return ASSET_SUCCESS;
}

int32_t GetCallingName(int32_t userId, uint8_t *name, uint32_t *nameLen, bool *isHap, int32_t *appIndex)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    int32_t code = ASSET_SUCCESS;
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP:
            *isHap = true;
            code = GetHapBundleName(userId, tokenId, name, nameLen, appIndex);
            break;
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL:
            *isHap = false;
            code = GetNativePackageName(tokenId, name, nameLen);
            break;
        default:
            LOGE("[FATAL]Invalid calling type: %{public}d", tokenType);
            code = ASSET_INVALID_ARGUMENT;
    }
    return code;
}