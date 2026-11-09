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

#ifndef PRIVACY_PROTOCOL_PROTOCOL_PARAMS_H
#define PRIVACY_PROTOCOL_PROTOCOL_PARAMS_H

#include "privacy_protocol_types.h"

#include <cstddef>
#include <cstdint>

namespace OHOS::Security::PrivacyComputation::internal {

enum class ProtocolMode : std::uint32_t {
    INTERSECTION = 0,
    PIR = 1,
};

enum class FheAlgorithm : std::uint32_t {
    BGV2048_48 = 0,
};

enum class SymAlgorithm : std::uint32_t {
    NONE = 0,
    AES_GCM = 1,
};

constexpr std::size_t kDefaultCompareBits = 64;
constexpr std::size_t kDefaultBitsPerSlot = 16;
constexpr std::size_t kDefaultSlotsPerData = 4;
constexpr std::size_t kDefaultDataNumberPerPlaintext = 512;
constexpr std::uint64_t kDefaultPlainModulus = 65537;
constexpr std::size_t kDefaultPolyModulusDegree = 2048;
constexpr std::size_t kDefaultSymKeySize = 0;

struct ProtocolParams {
    ProtocolMode mode{ProtocolMode::INTERSECTION};
    std::size_t compareBits{kDefaultCompareBits};
    std::size_t bitsPerSlot{kDefaultBitsPerSlot};
    std::size_t slotsPerData{kDefaultSlotsPerData};
    std::size_t dataNumPerPlaintext{kDefaultDataNumberPerPlaintext};
    std::uint64_t plainModulus{kDefaultPlainModulus};
    std::size_t polyModulusDegree{kDefaultPolyModulusDegree};
    FheAlgorithm fheAlgorithm{FheAlgorithm::BGV2048_48};
    std::size_t symKeySize{kDefaultSymKeySize};
    SymAlgorithm symAlgorithm{SymAlgorithm::NONE};
};

} // namespace OHOS::Security::PrivacyComputation::internal

#endif // PRIVACY_PROTOCOL_PROTOCOL_PARAMS_H
