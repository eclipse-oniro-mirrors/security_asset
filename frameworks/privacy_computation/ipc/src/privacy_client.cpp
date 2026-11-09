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
#include "privacy_log.h"
#include <limits>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
int32_t GenPrivacyTargetRemote(const TargetInput &input, const PrivacyProtocol &protocol, Bytes &output)
{
    LOGI("IPC genPrivacyTarget begin, inputBytes=%{public}zu", input.elemData.size());
    if (input.elemData.size() > std::numeric_limits<uint32_t>::max()) {
        LOGE("IPC genPrivacyTarget input length does not fit the request");
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    const PrivacyTargetRequest request{
        {static_cast<uint32_t>(protocol.dataSetSize), static_cast<uint32_t>(protocol.protocolType)},
        static_cast<uint32_t>(input.hashAlg),
        {input.elemData.data(), static_cast<uint32_t>(input.elemData.size())}};
    Bytes bytes(MAX_TARGET_RESULT_BYTES);
    PrivacyBuffer result{bytes.data(), static_cast<uint32_t>(bytes.size())};
    const auto ret = asset_privacy_gen_target(&request, &result);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("IPC genPrivacyTarget failed, ret=%{public}d", ret);
        return ret;
    }
    if (result.size == 0 || result.size > bytes.size()) {
        LOGE("IPC genPrivacyTarget invalid response size=%{public}u", result.size);
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    bytes.resize(result.size);
    output = std::move(bytes);
    LOGI("IPC genPrivacyTarget success, outputBytes=%{public}zu", output.size());
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t GetSearchResultRemote(const PrivacySearchResult &input, const PrivacyProtocol &protocol,
                              SearchResult &output)
{
    LOGI("IPC getSearchResult begin, batches=%{public}zu", input.resultCipherText.size());
    std::vector<Bytes> plaintexts;
    plaintexts.reserve(input.resultCipherText.size());
    for (size_t batch = 0; batch < input.resultCipherText.size(); ++batch) {
        const auto &ciphertext = input.resultCipherText[batch];
        if (ciphertext.empty() ||
            ciphertext.size() > std::numeric_limits<uint32_t>::max()) {
            LOGE("IPC invalid HE batch, batch=%{public}zu, bytes=%{public}zu", batch, ciphertext.size());
            return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
        }
        const PrivacyDecryptRequest request{
            {static_cast<uint32_t>(protocol.dataSetSize), static_cast<uint32_t>(protocol.protocolType)},
            {ciphertext.data(), static_cast<uint32_t>(ciphertext.size())}};
        Bytes plaintext(MAX_DECRYPT_RESULT_BYTES);
        PrivacyBuffer result{plaintext.data(), static_cast<uint32_t>(plaintext.size())};
        LOGI("IPC decryptCiphertext begin, batch=%{public}zu, bytes=%{public}zu", batch, ciphertext.size());
        const auto ret = asset_privacy_decrypt_ciphertext(&request, &result);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("IPC decryptCiphertext failed, batch=%{public}zu, ret=%{public}d", batch, ret);
            return ret;
        }
        if (result.size != MAX_DECRYPT_RESULT_BYTES) {
            LOGE("IPC invalid HE plaintext length=%{public}u", result.size);
            return PRIVACY_PROTOCOL_CODEC_ERROR;
        }
        plaintexts.push_back(std::move(plaintext));
        LOGI("IPC decryptCiphertext success, batch=%{public}zu", batch);
    }
    // Preserve all batch positions; symmetric frames are borrowed locally and never enter IPC.
    return PostProcessSearchResult(plaintexts, input.valueCipherText, protocol, output);
}
} // namespace OHOS::Security::PrivacyComputation
