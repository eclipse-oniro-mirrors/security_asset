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

#ifndef PSI_PIR_PROTOCOL_H
#define PSI_PIR_PROTOCOL_H

#include "psi_pir_error_code.h"
#include "psi_pir_types.h"

namespace psi_pir {

// Generates an encrypted privacy target using the caller-provided identity.
int32_t genPrivacyTarget(const TargetInput &input, const PrivacyProtocol &privacyProtocol,
                         const OperationIdentity &identity, Bytes &privacyTarget) noexcept;

// B-side HE evaluation (the second step of the northbound flow).
int32_t privacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
                      const PrivacyProtocol &privacyProtocol, PrivacySearchResult &privacySearchResult) noexcept;

// Decrypts HE ciphertext batches, then performs shared open-source match detection
// and PIR attached-value processing. Failure leaves searchResult unchanged.
// CCZ selects raw HE decryption only; an unavailable backend returns an error.
int32_t getSearchResult(const PrivacySearchResult &privacySearchResult, const PrivacyProtocol &privacyProtocol,
                        const OperationIdentity &identity, DecryptionBackend backend,
                        SearchResult &searchResult) noexcept;

} // namespace psi_pir

#endif // PSI_PIR_PROTOCOL_H
