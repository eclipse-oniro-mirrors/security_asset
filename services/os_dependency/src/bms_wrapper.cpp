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

int32_t GetBundleNameAndAppIndex(sptr<IBundleMgr> bundleMgr, uint64_t uid, ProcessInfo *processInfo)
{
    int32_t appIndex = 0;
    std::string bundleName;
    int32_t ret = bundleMgr->GetNameAndIndexForUid(uid, bundleName, appIndex);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]GetNameAndIndexForUid get bundleName and appIndex failed. ret:%{public}d", ret);
        return ASSET_BMS_ERROR;
    }

    processInfo->hapInfo.appIndex = appIndex;
    if (memcpy_s(processInfo->processName.data, processInfo->processName.size, bundleName.c_str(), bundleName.size())
        != EOK) {
        LOGE("[FATAL]The processName buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            bundleName.size(), processInfo->processName.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->processName.size = bundleName.size();

    return ASSET_SUCCESS;
}

int32_t GetBundleInfo(AppExecFwk::BundleMgrClient bmsClient, uint32_t userId, ProcessInfo *processInfo)
{
    AppExecFwk::BundleInfo bundleInfo;
    std::string bundleName(reinterpret_cast<const char*>(processInfo->processName.data), processInfo->processName.size);
    if (!bmsClient.GetBundleInfo(bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return ASSET_BMS_ERROR;
    }

    if (memcpy_s(processInfo->hapInfo.appId.data, processInfo->hapInfo.appId.size, bundleInfo.appId.c_str(),
        bundleInfo.appId.size()) != EOK) {
        LOGE("[FATAL]The app id buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            bundleInfo.appId.size(), processInfo->hapInfo.appId.size);
        return ASSET_OUT_OF_MEMORY;
    }
    processInfo->hapInfo.appId.size = bundleInfo.appId.size();

    if (processInfo->hapInfo.groupId.data == nullptr || processInfo->hapInfo.groupId.size == 0) {
        return ASSET_SUCCESS;
    }

    for (const std::string &groupId : bundleInfo.applicationInfo.assetAccessGroups) {
        if (groupId.size() <= processInfo->hapInfo.groupId.size &&
            memcmp(processInfo->hapInfo.groupId.data, groupId.data(), processInfo->hapInfo.groupId.size) == 0) {
            LOGI("[INFO]Found matching group id.");
            return ASSET_SUCCESS;
        }
    }
    LOGE("[FATAL]No matching group id found!");
    return ASSET_INVALID_ARGUMENT;
}

int32_t GetAppProvisionInfo(sptr<IBundleMgr> bundleMgr, uint32_t userId, ProcessInfo *processInfo)
{
    if (processInfo->hapInfo.developerId.data == nullptr || processInfo->hapInfo.developerId.size == 0) {
        return ASSET_SUCCESS;
    }

    AppExecFwk::AppProvisionInfo appProvisionInfo;
    std::string bundleName(reinterpret_cast<const char*>(processInfo->processName.data), processInfo->processName.size);
    int32_t ret = bundleMgr->GetAppProvisionInfo(bundleName, userId, appProvisionInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get app provision info failed!");
        return ASSET_BMS_ERROR;
    }

    if (memcpy_s(processInfo->hapInfo.developerId.data, processInfo->hapInfo.developerId.size,
        appProvisionInfo.developerId.c_str(), appProvisionInfo.developerId.size()) != EOK) {
        LOGE("[FATAL]The developer id buffer is too small. Expect size: %{public}zu, actual size: %{public}u",
            appProvisionInfo.developerId.size(), processInfo->hapInfo.developerId.size);
        return ASSET_OUT_OF_MEMORY;
    }

    return ASSET_SUCCESS;
}

int32_t GetHapProcessInfo(uint32_t userId, uint64_t uid, ProcessInfo *processInfo)
{
    auto bundleMgr = GetBundleMgr();
    if (bundleMgr == nullptr) {
        LOGE("[FATAL]bundleMgr is nullptr, please check.");
        return ASSET_BMS_ERROR;
    }
    AppExecFwk::BundleMgrClient bmsClient;

    int32_t ret = GetBundleNameAndAppIndex(bundleMgr, uid, processInfo);
    if (ret != ASSET_SUCCESS) {
        return ret;
    }

    ret = GetBundleInfo(bmsClient, userId, processInfo);
    if (ret != ASSET_SUCCESS) {
        return ret;
    }

    return GetAppProvisionInfo(bundleMgr, userId, processInfo);
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
            if (processInfo->hapInfo.groupId.data != nullptr) {
                processInfo->ownerType = GROUP;
            } else {
                processInfo->ownerType = HAP;
            }
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