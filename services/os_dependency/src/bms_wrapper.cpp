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
#include "bundle_mgr_proxy.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "os_account_manager.h"
#include "system_ability_definition.h"

#include "asset_log.h"
#include "asset_mem.h"
#include "asset_type.h"

using namespace OHOS;
using namespace AppExecFwk;
using namespace Security::AccessToken;

uint32_t GetCallingTokenId()
{
    return IPCSkeleton::GetCallingTokenID();
}

static sptr<IBundleMgr> GetBundleMgrProxy()
{
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == NULL) {
        LOGE("[FATAL]Fail to get system ability manager.");
        return NULL;
    }

    sptr<IRemoteObject> remoteObject = saManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == NULL) {
        LOGE("[FATAL]Fail to get bundle manager service.");
        return NULL;
    }

    return iface_cast<IBundleMgr>(remoteObject);
}

static bool GetHapInfo(int32_t userId, uint32_t tokenId, std::string &info)
{
    HapTokenInfo tokenInfo;
    int32_t ret = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get hap token info failed, ret = %d", ret);
        return false;
    }

    sptr<IBundleMgr> bms = GetBundleMgrProxy();
    if (bms == NULL) {
        return false;
    }

    AppExecFwk::BundleInfo bundleInfo;
    if (!bms->GetBundleInfo(tokenInfo.bundleName, BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId)) {
        LOGE("[FATAL]Get bundle info failed!");
        return false;
    }

    info = bundleInfo.appId + "_" + std::to_string(bundleInfo.appIndex);
    return true;
}

static bool GetProcessInfo(uint32_t tokenId, uint64_t uid, std::string &info)
{
    NativeTokenInfo tokenInfo;
    int32_t ret = AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo);
    if (ret != RET_SUCCESS) {
        LOGE("[FATAL]Get native token info failed, ret = %d", ret);
        return false;
    }

    info = tokenInfo.processName + "_" + std::to_string(uid);
    return true;
}

bool GetOwnerInfo(int32_t userId, uint64_t uid, OwnerType *ownerType, char *ownerInfo, uint32_t *infoLen) {
    if (ownerType == NULL || ownerInfo == NULL || infoLen == NULL) {
        return false;
    }
    auto tokenId = GetCallingTokenId();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    std::string info;
    bool ret = false;
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP:
            *ownerType = HAP;
            ret = GetHapInfo(userId, tokenId, info);
            break;
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL:
            *ownerType = NATIVE;
            ret = GetProcessInfo(tokenId, uid, info);
            break;
        default:
            LOGE("[FATAL]Unsupported calling type: %d", tokenType);
    }

    if (ret && memcpy_s(ownerInfo, *infoLen, info.c_str(), info.size()) == EOK) {
        *infoLen = info.size();
        return true;
    }
    return false;
}
