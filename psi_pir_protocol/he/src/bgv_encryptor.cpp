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

#include "bgv_encryptor.h"
#include "he_key_deriver.h"
#include "hifhe_adapter.h"
#include "protocol_encoder.h"
#include "psi_pir_error_code.h"
#include "psi_pir_log.h"
#include <utility>

namespace psi_pir::internal {
int32_t BgvEncryptor::GenerateTarget(const TargetInput &input, const ProtocolParams &params,
                                     const CallerIdentity &identity, Bytes &output) const noexcept
{
    psi_pir::internal::ProtocolEncoder encoder;
    if (input.elemData.empty() || encoder.ValidateHashAlgorithm(input.hashAlg) != psi_pir::PSI_PIR_SUCCESS) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    Slots target;
    const psi_pir::Element element{input.elemData, input.hashAlg, std::nullopt};
    const auto ret = encoder.EncodeTargetVector(element, params, target);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    HeSeed seed;
    const auto deriveRet = HeKeyDeriver().DeriveKey(identity, seed);
    if (deriveRet != psi_pir::PSI_PIR_SUCCESS) {
        return deriveRet;
    }
    HifheAdapter adapter;
    const auto initRet = adapter.InitializeKeyed(params, seed);
    if (initRet != psi_pir::PSI_PIR_SUCCESS) {
        return initRet;
    }
    Bytes plaintext;
    const auto encodeRet = adapter.Encode(target, plaintext);
    if (encodeRet != psi_pir::PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("target HE encoding failed, ret=%{public}d", encodeRet);
        return encodeRet;
    }
    Bytes ciphertext;
    const auto encryptRet = adapter.Encrypt(plaintext, ciphertext);
    if (encryptRet != psi_pir::PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("target HE encryption failed, ret=%{public}d", encryptRet);
        return encryptRet;
    }
    if (ciphertext.empty()) {
        PSI_PIR_LOGE("target HE encryption returned empty ciphertext");
        return psi_pir::PSI_PIR_HE_ERROR;
    }
    output = std::move(ciphertext);
    return psi_pir::PSI_PIR_SUCCESS;
}
} // namespace psi_pir::internal
