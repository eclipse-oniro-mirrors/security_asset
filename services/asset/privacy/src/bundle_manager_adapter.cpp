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

#include "bundle_manager_adapter.h"
#include "bundle_mgr_interface.h"
#include "iservice_registry.h"
#include "privacy_error_code.h"
#include "privacy_log.h"
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
constexpr int32_t BUNDLE_MANAGER_SA_ID = 401;
} // namespace

int32_t GetCallerAppId(uint32_t uid, std::string &appId)
{
    const auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) {
        LOGE("Privacy system ability manager is unavailable");
        return PRIVACY_PROTOCOL_INTERNAL_ERROR;
    }
    const auto remote = manager->GetSystemAbility(BUNDLE_MANAGER_SA_ID);
    if (remote == nullptr) {
        LOGE("Privacy bundle manager service is unavailable");
        return PRIVACY_PROTOCOL_INTERNAL_ERROR;
    }
    const auto bundleManager = iface_cast<AppExecFwk::IBundleMgr>(remote);
    if (bundleManager == nullptr) {
        LOGE("Privacy bundle manager interface is unavailable");
        return PRIVACY_PROTOCOL_INTERNAL_ERROR;
    }
    AppExecFwk::AssetGroupInfo groupInfo;
    const auto ret = bundleManager->GetAssetGroupsInfo(uid, groupInfo);
    if (ret != 0 || groupInfo.appId.empty()) {
        LOGE("Resolve privacy caller app ID failed, ret=%{public}d", ret);
        return PRIVACY_PROTOCOL_INTERNAL_ERROR;
    }
    appId = std::move(groupInfo.appId);
    return PRIVACY_PROTOCOL_SUCCESS;
}
} // namespace OHOS::Security::PrivacyComputation
