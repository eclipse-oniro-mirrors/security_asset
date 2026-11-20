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

#ifndef ASSET_PRIVACY_BUNDLE_MANAGER_ADAPTER_H
#define ASSET_PRIVACY_BUNDLE_MANAGER_ADAPTER_H

#include <cstdint>
#include <string>

namespace OHOS::Security::PrivacyComputation {
// Resolves the trusted caller UID through BMS; failure preserves the output app ID.
int32_t GetCallerAppId(uint32_t uid, std::string &appId);
} // namespace OHOS::Security::PrivacyComputation

#endif // ASSET_PRIVACY_BUNDLE_MANAGER_ADAPTER_H
