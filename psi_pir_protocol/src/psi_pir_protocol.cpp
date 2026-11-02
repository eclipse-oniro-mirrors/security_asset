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

#include "psi_pir_protocol.h"

#include "protocol_mapper.h"
#include "psi_pir_log.h"
#include "psi_pir_protocol_impl.h"

namespace psi_pir {

int32_t GenPrivacyTarget(const TargetInput &input, const PrivacyProtocol &protocol, const CallerIdentity &identity,
                         Bytes &output) noexcept
{
    PSI_PIR_LOGI("protocol genPrivacyTarget begin, inputBytes=%{public}zu", input.elemData.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto targetRet = internal::PsiPirProtocolImpl().GenPrivacyTarget(input, params, identity, output);
    if (targetRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("genPrivacyTarget failed in protocol core, ret=%{public}d", targetRet);
    } else {
        PSI_PIR_LOGI("protocol genPrivacyTarget success, outputBytes=%{public}zu", output.size());
    }
    return targetRet;
}

int32_t PrivacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements, const PrivacyProtocol &protocol,
                      PrivacySearchResult &output) noexcept
{
    PSI_PIR_LOGI("protocol privacySearch begin, targetBytes=%{public}zu, elements=%{public}zu",
                 privacyTarget.size(), elements.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto searchRet = internal::PsiPirProtocolImpl().PrivacySearch(privacyTarget, elements, params, output);
    if (searchRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("privacySearch failed in protocol core, ret=%{public}d", searchRet);
    } else {
        PSI_PIR_LOGI("protocol privacySearch success, batches=%{public}zu", output.fheCipherText.size());
    }
    return searchRet;
}

int32_t DecryptCiphertext(const Bytes &ciphertext, const PrivacyProtocol &protocol,
    const CallerIdentity &identity, Bytes &output) noexcept
{
    PSI_PIR_LOGI("protocol decryptCiphertext begin, bytes=%{public}zu", ciphertext.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto decryptRet = internal::PsiPirProtocolImpl().DecryptCiphertext(ciphertext, params, identity, output);
    if (decryptRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("decryptCiphertext failed, ret=%{public}d", decryptRet);
    } else {
        PSI_PIR_LOGI("protocol decryptCiphertext success, bytes=%{public}zu", output.size());
    }
    return decryptRet;
}

int32_t PostProcessSearchResult(const std::vector<Bytes> &fhePlainText,
    const std::optional<std::vector<Bytes>> &symCipherText, const PrivacyProtocol &protocol,
    SearchResult &output) noexcept
{
    PSI_PIR_LOGI("protocol PostProcessSearchResult postprocess begin, batches=%{public}zu", fhePlainText.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto resultRet = internal::PsiPirProtocolImpl().PostProcessSearchResult(fhePlainText, symCipherText, params, output);
    if (resultRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("PostProcessSearchResult postprocess failed, ret=%{public}d", resultRet);
    } else {
        PSI_PIR_LOGI("protocol PostProcessSearchResult postprocess success");
    }
    return resultRet;
}
} // namespace psi_pir
