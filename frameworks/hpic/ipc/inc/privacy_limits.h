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

#ifndef HPIC_PRIVACY_LIMITS_H
#define HPIC_PRIVACY_LIMITS_H

#include <cstddef>
#include <cstdint>

namespace OHOS::Security::PrivacyComputation {
// Per-operation resource budget; large payloads travel through read-only shared memory.
constexpr uint32_t MAX_PRIVACY_BYTES = 64 * 1024 * 1024;
constexpr size_t PRIVACY_LENGTH_BYTES = sizeof(uint32_t);
constexpr size_t PRIVACY_FLAG_BYTES = sizeof(uint32_t);
constexpr size_t PRIVACY_PROTOCOL_BYTES = sizeof(uint32_t) + sizeof(uint32_t);
constexpr size_t TARGET_FIXED_BYTES = PRIVACY_PROTOCOL_BYTES + sizeof(uint32_t); // Protocol and hash algorithm.
constexpr size_t SEARCH_FIXED_BYTES = PRIVACY_PROTOCOL_BYTES + PRIVACY_FLAG_BYTES;
constexpr size_t RESULT_FIXED_BYTES = PRIVACY_FLAG_BYTES + PRIVACY_FLAG_BYTES;
constexpr size_t MAX_PRIVACY_ITEMS = 65536;
constexpr uint32_t MAX_TARGET_RESULT_BYTES = 256 * 1024;
} // namespace OHOS::Security::PrivacyComputation

#endif // HPIC_PRIVACY_LIMITS_H
