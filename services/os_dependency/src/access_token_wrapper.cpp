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

#include "access_token_wrapper.h"

#include <cstring>
#include "securec.h"
#include <string>
#include <vector>

#include "accesstoken_kit.h"
#include "tokenid_kit.h"
#include "ipc_skeleton.h"

#include "asset_type.h"
#include "asset_log.h"
#ifdef ASSET_ENABLED_CLOUD_AUTH_SERVICE
#include "cloud_auth_service_client.h"
#include "common_defined.h"

const char * const CLOUD_AUTH_SERVICE_NAME = ASSET_CLOUD_AUTH_SERVICE_NAME;
#endif

using namespace OHOS;
using namespace Security::AccessToken;

namespace {
bool CheckSystemApp(void)
{
    auto accessTokenId = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenId);
    if (isSystemApp) {
        LOGI("[INFO]Check system app success!");
        return true;
    } else {
        LOGE("[FATAL]Check system app failed");
        return false;
    }
}

} // namespace

bool CheckPermission(const char *permission)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenId, permission);
    if (result == PERMISSION_GRANTED) {
        return true;
    } else {
        LOGE("[FATAL]Check permission failed, ret=%{public}d", result);
        return false;
    }
}

bool CheckSystemHapPermission(void)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    return (tokenType == ATokenTypeEnum::TOKEN_HAP) ? CheckSystemApp() : true;
}

bool IsPermissionEnabled(void)
{
#ifdef ASSET_ENABLED_CLOUD_AUTH_SERVICE
    auto callerUid = OHOS::IPCSkeleton::GetCallingUid();
    std::string bundleNameStr(CLOUD_AUTH_SERVICE_NAME);
    const std::vector<std::string> permission = {bundleNameStr};
    std::vector<int32_t> permissionResult = {CloudAuth::UNGRANTED};
    auto result = CloudAuth::CloudAuthServiceClient::GetInstance().VerifyCloudCapability(
        static_cast<int32_t>(callerUid),
        permission,
        permissionResult);
    if (result != ERR_OK) {
        LOGE("Verify capability failed. Result = %{public}d", result);
        return false;
    }
    if (permissionResult.size() != permission.size()) {
        LOGE("Invalid permission result! Result size is %{public}lu", permissionResult.size());
        return false;
    }
    return permissionResult.front() == CloudAuth::GRANTED;
#else
    return false;
#endif
}