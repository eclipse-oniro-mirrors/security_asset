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

#include "he_key_deriver.h"
#include "huks_seed_deriver.h"
#include "os_account_manager.h"
#include "privacy_limits.h"
#include "privacy_request.h"
#include "privacy_result_codec.h"
#include "protocol_mapper.h"
#include "psi_pir_log.h"
#include "psi_pir_protocol.h"
#include <algorithm>
#include <memory>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
using namespace psi_pir;
using namespace psi_pir::internal;
int32_t PrepareRootKey(uint32_t userId, RootKeyPolicy policy)
{
    // Function-local static initialization serializes provider registration once per SA process.
    static const auto deriver = [] {
        auto provider = std::make_shared<HuksSeedDeriver>();
        SetHeSeedDeriver(provider);
        return provider;
    }();
    return deriver->PrepareRootKey(userId, policy);
}
int32_t PrepareIdentity(uint32_t uid, CallerIdentity &identity)
{
    int32_t userId = -1;
    if (AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userId) != 0 || userId < 0) {
        PSI_PIR_LOGE("Failed to resolve privacy caller OS account");
        return PSI_PIR_INTERNAL_ERROR;
    }
    identity = {uid, static_cast<uint32_t>(userId)};
    PSI_PIR_LOGI("HPIC SA caller identity resolved, uid=%{private}u, userId=%{private}u", identity.uid,
                 identity.userId);
    return PSI_PIR_SUCCESS;
}
bool CopyBlob(const PrivacyBlob &input, Bytes &output, size_t &total)
{
    if (total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
        input.size > MAX_PRIVACY_BYTES - total - PRIVACY_LENGTH_BYTES || (input.size != 0 && input.data == nullptr)) {
        return false;
    }
    total += PRIVACY_LENGTH_BYTES + input.size;
    if (input.size != 0) {
        output.assign(input.data, input.data + input.size);
    }
    return true;
}
bool CopyBlobs(const PrivacyBlobList &input, std::vector<Bytes> &output, size_t &total)
{
    if (input.count > MAX_PRIVACY_ITEMS || total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
        (input.count != 0 && input.data == nullptr)) {
        return false;
    }
    total += PRIVACY_LENGTH_BYTES;
    for (uint32_t i = 0; i < input.count; ++i) {
        Bytes value;
        if (!CopyBlob(input.data[i], value, total)) {
            return false;
        }
        output.push_back(std::move(value));
    }
    return true;
}
bool CopySearchRequest(const PrivacySearchRequest &input, psi_pir::PrivacySearchResult &request)
{
    size_t total = SEARCH_FIXED_BYTES;
    if (!CopyBlobs(input.ciphertexts, request.fheCipherText, total)) {
        PSI_PIR_LOGE("HPIC SA getSearchResult invalid ciphertext buffers or byte budget");
        return false;
    }
    if (input.hasSymmetric == 1) {
        request.symCipherText.emplace();
        if (!CopyBlobs(input.symmetric, *request.symCipherText, total)) {
            PSI_PIR_LOGE("HPIC SA getSearchResult invalid symmetric buffers or byte budget");
            return false;
        }
    }
    return true;
}
} // namespace
} // namespace OHOS::Security::PrivacyComputation

extern "C" int32_t AssetGenPrivacyTarget(uint32_t uid, const PrivacyTargetRequest *input, PrivacyBuffer *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    PSI_PIR_LOGI("HPIC SA genPrivacyTarget begin, uid=%{private}u", uid);
    if (input == nullptr || output == nullptr || output->data == nullptr || output->size > MAX_PRIVACY_BYTES) {
        PSI_PIR_LOGE("HPIC SA genPrivacyTarget invalid request or output buffer");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::TargetInput target;
    target.hashAlg = static_cast<psi_pir::HashAlg>(input->hashAlg);
    size_t total = TARGET_FIXED_BYTES;
    if (!CopyBlob(input->element, target.elemData, total)) {
        PSI_PIR_LOGE("HPIC SA genPrivacyTarget invalid element buffer or byte budget");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    const psi_pir::PrivacyProtocol protocol{static_cast<psi_pir::DataSetSize>(input->protocol.dataSetSize),
                                            static_cast<psi_pir::ProtocolType>(input->protocol.protocolType)};
    psi_pir::internal::ProtocolParams params;
    auto ret = psi_pir::internal::MapPrivacyProtocol(protocol, params);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::CallerIdentity identity;
    ret = PrepareIdentity(uid, identity);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = PrepareRootKey(identity.userId, RootKeyPolicy::CREATE_IF_MISSING);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::Bytes result;
    ret = psi_pir::GenPrivacyTarget(target, protocol, identity, result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (result.size() > output->size) {
        PSI_PIR_LOGE("HPIC SA genPrivacyTarget output exceeds capacity, bytes=%{public}zu", result.size());
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    std::copy(result.begin(), result.end(), output->data);
    output->size = static_cast<uint32_t>(result.size());
    PSI_PIR_LOGI("HPIC SA genPrivacyTarget success, outputBytes=%{public}u", output->size);
    return psi_pir::PSI_PIR_SUCCESS;
}

extern "C" int32_t AssetGetPrivacyResult(uint32_t uid, const PrivacySearchRequest *input, const PrivacyOutput *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    PSI_PIR_LOGI("HPIC SA getSearchResult begin, uid=%{private}u", uid);
    if (input == nullptr || output == nullptr || output->context == nullptr || output->write == nullptr ||
        input->hasSymmetric > 1) {
        PSI_PIR_LOGE("HPIC SA getSearchResult invalid request or output buffer");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::PrivacySearchResult request;
    if (!CopySearchRequest(*input, request)) {
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    const psi_pir::PrivacyProtocol protocol{static_cast<psi_pir::DataSetSize>(input->protocol.dataSetSize),
                                            static_cast<psi_pir::ProtocolType>(input->protocol.protocolType)};
    psi_pir::internal::ProtocolParams params;
    auto ret = psi_pir::internal::MapPrivacyProtocol(protocol, params);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::CallerIdentity identity;
    ret = PrepareIdentity(uid, identity);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = PrepareRootKey(identity.userId, RootKeyPolicy::REQUIRE_EXISTING);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::SearchResult result;
    ret = psi_pir::GetSearchResult(request, protocol, identity, result);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    psi_pir::Bytes bytes;
    if (!EncodePrivacyResult(result, bytes)) {
        PSI_PIR_LOGE("Privacy result exceeds output capacity or byte budget");
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    ret = output->write(output->context, bytes.data(), static_cast<uint32_t>(bytes.size()));
    if (ret == psi_pir::PSI_PIR_SUCCESS) {
        PSI_PIR_LOGI("HPIC SA getSearchResult success, outputBytes=%{public}zu", bytes.size());
    }
    return ret;
}
