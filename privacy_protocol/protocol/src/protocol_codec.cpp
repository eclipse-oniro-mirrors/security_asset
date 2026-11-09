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

#include "privacy_log.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace OHOS::Security::PrivacyComputation::internal {
namespace {

constexpr std::uint64_t BGV_PLAIN_MODULUS = 65537;
constexpr std::size_t BGV_POLY_MODULUS_DEGREE = 2048;
constexpr std::size_t AES_KEY_BITS = 256;

int32_t ValidatePackingDimensions(const ProtocolParams &params) noexcept
{
    if (params.compareBits == 0 || params.bitsPerSlot == 0 || params.slotsPerData == 0 ||
        params.dataNumPerPlaintext == 0) {
        LOGE("zero dimensions: compareBits=%{public}zu, bitsPerSlot=%{public}zu, "
             "slotsPerData=%{public}zu, groups=%{public}zu",
             params.compareBits, params.bitsPerSlot, params.slotsPerData, params.dataNumPerPlaintext);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.slotsPerData > std::numeric_limits<std::size_t>::max() / params.bitsPerSlot) {
        LOGE("slotsPerData overflow: slotsPerData=%{public}zu, bitsPerSlot=%{public}zu", params.slotsPerData,
             params.bitsPerSlot);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.compareBits != params.bitsPerSlot * params.slotsPerData) {
        LOGE("compareBits mismatch: compareBits=%{public}zu, bitsPerSlot*slotsPerData=%{public}zu", params.compareBits,
             params.bitsPerSlot * params.slotsPerData);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.compareBits % kBitsPerByte != 0 || params.bitsPerSlot >= kMaxBitsPerSlot) {
        LOGE("unsupported packing: compareBits%%8=%{public}zu, bitsPerSlot=%{public}zu",
             params.compareBits % kBitsPerByte, params.bitsPerSlot);
        return PRIVACY_PROTOCOL_UNSUPPORTED_PROTOCOL;
    }
    const auto slotLimit = std::uint64_t{1} << params.bitsPerSlot;
    if (params.plainModulus < slotLimit) {
        LOGE("plainModulus too small: plainModulus=%{public}llu, slotLimit=%{public}llu",
             static_cast<unsigned long long>(params.plainModulus), static_cast<unsigned long long>(slotLimit));
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

bool IsSupportedMode(ProtocolMode mode) noexcept
{
    return mode == ProtocolMode::INTERSECTION || mode == ProtocolMode::PIR;
}

int32_t ValidateAlgorithmParameters(const ProtocolParams &params) noexcept
{
    if (params.fheAlgorithm != FheAlgorithm::BGV2048_48) {
        LOGE("unsupported FHE algorithm=%{public}u", static_cast<uint32_t>(params.fheAlgorithm));
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.plainModulus != BGV_PLAIN_MODULUS) {
        LOGE("plainModulus mismatch: expected=%{public}llu, actual=%{public}llu",
             static_cast<unsigned long long>(BGV_PLAIN_MODULUS), static_cast<unsigned long long>(params.plainModulus));
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.polyModulusDegree != BGV_POLY_MODULUS_DEGREE) {
        LOGE("polyModulusDegree mismatch: expected=%{public}zu, actual=%{public}zu", BGV_POLY_MODULUS_DEGREE,
             params.polyModulusDegree);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    const bool validSymmetricParams =
        params.mode == ProtocolMode::PIR
            ? params.symAlgorithm == SymAlgorithm::AES_GCM && params.symKeySize == AES_KEY_BITS
            : params.symAlgorithm == SymAlgorithm::NONE && params.symKeySize == kDefaultSymKeySize;
    if (!validSymmetricParams) {
        LOGE("invalid symmetric params: mode=%{public}u, symAlgorithm=%{public}u, symKeySize=%{public}zu",
             static_cast<uint32_t>(params.mode), static_cast<uint32_t>(params.symAlgorithm), params.symKeySize);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t ValidatePlaintextLayout(const ProtocolParams &params) noexcept
{
    if (!IsSupportedMode(params.mode)) {
        LOGE("unsupported mode=%{public}u", static_cast<uint32_t>(params.mode));
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.mode == ProtocolMode::PIR && params.dataNumPerPlaintext % kPirGroupsPerItem != 0) {
        LOGE("PIR requires even dataNumPerPlaintext, got=%{public}zu", params.dataNumPerPlaintext);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    if (params.slotsPerData > std::numeric_limits<std::size_t>::max() / params.dataNumPerPlaintext) {
        LOGE("slotsPerData overflow: slotsPerData=%{public}zu, dataNumPerPlaintext=%{public}zu", params.slotsPerData,
             params.dataNumPerPlaintext);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    if (slotCount != params.polyModulusDegree) {
        LOGE("slotCount mismatch: slotCount=%{public}zu, polyModulusDegree=%{public}zu", slotCount,
             params.polyModulusDegree);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t CalculatePlaintextByteCount(const ProtocolParams &params, std::size_t &byteCount) noexcept
{
    if (params.slotsPerData != 0 &&
        params.dataNumPerPlaintext > std::numeric_limits<std::size_t>::max() / params.slotsPerData) {
        LOGE("slot count overflow: dataNumPerPlaintext=%{public}zu, slotsPerData=%{public}zu",
             params.dataNumPerPlaintext, params.slotsPerData);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    if (slotCount > std::numeric_limits<std::size_t>::max() / sizeof(std::uint64_t)) {
        LOGE("byte count overflow: slotCount=%{public}zu", slotCount);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    byteCount = slotCount * sizeof(std::uint64_t);
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t ValidateSerializedPlaintexts(const std::vector<Bytes> &plaintexts, std::size_t expectedBytes) noexcept
{
    for (std::size_t batch = 0; batch < plaintexts.size(); ++batch) {
        const auto actualBytes = plaintexts[batch].size();
        if (actualBytes != expectedBytes) {
            LOGE("plaintext size mismatch: batch=%{public}zu, expected=%{public}zu, "
                 "actual=%{public}zu",
                 batch, expectedBytes, actualBytes);
            return PRIVACY_PROTOCOL_CODEC_ERROR;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

std::size_t CalculateBatchCount(std::size_t itemCount, std::size_t capacity) noexcept
{
    return itemCount == 0 || capacity == 0 ? 0 : 1 + (itemCount - 1) / capacity;
}

int32_t ValidateSymmetricCiphertexts(const std::vector<Bytes> &ciphertexts) noexcept
{
    for (std::size_t index = 0; index < ciphertexts.size(); ++index) {
        if (ciphertexts[index].empty()) {
            LOGE("empty symmetric ciphertext at index=%{private}zu", index);
            return PRIVACY_PROTOCOL_CODEC_ERROR;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t ValidatePirResultLayout(std::size_t batchCount, const std::optional<std::vector<Bytes>> &valueCipherText,
                                const ProtocolParams &params) noexcept
{
    const auto capacity = params.dataNumPerPlaintext / kPirGroupsPerItem;
    if (capacity == 0) {
        LOGE("PIR capacity is zero, dataNumPerPlaintext=%{public}zu", params.dataNumPerPlaintext);
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    const auto symmetricCount = valueCipherText.has_value() ? valueCipherText->size() : 0;
    const auto expectedBatches = CalculateBatchCount(symmetricCount, capacity);
    if (expectedBatches != batchCount) {
        LOGE("batch count mismatch: expected=%{public}zu, actual=%{public}zu, "
             "symCount=%{public}zu, capacity=%{public}zu",
             expectedBatches, batchCount, symmetricCount, capacity);
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    if (batchCount != 0 && !valueCipherText.has_value()) {
        LOGE("resultCipherText non-empty but valueCipherText missing");
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    if (valueCipherText.has_value()) {
        return ValidateSymmetricCiphertexts(*valueCipherText);
    }
    return PRIVACY_PROTOCOL_SUCCESS;
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
    if (packingResult != PRIVACY_PROTOCOL_SUCCESS) {
        return packingResult;
    }
    const auto layoutResult = ValidatePlaintextLayout(params);
    if (layoutResult != PRIVACY_PROTOCOL_SUCCESS) {
        return layoutResult;
    }
    return ValidateAlgorithmParameters(params);
}

int32_t ProtocolCodec::ValidatePrivacyTarget(const Bytes &privacyTarget) const noexcept
{
    if (privacyTarget.empty()) {
        LOGE("privacyTarget is empty");
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t ProtocolCodec::SerializePlaintext(const Slots &slots, const ProtocolParams &params,
                                          Bytes &output) const noexcept
{
    if (slots.size() != params.polyModulusDegree) {
        LOGE("decoded slot count does not match protocol dimension");
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    Bytes result;
    result.reserve(slots.size() * sizeof(uint64_t));
    for (const auto slot : slots) {
        if (slot >= params.plainModulus) {
            LOGE("decoded slot exceeds plaintext modulus");
            return PRIVACY_PROTOCOL_CODEC_ERROR;
        }
        for (size_t byte = 0; byte < sizeof(uint64_t); ++byte) {
            result.push_back(static_cast<uint8_t>(slot >> (byte * kBitsPerByte)));
        }
    }
    output = std::move(result);
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t ProtocolCodec::ParsePlaintexts(const std::vector<Bytes> &fhePlainText,
                                       const std::optional<std::vector<Bytes>> &valueCipherText,
                                       const ProtocolParams &params, std::vector<Slots> &plaintexts) const noexcept
{
    std::size_t expectedBytes = 0;
    auto ret = CalculatePlaintextByteCount(params, expectedBytes);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    ret = ValidateSerializedPlaintexts(fhePlainText, expectedBytes);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    if (params.mode == ProtocolMode::PIR) {
        ret = ValidatePirResultLayout(fhePlainText.size(), valueCipherText, params);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return ret;
        }
    } else if (valueCipherText.has_value()) {
        LOGE("intersection plaintext result must not contain value ciphertexts");
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    std::vector<Slots> decoded;
    decoded.reserve(fhePlainText.size());
    for (const auto &serialized : fhePlainText) {
        decoded.push_back(DeserializePlaintext(serialized, slotCount));
    }
    plaintexts = std::move(decoded);
    LOGI("plaintext parsing complete, batches=%{public}zu, slotsPerBatch=%{public}zu", fhePlainText.size(), slotCount);
    return PRIVACY_PROTOCOL_SUCCESS;
}

} // namespace OHOS::Security::PrivacyComputation::internal
