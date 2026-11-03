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

#include "protocol_encoder.h"

#include "provider_registry.h"
#include "psi_pir_log.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace psi_pir::internal {
namespace {

constexpr std::size_t kMaxRandomAttempts = 128;
[[maybe_unused]] constexpr std::size_t SHA256_DIGEST_SIZE = 32;
[[maybe_unused]] constexpr std::size_t SHA384_DIGEST_SIZE = 48;
[[maybe_unused]] constexpr std::size_t SHA512_DIGEST_SIZE = 64;

bool IsValidHashAlgorithm(HashAlg algorithm) noexcept
{
    return algorithm == HashAlg::NONE || algorithm == HashAlg::SHA256 || algorithm == HashAlg::SHA384 ||
           algorithm == HashAlg::SHA512;
}

bool HasValidPackingDimensions(const ProtocolParams &params) noexcept
{
    return params.compareBits != 0 && params.compareBits % kBitsPerByte == 0 && params.bitsPerSlot != 0 &&
           params.bitsPerSlot < kMaxBitsPerSlot && params.slotsPerData != 0 &&
           params.slotsPerData <= std::numeric_limits<std::size_t>::max() / params.bitsPerSlot &&
           params.compareBits == params.bitsPerSlot * params.slotsPerData;
}

std::uint64_t ExtractBitsFromTail(const Bytes &input, std::size_t startBit, std::size_t bitCount) noexcept
{
    // The TEE numbers bit zero as the least-significant bit of the final byte.
    std::uint64_t result = 0;
    std::size_t bitsCollected = 0;
    while (bitsCollected < bitCount) {
        const auto bitPosition = startBit + bitsCollected;
        const auto byteFromEnd = bitPosition / kBitsPerByte;
        const auto bitInByte = bitPosition % kBitsPerByte;
        const auto roundBits = std::min(bitCount - bitsCollected, kBitsPerByte - bitInByte);
        const auto roundMask =
            static_cast<std::uint8_t>(roundBits == kBitsPerByte ? 0xffU : (std::uint32_t{1} << roundBits) - 1U);
        const auto source = input[input.size() - byteFromEnd - 1];
        const auto value = static_cast<std::uint64_t>((source >> bitInByte) & roundMask);
        result |= value << bitsCollected;
        bitsCollected += roundBits;
    }
    return result;
}

struct MaskSamplingConfig {
    std::uint64_t candidateRange;
    std::size_t bytesPerValue;
    std::uint8_t highByteMask;
};

std::size_t CountSignificantBits(std::uint64_t value) noexcept
{
    std::size_t bitCount = 0;
    for (; value != 0; value >>= 1) {
        ++bitCount;
    }
    return bitCount;
}

std::uint8_t CreateHighByteMask(std::size_t randomBits) noexcept
{
    if (randomBits == 0) {
        return 0;
    }
    const auto highBits = randomBits % kBitsPerByte;
    if (highBits == 0) {
        return 0xff;
    }
    return static_cast<std::uint8_t>((std::uint16_t{1} << highBits) - 1);
}

MaskSamplingConfig CreateMaskSamplingConfig(std::uint64_t plainModulus) noexcept
{
    const auto candidateRange = plainModulus - 1;
    const auto randomBits = CountSignificantBits(candidateRange - 1);
    const auto bytesPerValue = std::max<std::size_t>(1, (randomBits + kBitsPerByte - 1) / kBitsPerByte);
    return {candidateRange, bytesPerValue, CreateHighByteMask(randomBits)};
}

std::uint64_t DecodeMaskCandidate(const Bytes &randomBytes, std::size_t offset,
                                  const MaskSamplingConfig &config) noexcept
{
    std::uint64_t candidate = randomBytes[offset] & config.highByteMask;
    for (std::size_t byte = 1; byte < config.bytesPerValue; ++byte) {
        candidate = (candidate << kBitsPerByte) | randomBytes[offset + byte];
    }
    return candidate;
}

int32_t ApplyMaskCandidates(const Bytes &randomBytes, const std::vector<std::size_t> &indices,
                            const MaskSamplingConfig &config, Slots &maskVector,
                            std::vector<std::size_t> &rejected) noexcept
{
    rejected.clear();
    rejected.reserve(indices.size());
    for (std::size_t item = 0; item < indices.size(); ++item) {
        const auto index = indices[item];
        if (index >= maskVector.size()) {
            LOGE("mask index out of range: index=%{public}zu, slots=%{public}zu", index, maskVector.size());
            return PSI_PIR_INVALID_ARGUMENT;
        }
        const auto candidate = DecodeMaskCandidate(randomBytes, item * config.bytesPerValue, config);
        if (candidate >= config.candidateRange) {
            rejected.push_back(index);
            continue;
        }
        maskVector[index] = candidate + 1;
    }
    return PSI_PIR_SUCCESS;
}

int32_t SampleMaskCandidates(IRandomSource &randomSource, const std::vector<std::size_t> &indices,
                             const MaskSamplingConfig &config, Slots &maskVector,
                             std::vector<std::size_t> &rejected) noexcept
{
    if (indices.size() > std::numeric_limits<std::size_t>::max() / config.bytesPerValue) {
        LOGE("random byte count overflow: entries=%{public}zu, bytesPerEntry=%{public}zu", indices.size(),
                     config.bytesPerValue);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    Bytes randomBytes(indices.size() * config.bytesPerValue);
    const auto resultCode = randomSource.Fill(randomBytes);
    if (resultCode != PSI_PIR_SUCCESS) {
        LOGE("random fill failed: requested=%{public}zu, ret=%{public}d", randomBytes.size(), resultCode);
        return resultCode;
    }
    return ApplyMaskCandidates(randomBytes, indices, config, maskVector, rejected);
}

int32_t FillNonZeroMaskEntries(IRandomSource &randomSource, const std::vector<std::size_t> &indices,
                               std::uint64_t plainModulus, Slots &maskVector) noexcept
{
    if (plainModulus <= 1) {
        LOGE("plain modulus cannot produce a non-zero mask, modulus=%{public}llu",
                     static_cast<unsigned long long>(plainModulus));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    // An accepted c in [0, p - 2] maps to c + 1 in F_p^*; out-of-range
    // candidates are retried so modulo reduction cannot introduce zero masks.
    const auto config = CreateMaskSamplingConfig(plainModulus);
    std::vector<std::size_t> pending = indices;
    for (std::size_t attempt = 0; attempt < kMaxRandomAttempts && !pending.empty(); ++attempt) {
        std::vector<std::size_t> rejected;
        const auto resultCode = SampleMaskCandidates(randomSource, pending, config, maskVector, rejected);
        if (resultCode != PSI_PIR_SUCCESS) {
            return resultCode;
        }
        pending = std::move(rejected);
    }
    if (!pending.empty()) {
        LOGE("non-zero mask sampling exhausted, remaining=%{public}zu", pending.size());
        return PSI_PIR_RANDOM_ERROR;
    }
    return PSI_PIR_SUCCESS;
}

int32_t ValidateMaskDimensions(const ProtocolParams &params) noexcept
{
    if (params.bitsPerSlot == 0 || params.bitsPerSlot >= kMaxBitsPerSlot || params.plainModulus <= 1 ||
        params.slotsPerData == 0 || params.dataNumPerPlaintext == 0) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (params.slotsPerData > std::numeric_limits<std::size_t>::max() / params.dataNumPerPlaintext) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    return PSI_PIR_SUCCESS;
}

std::vector<std::size_t> CreateSequentialIndices(std::size_t count)
{
    std::vector<std::size_t> indices(count);
    for (std::size_t index = 0; index < count; ++index) {
        indices[index] = index;
    }
    return indices;
}

[[maybe_unused]] void NegateFieldElements(Slots &slots, std::uint64_t plainModulus) noexcept
{
    for (auto &slot : slots) {
        slot %= plainModulus;
        slot = slot == 0 ? 0 : plainModulus - slot;
    }
}

bool ContainsSlotRange(const Slots &plain, std::size_t offset, std::size_t slotCount) noexcept
{
    return offset <= plain.size() && slotCount <= plain.size() - offset;
}

void WriteSlotBitsToTail(std::uint64_t value, std::size_t slot, std::size_t bitsPerSlot, Bytes &output) noexcept
{
    for (std::size_t bitInSlot = 0; bitInSlot < bitsPerSlot; ++bitInSlot) {
        if (((value >> bitInSlot) & 1U) == 0) {
            continue;
        }
        const auto bitFromEnd = slot * bitsPerSlot + bitInSlot;
        const auto byteFromEnd = bitFromEnd / kBitsPerByte;
        const auto bitInByte = bitFromEnd % kBitsPerByte;
        output[output.size() - byteFromEnd - 1] |= static_cast<std::uint8_t>(1U << bitInByte);
    }
}

} // namespace

int32_t ProtocolEncoder::ValidateHashAlgorithm(HashAlg algorithm) const noexcept
{
    if (!IsValidHashAlgorithm(algorithm)) {
        LOGE("invalid hash algorithm=%{public}u", static_cast<uint32_t>(algorithm));
        return PSI_PIR_INVALID_ARGUMENT;
    }
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::Hash(HashAlg algorithm, const Bytes &input, Bytes &digest) const noexcept
{
    const auto validationResult = ValidateHashAlgorithm(algorithm);
    if (validationResult != PSI_PIR_SUCCESS) {
        return validationResult;
    }
    const auto hashProvider = GetHashProvider();
    if (!hashProvider) {
        LOGE("hash provider not registered");
        return PSI_PIR_HASH_ERROR;
    }
    Bytes output;
    const auto digestRet = hashProvider->Digest(algorithm, input, output);
    if (digestRet != PSI_PIR_SUCCESS) {
        LOGE("hash provider digest failed, algorithm=%{public}u, ret=%{public}d",
                     static_cast<uint32_t>(algorithm), digestRet);
        return digestRet;
    }
    digest = std::move(output);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::BytesToSlots(const Bytes &input, const ProtocolParams &params, Slots &slots) const noexcept
{
    if (!HasValidPackingDimensions(params)) {
        LOGE("invalid packing dimensions");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    const auto requiredBytes = params.compareBits / kBitsPerByte;
    if (input.size() < requiredBytes) {
        LOGE("input too short: size=%{public}zu, required=%{public}zu", input.size(), requiredBytes);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    Slots output(params.slotsPerData, 0);
    for (std::size_t slot = 0; slot < params.slotsPerData; ++slot) {
        output[slot] = ExtractBitsFromTail(input, slot * params.bitsPerSlot, params.bitsPerSlot);
    }
    slots = std::move(output);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::EncodeServerVector(const Element &element, const ProtocolParams &params,
                                            Slots &serverVector) const noexcept
{
    Bytes digest;
    auto resultCode = Hash(element.hashAlg, element.elemKey, digest);
    if (resultCode != PSI_PIR_SUCCESS) {
        return resultCode;
    }
    Slots slots;
    resultCode = BytesToSlots(digest, params, slots);
    if (resultCode != PSI_PIR_SUCCESS) {
        return resultCode;
    }
    serverVector = std::move(slots);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::EncodeTargetVector(const Element &element, const ProtocolParams &params,
                                            Slots &targetVector) const noexcept
{
    if (!HasValidPackingDimensions(params) || params.polyModulusDegree == 0 ||
        params.polyModulusDegree % params.slotsPerData != 0 ||
        params.dataNumPerPlaintext != params.polyModulusDegree / params.slotsPerData) {
        LOGE("invalid target packing dimensions");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    Slots comparison;
    const auto ret = EncodeServerVector(element, params, comparison);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    // Zero padding in the final server batch must not match the query group.
    if (std::all_of(comparison.begin(), comparison.end(), [](auto slot) { return slot == 0; })) {
        LOGE("all-zero target comparison group is unsupported");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (std::any_of(comparison.begin(), comparison.end(),
                    [&params](auto slot) { return slot >= params.plainModulus; })) {
        LOGE("target comparison slot exceeds plaintext modulus");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    Slots output(params.polyModulusDegree);
    for (std::size_t slot = 0; slot < output.size(); ++slot) {
        output[slot] = comparison[slot % comparison.size()];
    }
    targetVector = std::move(output);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::EncodeSeed(const Bytes &seed, const ProtocolParams &params, Slots &seedSlots) const noexcept
{
    return BytesToSlots(seed, params, seedSlots);
}

int32_t ProtocolEncoder::ExtractSeed(const Slots &plain, std::size_t offset, const ProtocolParams &params,
                                     Bytes &seed) const noexcept
{
    if (!HasValidPackingDimensions(params)) {
        LOGE("invalid packing dimensions");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    if (!ContainsSlotRange(plain, offset, params.slotsPerData)) {
        LOGE("slot range out of bounds: offset=%{private}zu, slotsPerData=%{public}zu, "
                     "plainSize=%{public}zu",
                     offset, params.slotsPerData, plain.size());
        return PSI_PIR_CODEC_ERROR;
    }
    const auto slotMask = (std::uint64_t{1} << params.bitsPerSlot) - 1;
    Bytes output(params.compareBits / kBitsPerByte, 0);
    for (std::size_t slot = 0; slot < params.slotsPerData; ++slot) {
        const auto value = (plain[offset + slot] % params.plainModulus) & slotMask;
        WriteSlotBitsToTail(value, slot, params.bitsPerSlot, output);
    }
    seed = std::move(output);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::GenerateSeed(const ProtocolParams &params, Bytes &seed) const noexcept
{
    const auto randomSource = GetRandomSource();
    if (!randomSource) {
        LOGE("random source not registered");
        return PSI_PIR_RANDOM_ERROR;
    }
    Bytes output(params.compareBits / kBitsPerByte);
    const auto resultCode = randomSource->Fill(output);
    if (resultCode != PSI_PIR_SUCCESS) {
        LOGE("seed random fill failed, requested=%{public}zu, ret=%{public}d", output.size(), resultCode);
        return resultCode;
    }
    seed = std::move(output);
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::GenerateMaskVector(const ProtocolParams &params, Slots &maskVector) const noexcept
{
    const auto validationResult = ValidateMaskDimensions(params);
    if (validationResult != PSI_PIR_SUCCESS) {
        LOGE("invalid mask dimensions: bitsPerSlot=%{public}zu, plainModulus=%{public}llu",
                     params.bitsPerSlot, static_cast<unsigned long long>(params.plainModulus));
        return validationResult;
    }
    const auto randomSource = GetRandomSource();
    if (!randomSource) {
        LOGE("random source not registered");
        return PSI_PIR_RANDOM_ERROR;
    }
    const auto slotCount = params.dataNumPerPlaintext * params.slotsPerData;
    LOGI("mask generation begin, slots=%{public}zu, modulus=%{public}llu, mode=%{public}u", slotCount,
                 static_cast<unsigned long long>(params.plainModulus), static_cast<uint32_t>(params.mode));
    Slots output(slotCount, 0);
    const auto indices = CreateSequentialIndices(slotCount);
    auto resultCode = FillNonZeroMaskEntries(*randomSource, indices, params.plainModulus, output);
    if (resultCode != PSI_PIR_SUCCESS) {
        LOGE("FillNonZeroMaskEntries failed, ret=%{public}d", resultCode);
        return resultCode;
    }

    maskVector = std::move(output);
    LOGI("mask generation complete, slots=%{public}zu", maskVector.size());
    return PSI_PIR_SUCCESS;
}

int32_t ProtocolEncoder::IsMatch(const Slots &plain, std::size_t offset, const ProtocolParams &params,
                                 bool &matched) const noexcept
{
    if (!ContainsSlotRange(plain, offset, params.slotsPerData)) {
        LOGE("match range out of bounds: offset=%{private}zu, slotsPerData=%{public}zu, "
                     "plainSize=%{public}zu",
                     offset, params.slotsPerData, plain.size());
        return PSI_PIR_CODEC_ERROR;
    }
    matched = std::all_of(plain.begin() + static_cast<std::ptrdiff_t>(offset),
                          plain.begin() + static_cast<std::ptrdiff_t>(offset + params.slotsPerData),
                          [&params](std::uint64_t value) { return value % params.plainModulus == 0; });
    return PSI_PIR_SUCCESS;
}

} // namespace psi_pir::internal
