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

int32_t genPrivacyTarget(const TargetInput &input, const PrivacyProtocol &protocol, const CallerIdentity &identity,
                         Bytes &output) noexcept
{
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto targetRet = internal::PsiPirProtocolImpl().genPrivacyTarget(input, params, identity, output);
    if (targetRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("genPrivacyTarget failed in protocol core, ret=%{public}d", targetRet);
    }
    return targetRet;
}

int32_t privacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements, const PrivacyProtocol &protocol,
                      PrivacySearchResult &output) noexcept
{
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto searchRet = internal::PsiPirProtocolImpl().privacySearch(privacyTarget, elements, params, output);
    if (searchRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("privacySearch failed in protocol core, ret=%{public}d", searchRet);
    }
    return searchRet;
}

int32_t getSearchResult(const PrivacySearchResult &input, const PrivacyProtocol &protocol,
                        const CallerIdentity &identity, SearchResult &output) noexcept
{
    internal::ProtocolParams params;
    const auto ret = internal::MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto resultRet = internal::PsiPirProtocolImpl().getSearchResult(input, params, identity, output);
    if (resultRet != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("getSearchResult failed in protocol core, ret=%{public}d", resultRet);
    }
    return resultRet;
}
} // namespace psi_pir
