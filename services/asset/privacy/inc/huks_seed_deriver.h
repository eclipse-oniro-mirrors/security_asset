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

#ifndef ASSET_HUKS_SEED_DERIVER_H
#define ASSET_HUKS_SEED_DERIVER_H

#include "he_key_deriver.h"

namespace OHOS::Security::PrivacyComputation {
enum class RootKeyPolicy {
    REQUIRE_EXISTING,
    CREATE_IF_MISSING,
};

// Callers in one OS account share the fixed alias in its HUKS CE namespace; root bytes never leave HUKS.
class HuksSeedDeriver final : public ::PrivacyComputation::internal::IHeSeedDeriver {
public:
    // Only target generation may create a missing root; result decryption must fail if absent.
    int32_t PrepareRootKey(uint32_t userId, RootKeyPolicy policy) const;
    // userId selects the CE namespace, while the complete caller identity is included in materialHash.
    int32_t Derive(const ::PrivacyComputation::CallerIdentity &identity,
                   const std::array<uint8_t, ::PrivacyComputation::internal::kIdentityHashSize> &materialHash,
                   ::PrivacyComputation::internal::HeSeed &seed) const noexcept override;
};
} // namespace OHOS::Security::PrivacyComputation
#endif // ASSET_HUKS_SEED_DERIVER_H
