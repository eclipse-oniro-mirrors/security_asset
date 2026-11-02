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

#ifndef PSI_PIR_PROTOCOL_IMPL_H
#define PSI_PIR_PROTOCOL_IMPL_H

#include "protocol_codec.h"
#include "protocol_encoder.h"
#include "protocol_params.h"
#include "psi_pir_types.h"

namespace psi_pir::internal {

class PsiPirProtocolImpl final {
public:
    // Complete target generation: validate, encode, derive the caller seed, then encrypt.
    int32_t GenPrivacyTarget(const TargetInput &input, const ProtocolParams &params, const CallerIdentity &identity,
                             Bytes &result) const noexcept;
    int32_t PrivacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
                          const ProtocolParams &params, PrivacySearchResult &result) const noexcept;
    int32_t DecryptCiphertext(const Bytes &ciphertext, const ProtocolParams &params,
        const CallerIdentity &identity, Bytes &plaintext) const noexcept;
    int32_t PostProcessSearchResult(const std::vector<Bytes> &fhePlainText,
        const std::optional<std::vector<Bytes>> &symCipherText, const ProtocolParams &params,
        SearchResult &result) const noexcept;
    // Internal plaintext stage, also used by tests that inject decrypted slot data.
    int32_t ProcessDecryptedSearchResult(const PrivacySearchResult &privacySearchResult, const ProtocolParams &params,
                                         SearchResult &result) const noexcept;

private:
    ProtocolCodec codec_;
    ProtocolEncoder encoder_;
};

} // namespace psi_pir::internal

#endif // PSI_PIR_PROTOCOL_IMPL_H
