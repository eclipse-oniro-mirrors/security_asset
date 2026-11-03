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
#include "protocol_mapper.h"
#include "psi_pir_log.h"
#include "psi_pir_protocol.h"
#include "securec.h"
#include <memory>
#include <limits>
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
    if (uid > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
        LOGE("Privacy caller UID exceeds OSAccount integer range");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    int32_t userId = -1;
    if (AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(static_cast<int32_t>(uid), userId) != 0 || userId < 0) {
        LOGE("Failed to resolve privacy caller OS account");
        return PSI_PIR_INTERNAL_ERROR;
    }
    identity = {uid, static_cast<uint32_t>(userId)};
    LOGI("SA caller identity resolved, uid=%{private}u, userId=%{private}u", identity.uid, identity.userId);
    return PSI_PIR_SUCCESS;
}
bool CopyBlob(const PrivacyBlob &input, Bytes &output, size_t &total)
{
    if (total > MAX_PRIVACY_INLINE_BYTES - PRIVACY_LENGTH_BYTES ||
        input.size > MAX_PRIVACY_INLINE_BYTES - total - PRIVACY_LENGTH_BYTES ||
        (input.size != 0 && input.data == nullptr)) {
        return false;
    }
    total += PRIVACY_LENGTH_BYTES + input.size;
    if (input.size != 0) {
        output.assign(input.data, input.data + input.size);
    }
    return true;
}

int32_t ParseProtocol(const ::PrivacyProtocol &input, psi_pir::PrivacyProtocol &protocol)
{
    if (input.dataSetSize > static_cast<uint32_t>(psi_pir::DataSetSize::SIZE_512) ||
        input.protocolType > static_cast<uint32_t>(psi_pir::ProtocolType::PIR_PROTOCOL)) {
        LOGE("SA protocol enum value is out of range");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    protocol = {static_cast<psi_pir::DataSetSize>(input.dataSetSize),
                static_cast<psi_pir::ProtocolType>(input.protocolType)};
    psi_pir::internal::ProtocolParams params;
    return psi_pir::internal::MapPrivacyProtocol(protocol, params);
}
} // namespace
} // namespace OHOS::Security::PrivacyComputation

extern "C" int32_t AssetGenPrivacyTarget(uint32_t uid, const PrivacyTargetRequest *input, PrivacyBuffer *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    LOGI("SA genPrivacyTarget begin, uid=%{private}u", uid);
    if (input == nullptr || output == nullptr || output->data == nullptr || output->size > MAX_PRIVACY_INLINE_BYTES) {
        LOGE("SA genPrivacyTarget invalid request or output buffer");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::TargetInput target;
    if (input->hashAlg > static_cast<uint32_t>(psi_pir::HashAlg::SHA512)) {
        LOGE("SA hash algorithm enum value is out of range");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    target.hashAlg = static_cast<psi_pir::HashAlg>(input->hashAlg);
    size_t total = TARGET_FIXED_BYTES;
    if (!CopyBlob(input->element, target.elemData, total)) {
        LOGE("SA genPrivacyTarget invalid element buffer or byte budget");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::PrivacyProtocol protocol;
    auto ret = ParseProtocol(input->protocol, protocol);
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
        LOGE("SA genPrivacyTarget output exceeds capacity, bytes=%{public}zu", result.size());
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    if (memcpy_s(output->data, output->size, result.data(), result.size()) != EOK) {
        LOGE("SA failed to copy generated target");
        return psi_pir::PSI_PIR_INTERNAL_ERROR;
    }
    output->size = static_cast<uint32_t>(result.size());
    LOGI("SA genPrivacyTarget success, outputBytes=%{public}u", output->size);
    return psi_pir::PSI_PIR_SUCCESS;
}

extern "C" int32_t AssetDecryptPrivacyCiphertext(uint32_t uid, const PrivacyDecryptRequest *input,
                                                 PrivacyBuffer *output)
{
    using namespace OHOS::Security::PrivacyComputation;
    LOGI("SA decryptCiphertext begin, uid=%{private}u", uid);
    if (input == nullptr || output == nullptr || output->data == nullptr || output->size < MAX_DECRYPT_RESULT_BYTES ||
        output->size > MAX_PRIVACY_INLINE_BYTES) {
        LOGE("SA decryptCiphertext invalid request or output buffer");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::Bytes ciphertext;
    size_t total = DECRYPT_FIXED_BYTES;
    if (input->ciphertext.size == 0 || !CopyBlob(input->ciphertext, ciphertext, total)) {
        LOGE("SA decryptCiphertext invalid ciphertext");
        return psi_pir::PSI_PIR_INVALID_ARGUMENT;
    }
    psi_pir::PrivacyProtocol protocol;
    auto ret = ParseProtocol(input->protocol, protocol);
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
    psi_pir::Bytes plaintext;
    ret = psi_pir::DecryptCiphertext(ciphertext, protocol, identity, plaintext);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    if (plaintext.size() != MAX_DECRYPT_RESULT_BYTES) {
        LOGE("SA decryptCiphertext unexpected plaintext size=%{public}zu", plaintext.size());
        return psi_pir::PSI_PIR_CODEC_ERROR;
    }
    if (memcpy_s(output->data, output->size, plaintext.data(), plaintext.size()) != EOK) {
        LOGE("SA failed to copy HE plaintext");
        return psi_pir::PSI_PIR_INTERNAL_ERROR;
    }
    output->size = static_cast<uint32_t>(plaintext.size());
    LOGI("SA decryptCiphertext success, bytes=%{public}u", output->size);
    return psi_pir::PSI_PIR_SUCCESS;
}
