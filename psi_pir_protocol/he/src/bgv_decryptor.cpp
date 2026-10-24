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

#include "bgv_decryptor.h"
#include "he_key_deriver.h"
#include "hifhe_adapter.h"
#include "psi_pir_error_code.h"
#include <utility>

namespace psi_pir::internal {
int32_t BgvDecryptor::DecryptToSlots(const Bytes &ciphertext, const ProtocolParams &params,
                                     const OperationIdentity &identity, Bytes &output) const noexcept
{
    if (ciphertext.empty()) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    HeSeed seed;
    auto ret = HeKeyDeriver().DeriveKey(identity, seed);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    HifheAdapter adapter;
    ret = adapter.InitializeKeyed(params, seed);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    Bytes plaintext;
    ret = adapter.Decrypt(ciphertext, plaintext);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    Slots slots;
    ret = adapter.Decode(plaintext, slots);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (slots.size() != 2048) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    Bytes result;
    result.reserve(16384);
    for (const auto slot : slots) {
        if (slot >= params.plainModulus) {
            return psi_pir::PSI_PIR_CODEC_ERROR;
        }
        for (size_t byte = 0; byte < 8; ++byte) {
            result.push_back(static_cast<uint8_t>(slot >> (byte * 8)));
        }
    }
    output = std::move(result);
    return psi_pir::PSI_PIR_SUCCESS;
}
} // namespace psi_pir::internal
