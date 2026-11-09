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

#include "privacy_protocol.h"

#include "protocol_mapper.h"
#include "privacy_log.h"
#include "privacy_protocol_impl.h"

namespace OHOS::Security::PrivacyComputation {

int32_t GenPrivacyTarget(const TargetInput &input, const PrivacyProtocol &protocol, const CallerIdentity &identity,
                         Bytes &output) noexcept
{
    LOGI("protocol genPrivacyTarget begin, inputBytes=%{public}zu", input.elemData.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    const auto targetRet = internal::PrivacyProtocolImpl().GenPrivacyTarget(input, params, identity, output);
    if (targetRet != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("genPrivacyTarget failed in protocol core, ret=%{public}d", targetRet);
    } else {
        LOGI("protocol genPrivacyTarget success, outputBytes=%{public}zu", output.size());
    }
    return targetRet;
}

int32_t PrivacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements, const PrivacyProtocol &protocol,
                      PrivacySearchResult &output) noexcept
{
    LOGI("protocol privacySearch begin, targetBytes=%{public}zu, elements=%{public}zu", privacyTarget.size(),
         elements.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    const auto searchRet = internal::PrivacyProtocolImpl().PrivacySearch(privacyTarget, elements, params, output);
    if (searchRet != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("privacySearch failed in protocol core, ret=%{public}d", searchRet);
    } else {
        LOGI("protocol privacySearch success, batches=%{public}zu", output.resultCipherText.size());
    }
    return searchRet;
}

int32_t DecryptCiphertext(const Bytes &ciphertext, const PrivacyProtocol &protocol, const CallerIdentity &identity,
                          Bytes &output) noexcept
{
    LOGI("protocol decryptCiphertext begin, bytes=%{public}zu", ciphertext.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    const auto decryptRet = internal::PrivacyProtocolImpl().DecryptCiphertext(ciphertext, params, identity, output);
    if (decryptRet != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("decryptCiphertext failed, ret=%{public}d", decryptRet);
    } else {
        LOGI("protocol decryptCiphertext success, bytes=%{public}zu", output.size());
    }
    return decryptRet;
}

int32_t PostProcessSearchResult(const std::vector<Bytes> &fhePlainText,
                                const std::optional<std::vector<Bytes>> &valueCipherText,
                                const PrivacyProtocol &protocol, SearchResult &output) noexcept
{
    LOGI("protocol PostProcessSearchResult postprocess begin, batches=%{public}zu", fhePlainText.size());
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    const auto resultRet =
        internal::PrivacyProtocolImpl().PostProcessSearchResult(fhePlainText, valueCipherText, params, output);
    if (resultRet != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("PostProcessSearchResult postprocess failed, ret=%{public}d", resultRet);
    } else {
        LOGI("protocol PostProcessSearchResult postprocess success");
    }
    return resultRet;
}
} // namespace OHOS::Security::PrivacyComputation
