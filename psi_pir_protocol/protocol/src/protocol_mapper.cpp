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

#include "protocol_mapper.h"

#include "psi_pir_log.h"

#include <array>

namespace psi_pir::internal {
namespace {

constexpr std::size_t kCompareBits64 = 64;
constexpr std::size_t kCompareBits128 = 128;
constexpr std::size_t kBitsPerSlot16 = 16;
constexpr std::size_t kSlotsPerData4 = 4;
constexpr std::size_t kSlotsPerData8 = 8;
constexpr std::size_t kDataNumPerPlaintext256 = 256;
constexpr std::size_t kDataNumPerPlaintext512 = 512;
constexpr std::uint64_t kPlainModulus65537 = 65537;
constexpr std::size_t kPolyModulusDegree2048 = 2048;
constexpr std::size_t kSymKeySize256 = 256;

struct ProtocolKey {
    DataSetSize dataSetSize;
    ProtocolType protocolType;

    constexpr bool operator==(const ProtocolKey &other) const noexcept
    {
        return dataSetSize == other.dataSetSize && protocolType == other.protocolType;
    }
};

struct ProtocolPreset {
    ProtocolKey key;
    ProtocolParams params;
};

constexpr std::array<ProtocolPreset, 3> kProtocolPresets{{
    {
        {DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL},
        {ProtocolMode::PSI, kCompareBits64, kBitsPerSlot16, kSlotsPerData4, kDataNumPerPlaintext512, kPlainModulus65537,
         kPolyModulusDegree2048, FheAlgorithm::BGV2048_48, kDefaultSymKeySize, SymAlgorithm::NONE},
    },
    {
        {DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL},
        {ProtocolMode::PIR, kCompareBits64, kBitsPerSlot16, kSlotsPerData4, kDataNumPerPlaintext512, kPlainModulus65537,
         kPolyModulusDegree2048, FheAlgorithm::BGV2048_48, kSymKeySize256, SymAlgorithm::AES_GCM},
    },
    {
        {DataSetSize::SIZE_128, ProtocolType::PIR_PROTOCOL},
        {ProtocolMode::PIR, kCompareBits128, kBitsPerSlot16, kSlotsPerData8, kDataNumPerPlaintext256,
         kPlainModulus65537, kPolyModulusDegree2048, FheAlgorithm::BGV2048_48, kSymKeySize256, SymAlgorithm::AES_GCM},
    },
}};

constexpr bool HasUniqueProtocolKeys() noexcept
{
    for (std::size_t first = 0; first < kProtocolPresets.size(); ++first) {
        for (std::size_t second = first + 1; second < kProtocolPresets.size(); ++second) {
            if (kProtocolPresets[first].key == kProtocolPresets[second].key) {
                return false;
            }
        }
    }
    return true;
}

constexpr bool HasValidPresetDimensions() noexcept
{
    for (const auto &preset : kProtocolPresets) {
        const auto &params = preset.params;
        if (params.compareBits != params.bitsPerSlot * params.slotsPerData ||
            params.dataNumPerPlaintext * params.slotsPerData != params.polyModulusDegree) {
            return false;
        }
    }
    return true;
}

const ProtocolParams *FindProtocolPreset(const ProtocolKey &key) noexcept
{
    for (const auto &preset : kProtocolPresets) {
        if (preset.key == key) {
            return &preset.params;
        }
    }
    return nullptr;
}

static_assert(HasUniqueProtocolKeys(), "duplicate privacy protocol preset key");
static_assert(HasValidPresetDimensions(), "invalid privacy protocol preset dimensions");

} // namespace

int32_t MapPrivacyProtocol(const PrivacyProtocol &privacyProtocol, ProtocolParams &params) noexcept
{
    const ProtocolKey key{privacyProtocol.dataSetSize, privacyProtocol.protocolType};
    const auto *preset = FindProtocolPreset(key);
    if (preset == nullptr) {
        LOGE("protocol preset not found: dataSetSize=%{public}u, protocolType=%{public}u",
                     static_cast<uint32_t>(privacyProtocol.dataSetSize),
                     static_cast<uint32_t>(privacyProtocol.protocolType));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    params = *preset;
    LOGI("protocol mapped: mode=%{public}u, compareBits=%{public}zu, "
                 "bitsPerSlot=%{public}zu, slotsPerData=%{public}zu, groups=%{public}zu",
                 static_cast<uint32_t>(params.mode), params.compareBits, params.bitsPerSlot, params.slotsPerData,
                 params.dataNumPerPlaintext);
    return PSI_PIR_SUCCESS;
}

} // namespace psi_pir::internal
