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

#ifndef PSI_PIR_SOFT_HE_ENGINE_H
#define PSI_PIR_SOFT_HE_ENGINE_H

#include "crypto_provider.h"
#include "he_types.h"
#include <memory>

namespace psi_pir::internal {
class SoftHeEngine final : public psi_pir::internal::IHEEngine {
public:
    // Raw HE primitives: no caller identity lookup, target construction or search-result processing.
    int32_t Encrypt(const Slots &slots, const ProtocolParams &params, const HeSeed &seed, Bytes &output) const noexcept;
    int32_t Decrypt(const Bytes &ciphertext, const ProtocolParams &params, const HeSeed &seed,
                    Slots &output) const noexcept;
    bool IsCompatible(const ProtocolParams &params) const noexcept override;
    int32_t addPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept override;
    int32_t multiplyPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept override;
    int32_t RandCipherText(const Bytes &ciphertext, Bytes &output) const noexcept override;
};
std::shared_ptr<const psi_pir::internal::IHEEngine> GetSoftHeEngine() noexcept;
// Installs the protocol default without replacing an explicitly registered provider.
int32_t EnsureDefaultHeEngineInstalled() noexcept;

} // namespace psi_pir::internal

#endif // PSI_PIR_SOFT_HE_ENGINE_H
