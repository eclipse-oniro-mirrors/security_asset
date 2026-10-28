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

#ifndef PSI_PIR_HIFHE_ADAPTER_H
#define PSI_PIR_HIFHE_ADAPTER_H

#include "he_types.h"
#include "hifhe.h"

namespace psi_pir::internal {
// Native HiFHE API. Ciphertexts are transferred only through HIFHE_Serialize/Deserialize.
class HifheAdapter final {
public:
    HifheAdapter() noexcept = default;
    ~HifheAdapter();
    HifheAdapter(const HifheAdapter &) = delete;
    HifheAdapter &operator=(const HifheAdapter &) = delete;
    HifheAdapter(HifheAdapter &&) = delete;
    HifheAdapter &operator=(HifheAdapter &&) = delete;
    int32_t InitializeEvaluation(const ProtocolParams &params) noexcept;
    int32_t InitializeKeyed(const ProtocolParams &params, const HeSeed &seed) noexcept;
    int32_t Encode(const Slots &slots, Bytes &output) noexcept;
    int32_t Decode(const Bytes &plaintext, Slots &output) noexcept;
    int32_t Encrypt(const Bytes &plaintext, Bytes &output) noexcept;
    int32_t Decrypt(const Bytes &ciphertext, Bytes &output) noexcept;
    // SET_NEGATE is a transient flag: consume it in Add before serializing the result.
    int32_t SubtractFromPlaintext(const Bytes &ciphertext, const Bytes &plaintext, Bytes &output) noexcept;
    int32_t AddPlaintext(const Bytes &ciphertext, const Bytes &plaintext, Bytes &output) noexcept;
    int32_t MultiplyPlaintext(const Bytes &ciphertext, const Bytes &plaintext, Bytes &output) noexcept;
    int32_t Rerandomize(const Bytes &ciphertext, Bytes &output) noexcept;

private:
    int32_t ConfigureContext() noexcept;
    void Reset() noexcept;
    HIFHE_Ctx *ctx_ = nullptr;
};

} // namespace psi_pir::internal

#endif // PSI_PIR_HIFHE_ADAPTER_H
