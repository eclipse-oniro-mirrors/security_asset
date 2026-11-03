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

#ifndef PRIVACY_COVERAGE_OS_ACCOUNT_MANAGER_H
#define PRIVACY_COVERAGE_OS_ACCOUNT_MANAGER_H
#include "host_faults.h"
namespace OHOS::AccountSA {
class OsAccountManager {
public:
    static int32_t GetOsAccountLocalIdFromUid(int32_t, int32_t &userId)
    {
        userId = PrivacyHost::State().userId;
        return PrivacyHost::State().accountStatus;
    }
};
} // namespace OHOS::AccountSA
#endif
