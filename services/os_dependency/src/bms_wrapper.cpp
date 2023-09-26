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
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "hap_token_info.h"
#include "bundle_mgr_proxy.h"

#include "asset_log.h"
#include "asset_mem.h"
#include "os_account_manager.h"

using namespace OHOS;
using namespace Security::AccessToken;

bool GetCallingToken(uint32_t *tokenId)
{
    *tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    return true;
}

bool GetCallingOwnerType(uint32_t callingTokenId, int32_t *ownerType)
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

const char * GetCallingProcessName(uint32_t tokenId)
{
    // get process name
    NativeTokenInfo nativeTokenInfo;
    int32_t callingResult = AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo);
    LOGE("RET_SUCCESS val: %{public}i callingResult val: %{public}i", RET_SUCCESS, callingResult);
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

const char * GetHapOwnerInfo(uint32_t tokenId, int32_t userId)
{
    // get hap owner info
    HapTokenInfo hapTokenInfo;
    int32_t callingResult = AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    LOGE("RET_SUCCESS val: %{public}i", RET_SUCCESS);
    if (callingResult != RET_SUCCESS) {
        LOGE("Get hap info failed from access token kit.");
        return nullptr;
    }

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = GetBundleMgrProxy();
    if (bundleMgrProxy == NULL) {
        LOGE("bundle mgr proxy is nullptr.");
        return nullptr;
    }

    AppExecFwk::BundleInfo bundleInfo;
    const std::string bundleNameStr = hapTokenInfo.bundleName;
    bool isGetInfoSuccess = bundleMgrProxy->GetBundleInfo(bundleNameStr,
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId);
    if (!isGetInfoSuccess) {
        LOGE("GetBundleInfo failed.");
        return nullptr;
    }

    // The appid is concatenated from the bundle name and the developer's public key certificate.
    // transfer appid from string to char *
    int len = bundleInfo.appId.length();
    auto ownerInfo = static_cast<char *>(AssetMalloc((len + 1) * sizeof(char)));
    strcpy(ownerInfo, bundleInfo.appId.c_str());
    LOGE("ownerInfo val: %s", ownerInfo);
    return ownerInfo;
}


