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

#include "protocol_codec.h"

#include "psi_pir_log.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace psi_pir::internal {
namespace {

constexpr std::uint64_t kBgvPlainModulus = 65537;
constexpr std::size_t kBgvPolyModulusDegree = 2048;
constexpr std::size_t kAesKeyBits = 256;

int32_t ValidatePackingDimensions(const ProtocolParams &params) noexcept
{
    if (params.compareBits == 0 || params.bitsPerSlot == 0 || params.slotsPerData == 0 ||
        params.dataNumPerPlaintext == 0) {
        PSI_PIR_LOGE("zero dimensions: compareBits=%{public}zu, bitsPerSlot=%{public}zu, "
                     "slotsPerData=%{public}zu, groups=%{public}zu",
                     params.compareBits, params.bitsPerSlot, params.slotsPerData, params.dataNumPerPlaintext);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.slotsPerData > std::numeric_limits<std::size_t>::max() / params.bitsPerSlot) {
        PSI_PIR_LOGE("slotsPerData overflow: slotsPerData=%{public}zu, bitsPerSlot=%{public}zu", params.slotsPerData,
                     params.bitsPerSlot);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.compareBits != params.bitsPerSlot * params.slotsPerData) {
        PSI_PIR_LOGE("compareBits mismatch: compareBits=%{public}zu, bitsPerSlot*slotsPerData=%{public}zu",
                     params.compareBits, params.bitsPerSlot * params.slotsPerData);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.compareBits % kBitsPerByte != 0 || params.bitsPerSlot >= kMaxBitsPerSlot) {
        PSI_PIR_LOGE("unsupported packing: compareBits%%8=%{public}zu, bitsPerSlot=%{public}zu",
                     params.compareBits % kBitsPerByte, params.bitsPerSlot);
        return PSI_PIR_UNSUPPORTED_PROTOCOL;
    }
    const auto slotLimit = std::uint64_t{1} << params.bitsPerSlot;
    if (params.plainModulus < slotLimit) {
        PSI_PIR_LOGE("plainModulus too small: plainModulus=%{public}llu, slotLimit=%{public}llu",
                     static_cast<unsigned long long>(params.plainModulus), static_cast<unsigned long long>(slotLimit));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    return PSI_PIR_SUCCESS;
}

bool IsSupportedMode(ProtocolMode mode) noexcept
{
    return mode == ProtocolMode::PSI || mode == ProtocolMode::PIR;
}

int32_t ValidateAlgorithmParameters(const ProtocolParams &params) noexcept
{
    if (params.fheAlgorithm != FheAlgorithm::BGV2048_48) {
        PSI_PIR_LOGE("unsupported FHE algorithm=%{public}u", static_cast<uint32_t>(params.fheAlgorithm));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.plainModulus != kBgvPlainModulus) {
        PSI_PIR_LOGE("plainModulus mismatch: expected=%{public}llu, actual=%{public}llu",
                     static_cast<unsigned long long>(kBgvPlainModulus),
                     static_cast<unsigned long long>(params.plainModulus));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.polyModulusDegree != kBgvPolyModulusDegree) {
        PSI_PIR_LOGE("polyModulusDegree mismatch: expected=%{public}zu, actual=%{public}zu", kBgvPolyModulusDegree,
                     params.polyModulusDegree);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    const bool validSymmetricParams =
        params.mode == ProtocolMode::PIR
            ? params.symAlgorithm == SymAlgorithm::AES_GCM && params.symKeySize == kAesKeyBits
            : params.symAlgorithm == SymAlgorithm::NONE && params.symKeySize == kDefaultSymKeySize;
    if (!validSymmetricParams) {
        PSI_PIR_LOGE("invalid symmetric params: mode=%{public}u, symAlgorithm=%{public}u, symKeySize=%{public}zu",
                     static_cast<uint32_t>(params.mode), static_cast<uint32_t>(params.symAlgorithm), params.symKeySize);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    return PSI_PIR_SUCCESS;
}

int32_t ValidatePlaintextLayout(const ProtocolParams &params) noexcept
{
    if (!IsSupportedMode(params.mode)) {
        PSI_PIR_LOGE("unsupported mode=%{public}u", static_cast<uint32_t>(params.mode));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.mode == ProtocolMode::PIR && params.dataNumPerPlaintext % kPirGroupsPerItem != 0) {
        PSI_PIR_LOGE("PIR requires even dataNumPerPlaintext, got=%{public}zu", params.dataNumPerPlaintext);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.slotsPerData > std::numeric_limits<std::size_t>::max() / params.dataNumPerPlaintext) {
        PSI_PIR_LOGE("slotsPerData overflow: slotsPerData=%{public}zu, dataNumPerPlaintext=%{public}zu",
                     params.slotsPerData, params.dataNumPerPlaintext);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    if (slotCount != params.polyModulusDegree) {
        PSI_PIR_LOGE("slotCount mismatch: slotCount=%{public}zu, polyModulusDegree=%{public}zu", slotCount,
                     params.polyModulusDegree);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    return PSI_PIR_SUCCESS;
}

int32_t CalculatePlaintextByteCount(const ProtocolParams &params, std::size_t &byteCount) noexcept
{
    if (params.slotsPerData != 0 &&
        params.dataNumPerPlaintext > std::numeric_limits<std::size_t>::max() / params.slotsPerData) {
        PSI_PIR_LOGE("slot count overflow: dataNumPerPlaintext=%{public}zu, slotsPerData=%{public}zu",
                     params.dataNumPerPlaintext, params.slotsPerData);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    if (slotCount > std::numeric_limits<std::size_t>::max() / sizeof(std::uint64_t)) {
        PSI_PIR_LOGE("byte count overflow: slotCount=%{public}zu", slotCount);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    byteCount = slotCount * sizeof(std::uint64_t);
    return PSI_PIR_SUCCESS;
}

int32_t ValidateSerializedPlaintexts(const PrivacySearchResult &result, std::size_t expectedBytes) noexcept
{
    for (std::size_t batch = 0; batch < result.fheCipherText.size(); ++batch) {
        const auto actualBytes = result.fheCipherText[batch].size();
        if (actualBytes != expectedBytes) {
            PSI_PIR_LOGE("plaintext size mismatch: batch=%{public}zu, expected=%{public}zu, "
                         "actual=%{public}zu",
                         batch, expectedBytes, actualBytes);
            return PSI_PIR_CODEC_ERROR;
        }
    }
    return PSI_PIR_SUCCESS;
}

std::size_t CalculateBatchCount(std::size_t itemCount, std::size_t capacity) noexcept
{
    return itemCount == 0 ? 0 : 1 + (itemCount - 1) / capacity;
}

int32_t ValidateSymmetricCiphertexts(const std::vector<Bytes> &ciphertexts) noexcept
{
    for (std::size_t index = 0; index < ciphertexts.size(); ++index) {
        if (ciphertexts[index].empty()) {
            PSI_PIR_LOGE("empty symmetric ciphertext at index=%{private}zu", index);
            return PSI_PIR_CODEC_ERROR;
        }
    }
    return PSI_PIR_SUCCESS;
}

int32_t ValidatePirResultLayout(const PrivacySearchResult &result, const ProtocolParams &params) noexcept
{
    const auto capacity = params.dataNumPerPlaintext / kPirGroupsPerItem;
    if (capacity == 0) {
        PSI_PIR_LOGE("PIR capacity is zero, dataNumPerPlaintext=%{public}zu", params.dataNumPerPlaintext);
        return PSI_PIR_CODEC_ERROR;
    }
    const auto symmetricCount = result.symCipherText.has_value() ? result.symCipherText->size() : 0;
    const auto expectedBatches = CalculateBatchCount(symmetricCount, capacity);
    if (expectedBatches != result.fheCipherText.size()) {
        PSI_PIR_LOGE("batch count mismatch: expected=%{public}zu, actual=%{public}zu, "
                     "symCount=%{public}zu, capacity=%{public}zu",
                     expectedBatches, result.fheCipherText.size(), symmetricCount, capacity);
        return PSI_PIR_CODEC_ERROR;
    }
    if (!result.fheCipherText.empty() && !result.symCipherText.has_value()) {
        PSI_PIR_LOGE("fheCipherText non-empty but symCipherText missing");
        return PSI_PIR_CODEC_ERROR;
    }
    if (result.symCipherText.has_value()) {
        return ValidateSymmetricCiphertexts(*result.symCipherText);
    }
    return PSI_PIR_SUCCESS;
}

std::uint64_t DecodeLittleEndianSlot(const Bytes &serialized, std::size_t offset) noexcept
{
    std::uint64_t value = 0;
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        value |= static_cast<std::uint64_t>(serialized[offset + byte]) << (byte * kBitsPerByte);
    }
    return value;
}

Slots DeserializePlaintext(const Bytes &serialized, std::size_t slotCount)
{
    Slots slots(slotCount, 0);
    for (std::size_t slot = 0; slot < slotCount; ++slot) {
        slots[slot] = DecodeLittleEndianSlot(serialized, slot * sizeof(std::uint64_t));
    }
    return slots;
}

} // namespace

int32_t ProtocolCodec::ValidateProtocol(const ProtocolParams &params) const noexcept
{
    const auto packingResult = ValidatePackingDimensions(params);
    if (packingResult != PSI_PIR_SUCCESS) {
        return packingResult;
    }
    const auto layoutResult = ValidatePlaintextLayout(params);
    if (layoutResult != PSI_PIR_SUCCESS) {
        return layoutResult;
    }
    return ValidateAlgorithmParameters(params);
}

int32_t ProtocolCodec::ValidatePrivacyTarget(const Bytes &privacyTarget) const noexcept
{
    if (privacyTarget.empty()) {
        PSI_PIR_LOGE("privacyTarget is empty");
        return PSI_PIR_CODEC_ERROR;
    }
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolCodec::ValidateSearchResultLayout(const PrivacySearchResult &result,
                                                  const ProtocolParams &params) const noexcept
{
    return params.mode == ProtocolMode::PIR ? ValidatePirResultLayout(result, params) : PSI_PIR_SUCCESS;
}

int32_t ProtocolCodec::ValidatePlainSearchResult(const PrivacySearchResult &result,
                                                 const ProtocolParams &params) const noexcept
{
    std::size_t expectedBytes = 0;
    auto resultCode = CalculatePlaintextByteCount(params, expectedBytes);
    if (resultCode != PSI_PIR_SUCCESS) {
        return resultCode;
    }
    resultCode = ValidateSerializedPlaintexts(result, expectedBytes);
    if (resultCode != PSI_PIR_SUCCESS) {
        return resultCode;
    }
    return ValidateSearchResultLayout(result, params);
}

int32_t ProtocolCodec::SerializePlaintext(const Slots &slots, const ProtocolParams &params,
                                          Bytes &output) const noexcept
{
    if (slots.size() != params.polyModulusDegree) {
        PSI_PIR_LOGE("decoded slot count does not match protocol dimension");
        return PSI_PIR_CODEC_ERROR;
    }
    Bytes result;
    result.reserve(slots.size() * sizeof(uint64_t));
    for (const auto slot : slots) {
        if (slot >= params.plainModulus) {
            PSI_PIR_LOGE("decoded slot exceeds plaintext modulus");
            return PSI_PIR_CODEC_ERROR;
        }
        for (size_t byte = 0; byte < sizeof(uint64_t); ++byte) {
            result.push_back(static_cast<uint8_t>(slot >> (byte * kBitsPerByte)));
        }
    }
    output = std::move(result);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolCodec::ParsePlaintexts(const PrivacySearchResult &result, const ProtocolParams &params,
                                       std::vector<Slots> &plaintexts) const noexcept
{
    const auto resultCode = ValidatePlainSearchResult(result, params);
    if (resultCode != PSI_PIR_SUCCESS) {
        return resultCode;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    std::vector<Slots> decodedPlaintexts;
    decodedPlaintexts.reserve(result.fheCipherText.size());
    for (const auto &serialized : result.fheCipherText) {
        decodedPlaintexts.push_back(DeserializePlaintext(serialized, slotCount));
    }
    plaintexts = std::move(decodedPlaintexts);
    PSI_PIR_LOGI("plaintext parsing complete, batches=%{public}zu, slotsPerBatch=%{public}zu", plaintexts.size(),
                 slotCount);
    return PSI_PIR_SUCCESS;
}

} // namespace psi_pir::internal
