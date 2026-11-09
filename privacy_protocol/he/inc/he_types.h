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

#ifndef PRIVACY_PROTOCOL_HE_TYPES_H
#define PRIVACY_PROTOCOL_HE_TYPES_H

#include "internal_types.h"
#include "protocol_params.h"
#include "privacy_protocol_types.h"
#include <array>

namespace OHOS::Security::PrivacyComputation::internal {
constexpr std::size_t kHeSeedSize = 32;
constexpr std::size_t kIdentityHashSize = 32;
// Secret storage is cleared at scope exit; no copies may escape an operation.
class HeSeed final {
public:
    HeSeed() noexcept = default;
    ~HeSeed();
    HeSeed(const HeSeed &) = delete;
    HeSeed &operator=(const HeSeed &) = delete;
    std::array<uint8_t, kHeSeedSize> bytes{};
};

} // namespace OHOS::Security::PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_HE_TYPES_H
