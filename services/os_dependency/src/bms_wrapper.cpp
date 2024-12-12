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
int32_t GetFromHapTokenInfo(const HapTokenInfo hapTokenInfo, ProcessInfo *processInfo) {
    if (memcpy_s(processInfo->processName.data, processInfo->processName.size, hapTokenInfo.bundleName.c_str(),
        hapTokenInfo.bundleName.size()) != EOK) {
        LOGE("[FATAL]The processName buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            hapTokenInfo.bundleName.size(), processInfo->processName.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->processName.size = hapTokenInfo.bundleName.size();

    return ASSET_SUCCESS;
}

int32_t GetFromBundleInfo(const AppExecFwk::BundleInfo bundleInfo, ProcessInfo *processInfo) {
    if (memcpy_s(processInfo->hapInfo.appId.data, processInfo->hapInfo.appId.size, bundleInfo.appId.c_str(),
        bundleInfo.appId.size()) != EOK) {
        LOGE("[FATAL]The app id buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            bundleInfo.appId.size(), processInfo->hapInfo.appId.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->hapInfo.appId.size = bundleInfo.appId.size();

    if (processInfo->hapInfo.groupId.size != 0 && processInfo->hapInfo.developerId.size != 0) {
        if (bundleInfo.appIndex != 0) {
            LOGE("[FATAL]app with non-zero app index is not allowed to access groups!");
            return ASSET_PERMISSION_DENIED;
        }
        for (const AppExecFwk::AssetAccessGroups &group : bundleInfo.applicationInfo.assetAccessGroups) {
            if (std::memcmp(processInfo->hapInfo.groupId.data, group.assetGroupId.data(),
                processInfo->hapInfo.groupId.size) == 0) {
                LOGI("[INFO]Found matching group id.");
                if (memcpy_s(processInfo->hapInfo.developerId.data, processInfo->hapInfo.developerId.size,
                    group.developerId.c_str(), group.developerId.size()) != EOK) {
                    LOGE("[FATAL]The developer id buffer is too small. Expect size: %{public}zu, "
                    "actual size: %{public}u", group.developerId.size(), processInfo->hapInfo.developerId.size);
                    return ASSET_OUT_OF_MEMORY;
                }
                processInfo->hapInfo.developerId.size = group.developerId.size();
                return ASSET_SUCCESS;
            }
        }
        LOGE("[FATAL]No matching group id found!");
        return ASSET_PERMISSION_DENIED;
    }
    processInfo->hapInfo.appIndex = bundleInfo.appIndex;

    return ASSET_SUCCESS;
}

int32_t GetHapProcessInfo(int32_t userId, uint32_t tokenId, ProcessInfo *processInfo)
{
    HapTokenInfo hapTokenInfo;
    int32_t ret = AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get hap token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }
    ret = GetFromHapTokenInfo(hapTokenInfo, processInfo);
    if (ret != ASSET_SUCCESS) {
        return ret;
    }

    AppExecFwk::BundleMgrClient bmsClient;
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsClient.GetBundleInfo(hapTokenInfo.bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE,
        bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return ASSET_BMS_ERROR;
    }
    return GetFromBundleInfo(bundleInfo, processInfo);
}

int32_t GetNativeProcessInfo(uint32_t tokenId, uint64_t uid, ProcessInfo *processInfo)
{
    NativeTokenInfo nativeTokenInfo;
    int32_t ret = AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get native token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }

    if (memcpy_s(processInfo->processName.data, processInfo->processName.size, nativeTokenInfo.processName.c_str(),
        nativeTokenInfo.processName.size()) != EOK) {
        LOGE("[FATAL]The processName buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            nativeTokenInfo.processName.size(), processInfo->processName.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->processName.size = nativeTokenInfo.processName.size();
    processInfo->nativeInfo.uid = uid;
    return ASSET_SUCCESS;
}
} // namespace

int32_t GetCallingProcessInfo(uint32_t userId, uint64_t uid, ProcessInfo *processInfo)
{
    processInfo->userId = userId;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    int32_t res = ASSET_SUCCESS;
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP:
            processInfo->ownerType = HAP;
            res = GetHapProcessInfo(userId, tokenId, processInfo);
            break;
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL:
            processInfo->ownerType = NATIVE;
            res = GetNativeProcessInfo(tokenId, uid, processInfo);
            break;
        default:
            LOGE("[FATAL]Invalid calling type: %{public}d", tokenType);
            res = ASSET_INVALID_ARGUMENT;
    }
    return res;
}