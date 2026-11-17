/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Test-only stand-in for samgr's iservice_registry.h.
//
// See mock/isecure_access_fence.h for the rationale: the credential client is
// compiled into the test binary and its samgr access is redirected here so a
// test can script every GetProxy() branch (manager null, CheckSystemAbility
// hit/miss, LoadSystemAbility null/success).
//
// The definitions of SystemAbilityManagerClient live in the test .cpp.

#ifndef MOCK_ISERVICE_REGISTRY_H
#define MOCK_ISERVICE_REGISTRY_H

#include <cstdint>

#include "isecure_access_fence.h"

namespace OHOS {

/**
 * @brief Subset of ISystemAbilityManager used by the credential client.
 */
class ISystemAbilityManager {
public:
    virtual ~ISystemAbilityManager() = default;

    virtual sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) = 0;
    virtual sptr<IRemoteObject> LoadSystemAbility(int32_t systemAbilityId, int32_t timeoutSec) = 0;
};

/**
 * @brief Subset of SystemAbilityManagerClient used by the credential client.
 *
 * GetInstance() and GetSystemAbilityManager() are implemented by the test
 * fixture so each case controls the samgr state.
 */
class SystemAbilityManagerClient {
public:
    static SystemAbilityManagerClient &GetInstance();

    sptr<ISystemAbilityManager> GetSystemAbilityManager();
};

}  // namespace OHOS

#endif  // MOCK_ISERVICE_REGISTRY_H