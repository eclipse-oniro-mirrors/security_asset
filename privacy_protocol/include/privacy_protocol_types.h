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

#ifndef PRIVACY_PROTOCOL_TYPES_H
#define PRIVACY_PROTOCOL_TYPES_H

#include <cstdint>
#include <optional>
#include <vector>

namespace PrivacyComputation {

using Bytes = std::vector<std::uint8_t>;

// Shared hash values; omitted hashAlg means no hashing.
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
    PRIVATE_SET_INTERSECTION = 0,
    PIR_PROTOCOL = 1,
};

struct PrivacyProtocol {
    DataSetSize dataSetSize{DataSetSize::SIZE_512};
    ProtocolType protocolType{ProtocolType::PRIVATE_SET_INTERSECTION};
};

struct CallerIdentity {
    uint32_t uid{0};
    uint32_t userId{0};
};

struct TargetInput {
    Bytes elemData;
    HashAlg hashAlg{HashAlg::NONE};
};

struct Element {
    Bytes elemKey;
    HashAlg hashAlg{HashAlg::NONE};
    std::optional<Bytes> elemValue;
};

// PrivacySearch produces encrypted batches. DecryptCiphertext handles each HE batch;
// PostProcessSearchResult consumes decoded plaintext batches and locally held symmetric frames.
struct PrivacySearchResult {
    std::vector<Bytes> resultCipherText;
    std::optional<std::vector<Bytes>> valueCipherText;
};

struct SearchResult {
    bool matchedResult{false};
    std::optional<std::vector<Bytes>> attachedValues;
};

} // namespace PrivacyComputation

#endif // PRIVACY_PROTOCOL_TYPES_H
