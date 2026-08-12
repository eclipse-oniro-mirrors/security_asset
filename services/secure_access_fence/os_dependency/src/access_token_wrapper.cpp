/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "access_token_wrapper.h"

#include <cstring>
#include "securec.h"
#include <string>
#include <vector>

#include "accesstoken_kit.h"
#include "tokenid_kit.h"
#include "ipc_skeleton.h"

#include "saf_log.h"
#include "saf_result_code.h"

using namespace OHOS;
using namespace Security::AccessToken;
namespace {
bool CheckSystemApp()
{
    auto accessTokenId = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenId);
    if (isSystemApp) {
        LOGI("[INFO]Check system app success!");
        return true;
    } else {
        LOGE("[ERROR]Check system app failed!");
        return false;
    }
}
} // namespace

bool CheckPermission(const char *permission)
{
    if (permission == nullptr) {
        LOGE("[FATAL]Check permission failed, permission is nullptr");
        return false;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenId, permission);
    if (result == PERMISSION_GRANTED) {
        return true;
    } else {
        LOGE("[FATAL]Check permission failed, ret=%{public}d", result);
        return false;
    }
}

bool CheckIsSystemHap()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType == ATokenTypeEnum::TOKEN_INVALID) {
        return false;
    }
    return (tokenType == ATokenTypeEnum::TOKEN_HAP) ? CheckSystemApp() : true;
}

int32_t GetBundleNameFromTokenId(int32_t tokenId, char *bundleName, int32_t *len)
{
    if (tokenId < 0) {
        LOGE("[FATAL]GetBundleNameFromTokenId failed, tokenId is negative");
        return SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED;
    }
    if (bundleName == nullptr || len == nullptr) {
        LOGE("[FATAL]GetBundleNameFromTokenId failed, bundleName or len is nullptr");
        return SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED;
    }

    AccessTokenID accessTokenId = static_cast<AccessTokenID>(tokenId);
    HapTokenInfo hapTokenInfo;
    int result = AccessTokenKit::GetHapTokenInfo(accessTokenId, hapTokenInfo);
    if (result != RET_SUCCESS) {
        LOGE("[FATAL]GetHapTokenInfo failed, ret=%{public}d", result);
        return SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED;
    }

    const std::string &bundleNameStr = hapTokenInfo.bundleName;
    size_t bundleNameLen = bundleNameStr.size();
    if (bundleNameLen >= static_cast<size_t>(*len)) {
        LOGE("[FATAL]GetBundleNameFromTokenId failed, bundleName too long");
        return SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED;
    }

    if (memcpy_s(bundleName, *len, bundleNameStr.c_str(), bundleNameLen) != EOK) {
        LOGE("[FATAL]GetBundleNameFromTokenId failed, memcpy_s failed");
        return SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED;
    }
    bundleName[bundleNameLen] = '\0';
    *len = static_cast<int32_t>(bundleNameLen);

    LOGI("[INFO]GetBundleNameFromTokenId success, bundleName=%{public}s", bundleName);
    return SAF_SUCCESS;
}
