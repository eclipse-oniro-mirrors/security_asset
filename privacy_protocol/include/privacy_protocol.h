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

#ifndef PRIVACY_PROTOCOL_H
#define PRIVACY_PROTOCOL_H

#include "privacy_error_code.h"
#include "privacy_protocol_types.h"

namespace PrivacyComputation {

// Generates an encrypted privacy target using the caller-provided identity.
int32_t GenPrivacyTarget(const TargetInput &input, const PrivacyProtocol &privacyProtocol,
                         const CallerIdentity &identity, Bytes &privacyTarget) noexcept;

// B-side HE evaluation (the second step of the northbound flow).
int32_t PrivacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
                      const PrivacyProtocol &privacyProtocol, PrivacySearchResult &privacySearchResult) noexcept;

// Decrypts and decodes one HE ciphertext into uint64 little-endian slots.
// Derives the seed from the trusted identity; failure leaves plaintext unchanged.
int32_t DecryptCiphertext(const Bytes &ciphertext, const PrivacyProtocol &privacyProtocol,
                          const CallerIdentity &identity, Bytes &plaintext) noexcept;

// Matches all decrypted batches and decrypts local PIR associated values without HE keys.
// Batch order must match PrivacySearch output. Failure leaves searchResult unchanged.
int32_t PostProcessSearchResult(const std::vector<Bytes> &fhePlainText,
                                const std::optional<std::vector<Bytes>> &valueCipherText,
                                const PrivacyProtocol &privacyProtocol, SearchResult &searchResult) noexcept;

} // namespace PrivacyComputation

#endif // PRIVACY_PROTOCOL_H
