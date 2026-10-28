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
#include "privacy_result_codec.h"
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
bool MakeBlobs(const std::vector<psi_pir::Bytes> &input, std::vector<PrivacyBlob> &blobs, size_t &total)
{
    if (input.size() > MAX_PRIVACY_ITEMS || total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES) {
        return false;
    }
    total += PRIVACY_LENGTH_BYTES;
    for (const auto &value : input) {
        if (total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
            value.size() > MAX_PRIVACY_BYTES - total - PRIVACY_LENGTH_BYTES) {
            return false;
        }
        total += PRIVACY_LENGTH_BYTES + value.size();
        blobs.push_back({value.data(), static_cast<uint32_t>(value.size())});
    }
    return true;
}
} // namespace
int32_t GenPrivacyTargetRemote(const psi_pir::TargetInput &input, const psi_pir::PrivacyProtocol &protocol,
                               psi_pir::Bytes &output)
{
    if (input.elemData.size() > MAX_PRIVACY_BYTES - TARGET_FIXED_BYTES - PRIVACY_LENGTH_BYTES) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    const PrivacyTargetRequest request{
        {static_cast<uint32_t>(protocol.dataSetSize), static_cast<uint32_t>(protocol.protocolType)},
        static_cast<uint32_t>(input.hashAlg),
        {input.elemData.data(), static_cast<uint32_t>(input.elemData.size())}};
    psi_pir::Bytes bytes(MAX_PRIVACY_BYTES);
    PrivacyBuffer result{bytes.data(), static_cast<uint32_t>(bytes.size())};
    const auto ret = asset_privacy_gen_target(&request, &result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (result.size == 0 || result.size > bytes.size()) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    bytes.resize(result.size);
    output = std::move(bytes);
    return psi_pir::PSI_PIR_SUCCESS;
}
int32_t GetSearchResultRemote(const psi_pir::PrivacySearchResult &input, const psi_pir::PrivacyProtocol &protocol,
                              psi_pir::SearchResult &output)
{
    std::vector<PrivacyBlob> ciphertexts;
    std::vector<PrivacyBlob> symmetric;
    size_t total = SEARCH_FIXED_BYTES;
    if (!MakeBlobs(input.fheCipherText, ciphertexts, total) ||
        (input.symCipherText && !MakeBlobs(*input.symCipherText, symmetric, total))) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    const PrivacySearchRequest request{
        {static_cast<uint32_t>(protocol.dataSetSize), static_cast<uint32_t>(protocol.protocolType)},
        {ciphertexts.data(), static_cast<uint32_t>(ciphertexts.size())},
        input.symCipherText ? 1U : 0U,
        {symmetric.data(), static_cast<uint32_t>(symmetric.size())}};
    psi_pir::Bytes bytes(MAX_PRIVACY_BYTES);
    PrivacyBuffer result{bytes.data(), static_cast<uint32_t>(bytes.size())};
    const auto ret = asset_privacy_get_result(&request, &result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (result.size > bytes.size() || !DecodePrivacyResult(bytes.data(), result.size, output)) {
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    return psi_pir::PSI_PIR_SUCCESS;
}
} // namespace OHOS::Security::PrivacyComputation
