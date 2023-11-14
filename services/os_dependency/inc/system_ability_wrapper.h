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

#ifndef SYSTEM_ABILITY_WRAPPER
#define SYSTEM_ABILITY_WRAPPER

#include "if_system_ability_manager.h"
#include "system_ability_status_change_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

bool RegisterCommonEventListener(void);
bool DeregisterCommonEventListener(void);

class SystemAbilityHandler : public OHOS::SystemAbilityStatusChangeStub {
public:
    SystemAbilityHandler();
    ~SystemAbilityHandler() = default;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
};

class SystemAbilityManager {
public:
    static const int32_t LIBCESFWK_SERVICES_ID = 3299;
    static bool RegisterCommonEventListener(void);
    static bool DeregisterCommonEventListener(void);
private:
    static OHOS::sptr<OHOS::ISystemAbilityManager> GetSystemAbility(void);
};

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_ABILITY_WRAPPER_H
