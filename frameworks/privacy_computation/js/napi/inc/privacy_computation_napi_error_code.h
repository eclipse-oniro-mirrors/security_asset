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

#ifndef privacy_computation_napi_ERROR_CODE_H
#define privacy_computation_napi_ERROR_CODE_H

#include <cstdint>

namespace OHOS::Security::PrivacyComputation {
constexpr int32_t PRIVACY_COMPUTATION_SUCCESS = 0;
constexpr int32_t PRIVACY_COMPUTATION_INVALID_ARGUMENT = 401;
constexpr int32_t PRIVACY_COMPUTATION_SERVICE_ERROR = 24000001;
constexpr int32_t PRIVACY_COMPUTATION_OUT_OF_MEMORY = 24000006;
constexpr int32_t PRIVACY_COMPUTATION_DATA_CORRUPTED = 24000007;
constexpr int32_t PRIVACY_COMPUTATION_CRYPTO_ERROR = 24000009;
constexpr int32_t PRIVACY_COMPUTATION_ACCOUNT_ERROR = 24000012;
constexpr int32_t PRIVACY_COMPUTATION_UNAVAILABLE = 24000017;
int32_t MapProtocolError(int32_t result) noexcept;
const char *GetErrorMessage(int32_t code) noexcept;

} // namespace OHOS::Security::PrivacyComputation

#endif // privacy_computation_napi_ERROR_CODE_H
