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

#ifndef PRIVACY_COMPUTATION_PRIVACY_CLIENT_H
#define PRIVACY_COMPUTATION_PRIVACY_CLIENT_H
#include "privacy_limits.h"
#include "privacy_protocol.h"
namespace OHOS::Security::PrivacyComputation {
int32_t GenPrivacyTargetRemote(const TargetInput &input, const PrivacyProtocol &protocol, Bytes &output);
// Decrypts HE batches through SA, then postprocesses locally without sending symmetric frames.
int32_t GetSearchResultRemote(const PrivacySearchResult &input, const PrivacyProtocol &protocol,
                              SearchResult &output);
} // namespace OHOS::Security::PrivacyComputation
#endif // PRIVACY_COMPUTATION_PRIVACY_CLIENT_H
