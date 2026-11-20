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

#ifndef PRIVACY_COVERAGE_ISERVICE_REGISTRY_H
#define PRIVACY_COVERAGE_ISERVICE_REGISTRY_H

#include "bundle_mgr_interface.h"

namespace OHOS {
class PrivacySystemAbilityManager {
public:
    std::shared_ptr<PrivacyRemoteObject> GetSystemAbility(int32_t)
    {
        if (PrivacyHost::State().bundleManagerUnavailable) {
            return nullptr;
        }
        return std::shared_ptr<PrivacyRemoteObject>(new (std::nothrow) PrivacyRemoteObject());
    }
};

class SystemAbilityManagerClient {
public:
    static SystemAbilityManagerClient &GetInstance()
    {
        static SystemAbilityManagerClient client;
        return client;
    }

    std::shared_ptr<PrivacySystemAbilityManager> GetSystemAbilityManager()
    {
        if (PrivacyHost::State().systemAbilityManagerUnavailable) {
            return nullptr;
        }
        return std::shared_ptr<PrivacySystemAbilityManager>(new (std::nothrow) PrivacySystemAbilityManager());
    }
};
} // namespace OHOS
#endif
