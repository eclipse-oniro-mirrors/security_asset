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
#include "bundle_mgr_interface.h"
#include "iservice_registry.h"

#include <cstring>
#include "securec.h"

#include "accesstoken_kit.h"
#include "app_provision_info.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_interface.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"

#include "asset_type.h"
#include "asset_log.h"

using namespace OHOS;
using namespace AppExecFwk;
using namespace Security::AccessToken;

namespace {
constexpr int BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;

sptr<IBundleMgr> GetBundleMgr()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        LOGE("[FATAL]systemAbilityManager is nullptr, please check.");
        return nullptr;
    }
    auto bundleMgrRemoteObj = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrRemoteObj == nullptr) {
        LOGE("[FATAL]bundleMgrRemoteObj is nullptr, please check.");
        return nullptr;
    }
    return iface_cast<IBundleMgr>(bundleMgrRemoteObj);
}

int32_t FillProcessInfoWithHapInfo(int32_t appIndex, const AppExecFwk::AppProvisionInfo &appProvisionInfo,
    const AppExecFwk::BundleInfo &bundleInfo, const std::string &bundleName, ProcessInfo *processInfo)
{
    if (memcpy_s(processInfo->processName.data, processInfo->processName.size, bundleName.c_str(), bundleName.size())
        != EOK) {
        LOGE("[FATAL]The processName buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            bundleName.size(), processInfo->processName.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->processName.size = bundleName.size();

    if (memcpy_s(processInfo->hapInfo.appId.data, processInfo->hapInfo.appId.size, bundleInfo.appId.c_str(),
        bundleInfo.appId.size()) != EOK) {
        LOGE("[FATAL]The app id buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            bundleInfo.appId.size(), processInfo->hapInfo.appId.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->hapInfo.appId.size = bundleInfo.appId.size();

    processInfo->hapInfo.appIndex = appIndex;
    if (processInfo->hapInfo.groupId.data == nullptr || processInfo->hapInfo.groupId.size == 0 ||
        processInfo->hapInfo.developerId.data == nullptr || processInfo->hapInfo.developerId.size == 0) {
        return ASSET_SUCCESS;
    }
    if (processInfo->hapInfo.appIndex != 0) {
        LOGE("[FATAL]App with non-zero app index is not allowed to access groups!");
        return ASSET_PERMISSION_DENIED;
    }
    for (const std::string &groupId : bundleInfo.applicationInfo.assetAccessGroups) {
        if (groupId.size() <= processInfo->hapInfo.groupId.size &&
            memcmp(processInfo->hapInfo.groupId.data, groupId.data(), processInfo->hapInfo.groupId.size) == 0) {
            LOGI("[INFO]Found matching group id.");
            if (memcpy_s(processInfo->hapInfo.developerId.data, processInfo->hapInfo.developerId.size,
                appProvisionInfo.developerId.c_str(), appProvisionInfo.developerId.size()) != EOK) {
                LOGE("[FATAL]The developer id buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
                    appProvisionInfo.developerId.size(), processInfo->hapInfo.developerId.size);
                return ASSET_OUT_OF_MEMORY;
            }
            processInfo->hapInfo.developerId.size = appProvisionInfo.developerId.size();
            return ASSET_SUCCESS;
        }
    }
    LOGE("[FATAL]No matching group id found!");
    return ASSET_INVALID_ARGUMENT;
}

int32_t FillProcessInfoWithNativeInfo(const NativeTokenInfo &nativeTokenInfo, uint64_t uid, ProcessInfo *processInfo)
{
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

int32_t GetHapProcessInfo(uint32_t userId, uint64_t uid, ProcessInfo *processInfo)
{
    // Get bundle name and app index
    auto bundleMgr = GetBundleMgr();
    if (bundleMgr == nullptr) {
        LOGE("[FATAL]bundleMgr is nullptr, please check.");
        return ASSET_BMS_ERROR;
    }
    int32_t appIndex = 0;
    std::string bundleName;
    int32_t ret = bundleMgr->GetNameAndIndexForUid(uid, bundleName, appIndex);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]GetNameAndIndexForUid get bundleName and appIndex failed. ret:%{public}d", ret);
        return ASSET_BMS_ERROR;
    }

    // Get bundle info
    AppExecFwk::BundleMgrClient bmsClient;
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsClient.GetBundleInfo(bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return ASSET_BMS_ERROR;
    }

    // Get app provision info
    AppExecFwk::AppProvisionInfo appProvisionInfo;
    ret = bundleMgr->GetAppProvisionInfo(bundleName, userId, appProvisionInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get app provision info failed!");
        return ASSET_BMS_ERROR;
    }

    return FillProcessInfoWithHapInfo(appIndex, appProvisionInfo, bundleInfo, bundleName, processInfo);
}

int32_t GetNativeProcessInfo(uint32_t tokenId, uint64_t uid, ProcessInfo *processInfo)
{
    // Get native token info
    NativeTokenInfo nativeTokenInfo;
    int32_t ret = AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get native token info failed, ret = %{public}d", ret);
        return ASSET_ACCESS_TOKEN_ERROR;
    }

    return FillProcessInfoWithNativeInfo(nativeTokenInfo, uid, processInfo);
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
            res = GetHapProcessInfo(userId, uid, processInfo);
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