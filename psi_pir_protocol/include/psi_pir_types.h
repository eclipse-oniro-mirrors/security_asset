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

#ifndef PSI_PIR_PROTOCOL_TYPES_H
#define PSI_PIR_PROTOCOL_TYPES_H

#include <cstdint>
#include <optional>
#include <vector>

namespace psi_pir {

using Bytes = std::vector<std::uint8_t>;

// Protocol-internal values. NAPI maps northbound SHA256=0 explicitly to SHA256;
// a direct cast of that value would incorrectly select NONE.
enum class HashAlg : std::uint32_t {
    NONE = 0,
    SHA256 = 1,
    SHA384 = 2,
    SHA512 = 3,
};

enum class DataSetSize : std::uint32_t {
    SIZE_128 = 0,
    SIZE_256 = 1,
    SIZE_512 = 2,
};

enum class ProtocolType : std::uint32_t {
    PSI_PROTOCOL = 0,
    PIR_PROTOCOL = 1,
};

struct PrivacyProtocol {
    DataSetSize dataSetSize{DataSetSize::SIZE_512};
    ProtocolType protocolType{ProtocolType::PSI_PROTOCOL};
};

struct OperationIdentity {
    uint32_t uid{0};
    uint32_t userId{0};
};

struct TargetInput {
    Bytes elemData;
    HashAlg hashAlg{HashAlg::SHA256};
    bool isCt{false};
};

// Selects HE decryption only; result post-processing is always local.
enum class DecryptionBackend { SOFT, CCZ };

struct Element {
    Bytes elemKey;
    HashAlg hashAlg{HashAlg::NONE};
    std::optional<Bytes> elemValue;
};

// privacySearch produces HE ciphertext batches. getSearchResult decrypts them
// before applying the common protocol post-processing.
struct PrivacySearchResult {
    std::vector<Bytes> fheCipherText;
    std::optional<std::vector<Bytes>> symCipherText;
};

struct SearchResult {
    bool matchedResult{false};
    std::optional<std::vector<Bytes>> attachedValues;
};

} // namespace psi_pir

#endif // PSI_PIR_PROTOCOL_TYPES_H
