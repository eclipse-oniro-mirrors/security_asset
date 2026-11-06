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

#ifndef PRIVACY_PROTOCOL_HE_KEY_DERIVER_H
#define PRIVACY_PROTOCOL_HE_KEY_DERIVER_H

#include "he_types.h"
#include <memory>

namespace PrivacyComputation::internal {
// Service-side extension. No root material is exposed through the protocol API or IPC.
class IHeSeedDeriver {
public:
    virtual ~IHeSeedDeriver() = default;
    virtual int32_t Derive(const CallerIdentity &identity, const std::array<uint8_t, kIdentityHashSize> &materialHash,
                           HeSeed &seed) const noexcept = 0;
};
void SetHeSeedDeriver(std::shared_ptr<const IHeSeedDeriver> deriver) noexcept;

class HeKeyDeriver final {
public:
    int32_t DeriveKey(const CallerIdentity &identity, HeSeed &seed) const noexcept;
};

} // namespace PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_HE_KEY_DERIVER_H
