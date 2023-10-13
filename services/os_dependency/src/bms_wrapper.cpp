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

#include "accesstoken_kit.h"
#include "asset_log.h"
#include "asset_mem.h"
#include "bundle_mgr_proxy.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "os_account_manager.h"
#include "system_ability_definition.h"

#include "securec.h"

using namespace OHOS;
using namespace Security::AccessToken;

bool GetCallingTokenId(uint32_t *tokenId)
{
    *tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    return true;
}

bool GetCallingOwnerType(uint32_t callingTokenId, int32_t *ownerType) // todo: 直接返回type, 外面函数校验
{
    // get token type
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenType(callingTokenId);
    if ((tokenType != ATokenTypeEnum::TOKEN_HAP) && (tokenType != ATokenTypeEnum::TOKEN_NATIVE) && (tokenType != ATokenTypeEnum::TOKEN_SHELL)) {
        LOGE("get wrong token type %{public}i", tokenType);
        return false;
    }
    *ownerType = static_cast<int32_t>(tokenType);
    return true;
}

static sptr<AppExecFwk::IBundleMgr> GetBundleMgrProxy()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager==NULL) {
        LOGE("fail to get system ability mgr.");
        return NULL;
    }

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject==NULL) {
        LOGE("system ability d is nullptr");
        return NULL;
    }

    return iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
}

const char *GetCallingProcessName(uint32_t tokenId)
{
    // get process name
    NativeTokenInfo nativeTokenInfo;
    int32_t callingResult = AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo);
    if (callingResult != RET_SUCCESS) {
        LOGE("Get native info failed from access token kit.");
        return nullptr;
    }
    int len = nativeTokenInfo.processName.length();
    auto process_name = static_cast<char *>(AssetMalloc((len + 1) * sizeof(char)));
    strcpy(process_name, nativeTokenInfo.processName.c_str());
    LOGE("process name value: %s", process_name);
    return process_name;
}

bool GetHapOwnerInfo(uint32_t tokenId, int32_t userId, char** appId, int32_t *appIndex)
{
    // get hap owner info
    HapTokenInfo hapTokenInfo;
    int32_t callingResult = AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (callingResult != RET_SUCCESS) {
        LOGE("Get hap info failed from access token kit.");
        return false;
    }

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = GetBundleMgrProxy();
    if (bundleMgrProxy == NULL) {
        LOGE("bundle mgr proxy is nullptr.");
        return false;
    }

    AppExecFwk::BundleInfo bundleInfo;
    const std::string bundleNameStr = hapTokenInfo.bundleName;
    LOGE("use bundle name is : %{public}s", bundleNameStr.c_str());  // todo 后续删掉
    bool isGetInfoSuccess = bundleMgrProxy->GetBundleInfo(bundleNameStr,
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId);
    if (!isGetInfoSuccess) {
        LOGE("GetBundleInfo failed.");
        return false;
    }

    // The appid is concatenated from the bundle name and the developer's public key certificate.
    // transfer appid from string to char *
    int len = bundleInfo.appId.length();
    auto ownerInfo = static_cast<char *>(AssetMalloc((len + 1) * sizeof(char)));
    strcpy(ownerInfo, bundleInfo.appId.c_str());

    LOGE("ownerInfo val: %{public}s", ownerInfo);

    *appId = ownerInfo;
    *appIndex = bundleInfo.appIndex;
    return true;
}


bool GetCallingOwnerInfo(int64_t uid, int32_t userId, uint32_t *ownerType, char** ownerInfo) {
    // 1 get calling token id
    auto callingTokenId = IPCSkeleton::GetCallingTokenID();

    // 2 find this calling onwer type
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenType(callingTokenId);
    if (tokenType == ATokenTypeEnum::TOKEN_HAP) {
        // use owner type to construct hap owner info
        int *appIndex = NULL;
        char **appId = NULL;
        bool isGetInfoSuccess = GetHapOwnerInfo(callingTokenId, userId, appId, appIndex);
        if (!isGetInfoSuccess) {
            return false;
        }
        // use appid and appindex to construct owner info
        int len = 0;
        while (*appId[len] != '\0') {
            len ++;
        }
        std::string appIndexStr = std::to_string(*appIndex);
        len += appIndexStr.length();
        auto tmpOwnerInfo = static_cast<char *>(AssetMalloc((len + 1) * sizeof(char)));
        strcpy(tmpOwnerInfo, *appId);
        strcat(tmpOwnerInfo, appIndexStr.c_str());
        // release appId
        AssetFree(*appId);
        *ownerInfo = tmpOwnerInfo;
    } else if (tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL) {
        // use owner type to construct native owner info
        const char *processName = GetCallingProcessName(callingTokenId);
        int len = 0;
        while (processName[len] != '\0') {
            len ++;
        }
        std::string uidStr = std::to_string(uid);
        len += uidStr.length();
        auto tmpOwnerInfo = static_cast<char *>(AssetMalloc((len + 1) * sizeof(char)));
        strcpy(tmpOwnerInfo, uidStr.c_str());
        strcat(tmpOwnerInfo, processName);
        // release processName
        AssetFree((void *)processName);
        *ownerInfo = tmpOwnerInfo;
    } else {
        LOGE("get wrong token type %{public}i", tokenType);
        return false;
    }

    *ownerType = static_cast<int32_t>(tokenType);
    return true;
}

void FreeMemory(const char* freeStr) {
    AssetFree((void *)freeStr);
}