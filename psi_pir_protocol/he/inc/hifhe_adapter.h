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
// An operation-local native object. Never transfer the handle through IPC or public protocol APIs.
struct HifheText final {
    HifheText() noexcept = default;
    ~HifheText();
    HifheText(const HifheText &) = delete;
    HifheText &operator=(const HifheText &) = delete;
    HifheText(HifheText &&) = delete;
    HifheText &operator=(HifheText &&) = delete;
    HIFHE_HEText *value = nullptr;
};

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
    // Serialization is restricted to the ciphertext boundaries of a complete operation.
    int32_t Serialize(const HifheText &ciphertext, Bytes &output) const noexcept;
    int32_t Deserialize(const Bytes &ciphertext, HifheText &output) noexcept;
    int32_t Encode(const Slots &slots, HifheText &output) noexcept;
    int32_t Decode(const HifheText &plaintext, Slots &output) noexcept;
    int32_t Encrypt(const HifheText &plaintext, HifheText &output) noexcept;
    int32_t Decrypt(const HifheText &ciphertext, HifheText &output) noexcept;
    int32_t SubtractFromPlaintext(HifheText &ciphertext, const HifheText &plaintext, HifheText &output) noexcept;
    int32_t MultiplyPlaintext(const HifheText &ciphertext, const HifheText &plaintext, HifheText &output) noexcept;
    int32_t Rerandomize(const HifheText &ciphertext, HifheText &output) noexcept;

private:
    int32_t ConfigureContext() noexcept;
    void Reset() noexcept;
    HIFHE_Ctx *ctx_ = nullptr;
};
} // namespace psi_pir::internal
#endif // PSI_PIR_HIFHE_ADAPTER_H
