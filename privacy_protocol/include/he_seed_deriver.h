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

#ifndef PRIVACY_PROTOCOL_HE_SEED_DERIVER_H
#define PRIVACY_PROTOCOL_HE_SEED_DERIVER_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace OHOS::Security::PrivacyComputation {
constexpr std::size_t kHeSeedSize = 32;

// Operation-local secret storage, cleared when the protocol releases it.
class HeSeed final {
public:
    HeSeed() noexcept = default;
    ~HeSeed();
    HeSeed(const HeSeed &) = delete;
    HeSeed &operator=(const HeSeed &) = delete;
    std::array<uint8_t, kHeSeedSize> bytes{};
};

// A caller-owned derivation context that remains alive until the protocol call returns.
class IHeSeedDeriver {
public:
    IHeSeedDeriver() noexcept = default;
    virtual ~IHeSeedDeriver() = default;
    IHeSeedDeriver(const IHeSeedDeriver &) = delete;
    IHeSeedDeriver &operator=(const IHeSeedDeriver &) = delete;
    // Write exactly one HE seed on success; return a protocol error on failure.
    virtual int32_t Derive(HeSeed &seed) const noexcept = 0;
};
} // namespace OHOS::Security::PrivacyComputation
#endif // PRIVACY_PROTOCOL_HE_SEED_DERIVER_H
