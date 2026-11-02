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

#include "privacy_client.h"
#include "privacy_client_ffi.h"
#include "psi_pir_log.h"
#include <utility>

namespace OHOS::Security::PrivacyComputation {
int32_t GenPrivacyTargetRemote(const psi_pir::TargetInput &input, const psi_pir::PrivacyProtocol &protocol,
                               psi_pir::Bytes &output)
{
    PSI_PIR_LOGI("HPIC IPC genPrivacyTarget begin, inputBytes=%{public}zu", input.elemData.size());
    if (input.elemData.size() > MAX_PRIVACY_INLINE_BYTES - TARGET_FIXED_BYTES - PRIVACY_LENGTH_BYTES) {
        PSI_PIR_LOGE("HPIC IPC genPrivacyTarget input exceeds byte budget");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    const PrivacyTargetRequest request{
        {static_cast<uint32_t>(protocol.dataSetSize), static_cast<uint32_t>(protocol.protocolType)},
        static_cast<uint32_t>(input.hashAlg),
        {input.elemData.data(), static_cast<uint32_t>(input.elemData.size())}};
    psi_pir::Bytes bytes(MAX_TARGET_RESULT_BYTES);
    PrivacyBuffer result{bytes.data(), static_cast<uint32_t>(bytes.size())};
    const auto ret = asset_privacy_gen_target(&request, &result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HPIC IPC genPrivacyTarget failed, ret=%{public}d", ret);
        return ret;
    }
    if (result.size == 0 || result.size > bytes.size()) {
        PSI_PIR_LOGE("HPIC IPC genPrivacyTarget invalid response size=%{public}u", result.size);
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    bytes.resize(result.size);
    output = std::move(bytes);
    PSI_PIR_LOGI("HPIC IPC genPrivacyTarget success, outputBytes=%{public}zu", output.size());
    return psi_pir::PSI_PIR_SUCCESS;
}
int32_t GetSearchResultRemote(const psi_pir::PrivacySearchResult &input, const psi_pir::PrivacyProtocol &protocol,
                              psi_pir::SearchResult &output)
{
    PSI_PIR_LOGI("HPIC IPC getSearchResult begin, batches=%{public}zu", input.fheCipherText.size());
    std::vector<psi_pir::Bytes> plaintexts;
    plaintexts.reserve(input.fheCipherText.size());
    for (size_t batch = 0; batch < input.fheCipherText.size(); ++batch) {
        const auto &ciphertext = input.fheCipherText[batch];
        if (ciphertext.empty() ||
            ciphertext.size() > MAX_PRIVACY_INLINE_BYTES - DECRYPT_FIXED_BYTES - PRIVACY_LENGTH_BYTES) {
            PSI_PIR_LOGE("HPIC IPC invalid HE batch, batch=%{public}zu, bytes=%{public}zu", batch, ciphertext.size());
            return psi_pir::PSI_PIR_INVALID_ARGUMENT;
        }
        const PrivacyDecryptRequest request{
            {static_cast<uint32_t>(protocol.dataSetSize), static_cast<uint32_t>(protocol.protocolType)},
            {ciphertext.data(), static_cast<uint32_t>(ciphertext.size())}};
        psi_pir::Bytes plaintext(MAX_DECRYPT_RESULT_BYTES);
        PrivacyBuffer result{plaintext.data(), static_cast<uint32_t>(plaintext.size())};
        PSI_PIR_LOGI("HPIC IPC decryptCiphertext begin, batch=%{public}zu, bytes=%{public}zu", batch,
            ciphertext.size());
        const auto ret = asset_privacy_decrypt_ciphertext(&request, &result);
        if (ret != psi_pir::PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("HPIC IPC decryptCiphertext failed, batch=%{public}zu, ret=%{public}d", batch, ret);
            return ret;
        }
        if (result.size != MAX_DECRYPT_RESULT_BYTES) {
            PSI_PIR_LOGE("HPIC IPC invalid HE plaintext length=%{public}u", result.size);
            return psi_pir::PSI_PIR_CODEC_ERROR;
        }
        plaintexts.push_back(std::move(plaintext));
        PSI_PIR_LOGI("HPIC IPC decryptCiphertext success, batch=%{public}zu", batch);
    }
    // Preserve all batch positions; symmetric frames are borrowed locally and never enter IPC.
    return psi_pir::GetSearchResult(plaintexts, input.symCipherText, protocol, output);
}
} // namespace OHOS::Security::PrivacyComputation
