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

#include "privacy_protocol.h"
#include "securec.h"

#include "openssl_crypto_provider.h"
#include "protocol_codec.h"
#include "protocol_encoder.h"
#include "protocol_mapper.h"
#include "protocol_params.h"
#include "provider_registry.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ::PrivacyComputation;
using namespace ::PrivacyComputation::internal;

constexpr PrivacyProtocol PRIVATE_SET_INTERSECTION{DataSetSize::SIZE_512, ProtocolType::PRIVATE_SET_INTERSECTION};
constexpr PrivacyProtocol PIR_PROTOCOL{DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL};
constexpr PrivacyProtocol PIR_128_PROTOCOL{DataSetSize::SIZE_128, ProtocolType::PIR_PROTOCOL};
constexpr std::size_t SLOT_COUNT = 512U * 4U;
constexpr std::uint64_t PLAIN_MODULUS = 65537;
constexpr size_t K_EXTENDED_COMPARE_BYTES = 16;
constexpr size_t K_EXTENDED_SLOTS_PER_DATA = K_EXTENDED_COMPARE_BYTES / sizeof(uint16_t);
constexpr size_t HEX_DIGITS_PER_BYTE = 2;
constexpr size_t BITS_PER_HEX_DIGIT = 4;
constexpr uint8_t INVALID_HEX_DIGIT = 0xff;
constexpr size_t SHA256_DIGEST_BYTES = 32;

Bytes SerializeSlots(const Slots &slots)
{
    Bytes output;
    output.reserve(slots.size() * sizeof(std::uint64_t));
    for (const auto value : slots) {
        for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
            output.push_back(static_cast<std::uint8_t>(value >> (byte * kBitsPerByte)));
        }
    }
    return output;
}

int32_t DeserializeSlots(const Bytes &bytes, Slots &output)
{
    if (bytes.size() % sizeof(std::uint64_t) != 0) {
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    Slots slots(bytes.size() / sizeof(std::uint64_t));
    for (std::size_t slot = 0; slot < slots.size(); ++slot) {
        for (std::size_t byte = 0; byte < sizeof(std::uint64_t); ++byte) {
            slots[slot] |= static_cast<std::uint64_t>(bytes[slot * sizeof(std::uint64_t) + byte])
                           << (byte * kBitsPerByte);
        }
    }
    output = std::move(slots);
    return PRIVACY_PROTOCOL_SUCCESS;
}

// Builds a deterministic target fixture for protocol evaluation tests.
// The TEE takes the comparison suffix as big-endian bytes and stores its least
// significant 16-bit word in the first slot, then repeats the group until all
// 2048 HE slots are filled.
Bytes MakePrivacyTargetFromSuffix(const Bytes &compareSuffix, std::size_t slotsPerData)
{
    if (slotsPerData == 0 || slotsPerData > compareSuffix.size() / sizeof(uint16_t)) {
        return {};
    }
    Slots group;
    group.reserve(slotsPerData);
    for (std::size_t slot = 0; slot < slotsPerData; ++slot) {
        const auto end = compareSuffix.size() - slot * sizeof(uint16_t);
        group.push_back((static_cast<std::uint64_t>(compareSuffix[end - sizeof(uint16_t)]) << kBitsPerByte) |
                        compareSuffix[end - 1]);
    }
    Slots query;
    query.reserve(SLOT_COUNT);
    for (std::size_t item = 0; item < SLOT_COUNT / slotsPerData; ++item) {
        query.insert(query.end(), group.begin(), group.end());
    }
    return SerializeSlots(query);
}

Bytes CompareSuffix(std::uint64_t value, std::size_t compareBytes)
{
    Bytes output(compareBytes, 0);
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        output[output.size() - byte - 1] = static_cast<std::uint8_t>(value >> (byte * kBitsPerByte));
    }
    return output;
}

Bytes HexBytes(const std::string &hex)
{
    if (hex.size() % HEX_DIGITS_PER_BYTE != 0) {
        return {};
    }
    const auto nibble = [](char value) -> std::uint8_t {
        if (value >= '0' && value <= '9') {
            return static_cast<std::uint8_t>(value - '0');
        }
        if (value >= 'a' && value <= 'f') {
            return static_cast<std::uint8_t>(value - 'a' + 10);
        }
        return INVALID_HEX_DIGIT;
    };
    Bytes output;
    output.reserve(hex.size() / HEX_DIGITS_PER_BYTE);
    for (std::size_t index = 0; index < hex.size(); index += HEX_DIGITS_PER_BYTE) {
        const auto high = nibble(hex[index]);
        const auto low = nibble(hex[index + 1]);
        if (high > 0x0f || low > 0x0f) {
            return {};
        }
        output.push_back(static_cast<std::uint8_t>((high << BITS_PER_HEX_DIGIT) | low));
    }
    return output;
}

int32_t BuiltInDigest(HashAlg algorithm, const Bytes &input, Bytes &output)
{
    const auto provider = GetOpenSslHashProvider();
    return provider ? provider->Digest(algorithm, input, output) : PRIVACY_PROTOCOL_HASH_ERROR;
}

int32_t BuiltInFillRandom(Bytes &output)
{
    const auto provider = GetOpenSslRandomSource();
    return provider ? provider->Fill(output) : PRIVACY_PROTOCOL_RANDOM_ERROR;
}

int32_t BuiltInEncryptOrderValue(const Bytes &seed, const std::optional<Bytes> &value, const ProtocolParams &params,
                                 Bytes &output)
{
    const auto provider = GetOpenSslPirValueCrypto();
    return provider ? provider->Encrypt(seed, value, params, output) : PRIVACY_PROTOCOL_CRYPTO_ERROR;
}

int32_t BuiltInDecryptOrderValue(const Bytes &seed, const Bytes &ciphertext, const ProtocolParams &params,
                                 std::optional<Bytes> &output)
{
    const auto provider = GetOpenSslPirValueCrypto();
    return provider ? provider->Decrypt(seed, ciphertext, params, output) : PRIVACY_PROTOCOL_CRYPTO_ERROR;
}

Bytes MakePrivacyTargetForKey(const Bytes &key, std::size_t slotsPerData)
{
    Bytes digest;
    EXPECT_EQ(BuiltInDigest(HashAlg::SHA256, key, digest), PRIVACY_PROTOCOL_SUCCESS);
    if (digest.size() < slotsPerData * sizeof(uint16_t)) {
        return {};
    }
    return MakePrivacyTargetFromSuffix(
        Bytes(digest.end() - static_cast<std::ptrdiff_t>(slotsPerData * sizeof(uint16_t)), digest.end()), slotsPerData);
}

Bytes MakePrivacyTarget(std::uint64_t compareValue)
{
    return MakePrivacyTargetForKey(CompareSuffix(compareValue, kDefaultCompareBits / kBitsPerByte),
                                   kDefaultSlotsPerData);
}

Bytes Make128BitPrivacyTarget(std::uint64_t compareValue)
{
    return MakePrivacyTargetForKey(CompareSuffix(compareValue, K_EXTENDED_COMPARE_BYTES), K_EXTENDED_SLOTS_PER_DATA);
}

Bytes MakeZeroPrivacyTarget()
{
    return MakePrivacyTargetFromSuffix(Bytes(kDefaultCompareBits / kBitsPerByte, 0), kDefaultSlotsPerData);
}

// Simulates raw HE decryption for internal post-processing tests, using the
// same PrivacySearchResult DTO before and after this step: before decryption
// resultCipherText contains HE ciphertexts; afterwards each entry contains one
// serialized 2048-slot HE plaintext.
PrivacySearchResult DecryptOutsideProtocol(const PrivacySearchResult &response)
{
    PrivacySearchResult decrypted;
    decrypted.valueCipherText = response.valueCipherText;
    for (const auto &batch : response.resultCipherText) {
        Slots slots;
        const auto ret = DeserializeSlots(batch, slots);
        EXPECT_EQ(ret, PRIVACY_PROTOCOL_SUCCESS);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return {};
        }
        EXPECT_EQ(slots.size(), SLOT_COUNT);
        if (slots.size() != SLOT_COUNT) {
            return {};
        }
        decrypted.resultCipherText.push_back(SerializeSlots(slots));
    }
    return decrypted;
}

class RecordingHash final : public IHashProvider {
public:
    int32_t Digest(HashAlg algorithm, const Bytes &input, Bytes &output) const noexcept override
    {
        algorithms.push_back(algorithm);
        inputs.push_back(input);
        Bytes digest(SHA256_DIGEST_BYTES, 0);
        const auto copySize = std::min(digest.size(), input.size());
        if (copySize != 0 && memcpy_s(digest.data() + digest.size() - copySize, copySize,
                                      input.data() + input.size() - copySize, copySize) != EOK) {
            return PRIVACY_PROTOCOL_HASH_ERROR;
        }
        output = std::move(digest);
        return PRIVACY_PROTOCOL_SUCCESS;
    }

    mutable std::vector<HashAlg> algorithms;
    mutable std::vector<Bytes> inputs;
};

class RecordingRandom final : public IRandomSource {
public:
    int32_t Fill(Bytes &output) noexcept override
    {
        requestedSizes.push_back(output.size());
        const auto value = nextValue_++;
        std::fill(output.begin(), output.end(), value);
        // Supply distinct entropy to adjacent PIR groups within the same random fill.
        if (output.size() == SLOT_COUNT * 2U) {
            constexpr size_t groupBytes = 8;
            for (size_t index = 0; index < output.size(); ++index) {
                output[index] = static_cast<uint8_t>(value + (index / groupBytes) % 2U);
            }
        }
        return PRIVACY_PROTOCOL_SUCCESS;
    }

    std::vector<std::size_t> requestedSizes;

private:
    std::uint8_t nextValue_{0x31};
};

class BoundaryRandom final : public IRandomSource {
public:
    int32_t Fill(Bytes &output) noexcept override
    {
        requestedSizes.push_back(output.size());
        std::fill(output.begin(), output.end(), 0x12);
        constexpr std::array<uint8_t, sizeof(uint32_t)> boundaryCandidates{0, 0, UINT8_MAX, UINT8_MAX};
        if (output.size() >= boundaryCandidates.size()) {
            for (size_t i = 0; i < boundaryCandidates.size(); ++i) {
                output[i] = boundaryCandidates[i];
            }
        }
        return PRIVACY_PROTOCOL_SUCCESS;
    }

    std::vector<std::size_t> requestedSizes;
};

class RecordingPirValueCrypto final : public IPirValueCrypto {
public:
    int32_t Encrypt(const Bytes &seed, const std::optional<Bytes> &value, const ProtocolParams &params,
                    Bytes &output) const noexcept override
    {
        encryptParams.push_back(params);
        encryptSeeds.push_back(seed);
        Bytes encrypted{static_cast<std::uint8_t>(seed.size())};
        encrypted.insert(encrypted.end(), seed.begin(), seed.end());
        encrypted.push_back(value.has_value() ? 1 : 0);
        if (value.has_value()) {
            encrypted.push_back(static_cast<std::uint8_t>(value->size()));
            encrypted.insert(encrypted.end(), value->begin(), value->end());
        }
        output = std::move(encrypted);
        return PRIVACY_PROTOCOL_SUCCESS;
    }

    int32_t Decrypt(const Bytes &seed, const Bytes &ciphertext, const ProtocolParams &params,
                    std::optional<Bytes> &output) const noexcept override
    {
        decryptParams.push_back(params);
        if (ciphertext.empty()) {
            return PRIVACY_PROTOCOL_CRYPTO_ERROR;
        }
        std::size_t offset = 0;
        const auto seedSize = ciphertext[offset++];
        if (seed.size() != seedSize || offset + seedSize > ciphertext.size() ||
            !std::equal(seed.begin(), seed.end(), ciphertext.begin() + static_cast<std::ptrdiff_t>(offset))) {
            return PRIVACY_PROTOCOL_CRYPTO_ERROR;
        }
        offset += seedSize;
        if (offset >= ciphertext.size()) {
            return PRIVACY_PROTOCOL_CRYPTO_ERROR;
        }
        if (ciphertext[offset++] == 0) {
            if (offset != ciphertext.size()) {
                return PRIVACY_PROTOCOL_CRYPTO_ERROR;
            }
            output = std::nullopt;
            return PRIVACY_PROTOCOL_SUCCESS;
        }
        if (offset >= ciphertext.size()) {
            return PRIVACY_PROTOCOL_CRYPTO_ERROR;
        }
        const auto size = ciphertext[offset++];
        if (offset + size != ciphertext.size()) {
            return PRIVACY_PROTOCOL_CRYPTO_ERROR;
        }
        output = Bytes(ciphertext.begin() + static_cast<std::ptrdiff_t>(offset), ciphertext.end());
        return PRIVACY_PROTOCOL_SUCCESS;
    }

    mutable std::vector<ProtocolParams> encryptParams;
    mutable std::vector<ProtocolParams> decryptParams;
    mutable std::vector<Bytes> encryptSeeds;
};

class ScopedProviderReset final {
public:
    ScopedProviderReset()
    {
        Reset();
    }

    ~ScopedProviderReset()
    {
        Reset();
    }

private:
    static void Reset()
    {
        SetHashProvider(nullptr);
        SetRandomSource(nullptr);
        SetHEEngine(nullptr);
        SetPirValueCrypto(nullptr);
    }
};

// Test-only handle keeps arithmetic in Slots; only import and final export use Bytes.
class CleartextEvaluation final : public HeEvaluation {
public:
    explicit CleartextEvaluation(Slots slots) : slots_(std::move(slots)) {}
    int32_t AddPlain(const Slots &plaintext) noexcept override;
    int32_t MultiplyPlain(const Slots &plaintext) noexcept override;
    int32_t Rerandomize() noexcept override
    {
        return status_; // Cleartext fixtures need no cryptographic rerandomization.
    }
    int32_t Serialize(Bytes &output) const noexcept override
    {
        if (status_ == PRIVACY_PROTOCOL_SUCCESS) {
            output = SerializeSlots(slots_);
        }
        return status_;
    }

private:
    Slots slots_;
    int32_t status_ = PRIVACY_PROTOCOL_SUCCESS;
};

int32_t CleartextEvaluation::AddPlain(const Slots &plaintext) noexcept
{
    if (status_ != PRIVACY_PROTOCOL_SUCCESS) {
        return status_;
    }
    if (slots_.size() != plaintext.size()) {
        status_ = PRIVACY_PROTOCOL_HE_ERROR;
        return status_;
    }
    for (size_t i = 0; i < slots_.size(); ++i) {
        slots_[i] = (plaintext[i] % PLAIN_MODULUS + PLAIN_MODULUS - slots_[i]) % PLAIN_MODULUS;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t CleartextEvaluation::MultiplyPlain(const Slots &plaintext) noexcept
{
    if (status_ != PRIVACY_PROTOCOL_SUCCESS) {
        return status_;
    }
    if (slots_.size() != plaintext.size()) {
        status_ = PRIVACY_PROTOCOL_HE_ERROR;
        return status_;
    }
    for (size_t i = 0; i < slots_.size(); ++i) {
        slots_[i] = (slots_[i] * plaintext[i]) % PLAIN_MODULUS;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

class CleartextHe final : public IHEEngine {
public:
    bool IsCompatible(const ProtocolParams &params) const noexcept override
    {
        compatibleParams.push_back(params);
        return compatible;
    }
    int32_t CreateEvaluation(const Bytes &ciphertext,
                             std::unique_ptr<HeEvaluation> &evaluation) const noexcept override;
    bool compatible{true};
    bool emptyEvaluation{false};
    mutable std::vector<ProtocolParams> compatibleParams;
};

int32_t CleartextHe::CreateEvaluation(const Bytes &ciphertext, std::unique_ptr<HeEvaluation> &evaluation) const noexcept
{
    if (ciphertext.empty()) {
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    Slots slots;
    if (DeserializeSlots(ciphertext, slots) != PRIVACY_PROTOCOL_SUCCESS) {
        return PRIVACY_PROTOCOL_HE_ERROR;
    }
    if (emptyEvaluation) {
        return PRIVACY_PROTOCOL_SUCCESS;
    }
    evaluation = std::make_unique<CleartextEvaluation>(std::move(slots));
    return PRIVACY_PROTOCOL_SUCCESS;
}

struct RecordingProviders {
    std::shared_ptr<RecordingHash> hash;
    std::shared_ptr<RecordingRandom> randomSource;
    std::shared_ptr<RecordingPirValueCrypto> pirValue;
    std::shared_ptr<CleartextHe> he;
};

RecordingProviders InstallRecordingProviders()
{
    RecordingProviders providers{
        std::make_shared<RecordingHash>(),
        std::make_shared<RecordingRandom>(),
        std::make_shared<RecordingPirValueCrypto>(),
        std::make_shared<CleartextHe>(),
    };
    SetHashProvider(providers.hash);
    SetRandomSource(providers.randomSource);
    SetPirValueCrypto(providers.pirValue);
    SetHEEngine(providers.he);
    return providers;
}

std::shared_ptr<CleartextHe> ConfigureProtocol()
{
    SetHashProvider(nullptr);
    SetRandomSource(nullptr);
    SetPirValueCrypto(nullptr);
    auto he = std::make_shared<CleartextHe>();
    SetHEEngine(he);
    return he;
}

Bytes EightByteKey(std::uint64_t value)
{
    Bytes output(sizeof(value));
    for (std::size_t byte = 0; byte < output.size(); ++byte) {
        output[output.size() - byte - 1] = static_cast<std::uint8_t>(value >> (byte * kBitsPerByte));
    }
    return output;
}

Element MakeElement(std::uint64_t key, const std::optional<std::string> &value = std::nullopt)
{
    Element element;
    element.elemKey = EightByteKey(key);
    element.hashAlg = HashAlg::SHA256;
    if (value.has_value()) {
        element.elemValue = Bytes(value->begin(), value->end());
    }
    return element;
}

Element Make128BitElement(std::uint64_t key, const std::optional<std::string> &value = std::nullopt)
{
    Element element;
    element.elemKey = CompareSuffix(key, K_EXTENDED_COMPARE_BYTES);
    element.hashAlg = HashAlg::SHA256;
    if (value.has_value()) {
        element.elemValue = Bytes(value->begin(), value->end());
    }
    return element;
}

void ExpectCoreParams(const ProtocolParams &params, ProtocolMode mode, std::size_t compareBits = 64,
                      std::size_t slotsPerData = 4, std::size_t dataNumPerPlaintext = 512)
{
    EXPECT_EQ(params.mode, mode);
    EXPECT_EQ(params.compareBits, compareBits);
    EXPECT_EQ(params.bitsPerSlot, 16U);
    EXPECT_EQ(params.slotsPerData, slotsPerData);
    EXPECT_EQ(params.dataNumPerPlaintext, dataNumPerPlaintext);
    EXPECT_EQ(params.plainModulus, PLAIN_MODULUS);
    EXPECT_EQ(params.polyModulusDegree, 2048U);
    EXPECT_EQ(params.fheAlgorithm, FheAlgorithm::BGV2048_48);
}

TEST(PrivacyProtocolTargetTest, RepeatsComparisonGroupAcrossAllSlots)
{
    const Element element{Bytes{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, HashAlg::NONE, std::nullopt};
    const Slots expected{0x0f10, 0x0d0e, 0x0b0c, 0x090a, 0x0708, 0x0506, 0x0304, 0x0102};
    for (const auto &protocol : {PRIVATE_SET_INTERSECTION, PIR_PROTOCOL, PIR_128_PROTOCOL}) {
        ProtocolParams params;
        ASSERT_EQ(MapPrivacyProtocol(protocol, params), PRIVACY_PROTOCOL_SUCCESS);
        Slots target;
        ASSERT_EQ(ProtocolEncoder().EncodeTargetVector(element, params, target), PRIVACY_PROTOCOL_SUCCESS);
        ASSERT_EQ(target.size(), 2048U);
        EXPECT_EQ(target.size() / params.slotsPerData, params.slotsPerData == 4 ? 512U : 256U);
        for (std::size_t slot = 0; slot < target.size(); ++slot) {
            EXPECT_EQ(target[slot], expected[slot % params.slotsPerData]) << "slot=" << slot;
        }
    }
}

TEST(PrivacyProtocolTargetTest, InvalidTargetPackingPreservesOutput)
{
    ProtocolEncoder encoder;
    const Element element{Bytes{1, 2, 3, 4, 5, 6, 7, 8}, HashAlg::NONE, std::nullopt};
    Slots target{99};
    ProtocolParams params;
    params.polyModulusDegree = 2047;
    EXPECT_EQ(encoder.EncodeTargetVector(element, params, target), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    params = ProtocolParams{};
    params.slotsPerData = 0;
    EXPECT_EQ(encoder.EncodeTargetVector(element, params, target), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    params = ProtocolParams{};
    params.dataNumPerPlaintext = 256;
    EXPECT_EQ(encoder.EncodeTargetVector(element, params, target), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    params = ProtocolParams{};
    const Element zero{Bytes(8, 0), HashAlg::NONE, std::nullopt};
    EXPECT_EQ(encoder.EncodeTargetVector(zero, params, target), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    const Element shortInput{Bytes{1}, HashAlg::NONE, std::nullopt};
    EXPECT_EQ(encoder.EncodeTargetVector(shortInput, params, target), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
}

TEST(PrivacyProtocolHeContractTest, AddPlainMeansPlaintextMinusCiphertext)
{
    CleartextHe he;
    const Slots ciphertextSlots{1, 0, PLAIN_MODULUS - 1};
    const Slots plaintext{2, 1, PLAIN_MODULUS - 1};
    Bytes output;
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(he.CreateEvaluation(SerializeSlots(ciphertextSlots), evaluation), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(evaluation->AddPlain(plaintext), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(evaluation->Serialize(output), PRIVACY_PROTOCOL_SUCCESS);

    Slots result;
    ASSERT_EQ(DeserializeSlots(output, result), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(result, Slots({1, 1, 0}));
}

TEST(PrivacyProtocolOpenSslTest, HashAlgorithmsMatchKnownAnswerVectors)
{
    const Bytes input{'a', 'b', 'c'};
    const std::array<std::pair<HashAlg, std::string>, 3> vectors{{
        {HashAlg::SHA256, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {HashAlg::SHA512, "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                          "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"},
        {HashAlg::SHA384,
         "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7"},
    }};
    for (const auto &[algorithm, expectedHex] : vectors) {
        Bytes digest{0xff};
        ASSERT_EQ(BuiltInDigest(algorithm, input, digest), PRIVACY_PROTOCOL_SUCCESS);
        EXPECT_EQ(digest, HexBytes(expectedHex));
    }

    Bytes unchanged{1, 2, 3};
    EXPECT_NE(BuiltInDigest(static_cast<HashAlg>(99), input, unchanged), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(unchanged, Bytes({1, 2, 3}));
}

TEST(PrivacyProtocolOpenSslTest, Sha256SupportsEmptyInput)
{
    Bytes digest;
    ASSERT_EQ(BuiltInDigest(HashAlg::SHA256, {}, digest), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(digest, HexBytes("e3b0c44298fc1c149afbf4c8996fb924"
                               "27ae41e4649b934ca495991b7852b855"));
}

TEST(PrivacyProtocolOpenSslTest, RandomGenerationFillsRequestedBuffer)
{
    Bytes first(32, 0);
    Bytes second(32, 0);
    ASSERT_EQ(BuiltInFillRandom(first), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(BuiltInFillRandom(second), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_NE(first, Bytes(32, 0));
    EXPECT_NE(second, Bytes(32, 0));
    EXPECT_NE(first, second);

    Bytes empty;
    EXPECT_NE(BuiltInFillRandom(empty), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(empty.empty());
}

TEST(PrivacyProtocolOpenSslTest, AesGcmRoundTripsWithBothSupportedSeedSizes)
{
    const std::array<std::pair<PrivacyProtocol, Bytes>, 2> cases{{
        {PIR_PROTOCOL, Bytes{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}},
        {PIR_128_PROTOCOL,
         Bytes{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}},
    }};
    for (const auto &[protocol, seed] : cases) {
        ProtocolParams params;
        ASSERT_EQ(MapPrivacyProtocol(protocol, params), PRIVACY_PROTOCOL_SUCCESS);
        const std::optional<Bytes> value{Bytes{'v', 'a', 'l', 'u', 'e'}};
        Bytes ciphertext;
        ASSERT_EQ(BuiltInEncryptOrderValue(seed, value, params, ciphertext), PRIVACY_PROTOCOL_SUCCESS);
        EXPECT_FALSE(ciphertext.empty());

        std::optional<Bytes> plaintext;
        ASSERT_EQ(BuiltInDecryptOrderValue(seed, ciphertext, params, plaintext), PRIVACY_PROTOCOL_SUCCESS);
        EXPECT_EQ(plaintext, value);
    }
}

TEST(PrivacyProtocolOpenSslTest, AesGcmPreservesAbsentAndPresentEmptyValues)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

    Bytes absentCiphertext;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, std::nullopt, params, absentCiphertext), PRIVACY_PROTOCOL_SUCCESS);
    std::optional<Bytes> absentOutput{Bytes{9}};
    ASSERT_EQ(BuiltInDecryptOrderValue(seed, absentCiphertext, params, absentOutput), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(absentOutput.has_value());

    Bytes emptyCiphertext;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, Bytes{}, params, emptyCiphertext), PRIVACY_PROTOCOL_SUCCESS);
    std::optional<Bytes> emptyOutput;
    ASSERT_EQ(BuiltInDecryptOrderValue(seed, emptyCiphertext, params, emptyOutput), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(emptyOutput.has_value());
    EXPECT_TRUE(emptyOutput->empty());
}

TEST(PrivacyProtocolOpenSslTest, AesGcmRejectsTamperingAndWrongSeedWithoutChangingOutput)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    Bytes ciphertext;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, Bytes{'s', 'e', 'c', 'r', 'e', 't'}, params, ciphertext),
              PRIVACY_PROTOCOL_SUCCESS);

    const std::array<std::size_t, 4> tamperOffsets{4, 16, 28, ciphertext.size() - 1};
    for (const auto offset : tamperOffsets) {
        auto tampered = ciphertext;
        tampered[offset] ^= 0x01;
        std::optional<Bytes> output{Bytes{7, 8, 9}};
        EXPECT_NE(BuiltInDecryptOrderValue(seed, tampered, params, output), PRIVACY_PROTOCOL_SUCCESS);
        EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));
    }

    auto wrongSeed = seed;
    wrongSeed.front() ^= 0x01;
    std::optional<Bytes> output{Bytes{7, 8, 9}};
    EXPECT_NE(BuiltInDecryptOrderValue(wrongSeed, ciphertext, params, output), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));

    auto truncated = ciphertext;
    truncated.pop_back();
    EXPECT_NE(BuiltInDecryptOrderValue(seed, truncated, params, output), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));

    auto extended = ciphertext;
    extended.push_back(0);
    EXPECT_NE(BuiltInDecryptOrderValue(seed, extended, params, output), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));
}

TEST(PrivacyProtocolOpenSslTest, AesGcmUsesFreshNonceAndAuthenticatedFrameHeader)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    const std::optional<Bytes> value{Bytes{'v'}};
    Bytes first;
    Bytes second;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, value, params, first), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, value, params, second), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_GE(first.size(), 16U);
    EXPECT_EQ(Bytes(first.begin(), first.begin() + 4), Bytes({'P', 'I', 'R', '1'}));
    EXPECT_EQ(first[4], 1U);
    EXPECT_EQ(first[5], 1U);
    EXPECT_EQ(first[6], 12U);
    EXPECT_EQ(first[7], 16U);
    EXPECT_EQ(first[8], 1U);
    EXPECT_EQ(first[9], 0U);
    EXPECT_EQ(first[10], 0U);
    EXPECT_EQ(first[11], 64U);
    EXPECT_NE(first, second);
}

TEST(PrivacyProtocolOpenSslTest, AesGcmDecryptsFixedInteroperabilityVector)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed = HexBytes("0123456789abcdef");
    const Bytes frame = HexBytes("5049523101010c100100004000000006"
                                 "000102030405060708090a0b"
                                 "66ebe4ec5a3c"
                                 "9549d1d36f92a3c89ae969621786be62");
    std::optional<Bytes> value;
    ASSERT_EQ(BuiltInDecryptOrderValue(seed, frame, params, value), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, Bytes({'v', 'a', 'l', 'u', 'e'}));
}

TEST(PrivacyProtocolOpenSslTest, AesGcmRejectsProtocolMismatchWithoutChangingOutput)
{
    ProtocolParams params64;
    ProtocolParams params128;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params64), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, params128), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed64 = HexBytes("0123456789abcdef");
    Bytes frame;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed64, Bytes{'v'}, params64, frame), PRIVACY_PROTOCOL_SUCCESS);

    std::optional<Bytes> output{Bytes{7}};
    EXPECT_EQ(BuiltInDecryptOrderValue(seed64, frame, params128, output), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7}));

    const Bytes seed128(16, 1);
    EXPECT_EQ(BuiltInDecryptOrderValue(seed128, frame, params128, output), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7}));
}

TEST(PrivacyProtocolProviderTest, CustomProvidersAreSelected)
{
    ScopedProviderReset resetProviders;
    const auto providers = InstallRecordingProviders();

    const auto selectedHash = GetHashProvider();
    const auto selectedRandom = GetRandomSource();
    const auto selectedPirValue = GetPirValueCrypto();
    EXPECT_EQ(selectedHash.get(), providers.hash.get());
    EXPECT_EQ(selectedRandom.get(), providers.randomSource.get());
    EXPECT_EQ(selectedPirValue.get(), providers.pirValue.get());
    EXPECT_EQ(GetHEEngine().get(), providers.he.get());
}

TEST(PrivacyProtocolProviderTest, CustomProvidersDriveEvaluationAndResolution)
{
    ScopedProviderReset resetProviders;
    const auto providers = InstallRecordingProviders();

    const auto target = MakePrivacyTargetFromSuffix(CompareSuffix(2, 8), 4);
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(target, {MakeElement(1, "one"), MakeElement(2, "two")}, PIR_PROTOCOL, response),
              PRIVACY_PROTOCOL_SUCCESS);

    ASSERT_EQ(response.resultCipherText.size(), 1U);
    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "two");

    EXPECT_EQ(providers.hash->algorithms, std::vector<HashAlg>({HashAlg::SHA256, HashAlg::SHA256}));
    EXPECT_EQ(providers.hash->inputs, std::vector<Bytes>({EightByteKey(1), EightByteKey(2)}));
    EXPECT_EQ(providers.randomSource->requestedSizes, std::vector<std::size_t>({8U, 8U, SLOT_COUNT * 2U}));
    ASSERT_EQ(providers.pirValue->encryptParams.size(), 2U);
    ASSERT_EQ(providers.pirValue->decryptParams.size(), 1U);
    ExpectCoreParams(providers.pirValue->encryptParams.back(), ProtocolMode::PIR);
    ExpectCoreParams(providers.pirValue->decryptParams.back(), ProtocolMode::PIR);
}

TEST(PrivacyProtocolProviderTest, GetSearchResultReturnsAllPirMatchesAcrossBatchesInOrder)
{
    ScopedProviderReset resetProviders;
    auto pirValueCrypto = std::make_shared<RecordingPirValueCrypto>();
    SetPirValueCrypto(pirValueCrypto);
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolEncoder encoder;

    const Bytes firstSeed{1, 2, 3, 4, 5, 6, 7, 8};
    const Bytes lastSeed{8, 7, 6, 5, 4, 3, 2, 1};
    Bytes firstCiphertext;
    Bytes lastCiphertext;
    ASSERT_EQ(pirValueCrypto->Encrypt(firstSeed, Bytes{'f', 'i', 'r', 's', 't'}, params, firstCiphertext),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(pirValueCrypto->Encrypt(lastSeed, Bytes{'l', 'a', 's', 't'}, params, lastCiphertext),
              PRIVACY_PROTOCOL_SUCCESS);

    Slots firstBatch(SLOT_COUNT, 1);
    Slots lastBatch(SLOT_COUNT, 1);
    std::fill_n(firstBatch.begin(), params.slotsPerData, 0);
    std::fill_n(lastBatch.begin(), params.slotsPerData, 0);
    Slots firstSeedSlots;
    Slots lastSeedSlots;
    ASSERT_EQ(encoder.EncodeSeed(firstSeed, params, firstSeedSlots), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(encoder.EncodeSeed(lastSeed, params, lastSeedSlots), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(memcpy_s(firstBatch.data() + params.slotsPerData,
                       (firstBatch.size() - params.slotsPerData) * sizeof(uint64_t), firstSeedSlots.data(),
                       firstSeedSlots.size() * sizeof(uint64_t)),
              EOK);
    ASSERT_EQ(memcpy_s(lastBatch.data() + params.slotsPerData,
                       (lastBatch.size() - params.slotsPerData) * sizeof(uint64_t), lastSeedSlots.data(),
                       lastSeedSlots.size() * sizeof(uint64_t)),
              EOK);

    PrivacySearchResult input;
    input.resultCipherText = {SerializeSlots(firstBatch), SerializeSlots(lastBatch)};
    input.valueCipherText.emplace(257, Bytes{1});
    input.valueCipherText->front() = firstCiphertext;
    input.valueCipherText->back() = lastCiphertext;
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(input.resultCipherText, input.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 2U);
    EXPECT_EQ(result.attachedValues->at(0), Bytes({'f', 'i', 'r', 's', 't'}));
    EXPECT_EQ(result.attachedValues->at(1), Bytes({'l', 'a', 's', 't'}));
    EXPECT_EQ(pirValueCrypto->decryptParams.size(), 2U);
}

TEST(PrivacyProtocolProviderTest, IndependentPirMasksHideNonMatchingSeed)
{
    ScopedProviderReset resetProviders;
    const auto providers = InstallRecordingProviders();
    const auto target = MakePrivacyTargetFromSuffix(CompareSuffix(2, 8), 4);
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(target, {MakeElement(1, "one"), MakeElement(2, "two")}, PIR_PROTOCOL, response),
              PRIVACY_PROTOCOL_SUCCESS);

    Slots evaluatedSlots;
    ASSERT_EQ(DeserializeSlots(response.resultCipherText.front(), evaluatedSlots), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(evaluatedSlots.size(), SLOT_COUNT);
    ASSERT_EQ(providers.pirValue->encryptSeeds.size(), 2U);

    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, mapped), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolEncoder encoder;

    Slots nonMatchSeedSlots;
    ASSERT_EQ(encoder.EncodeSeed(providers.pirValue->encryptSeeds[0], mapped, nonMatchSeedSlots),
              PRIVACY_PROTOCOL_SUCCESS);
    const auto &expectedSeedSlots = nonMatchSeedSlots;
    Slots exposedDifference(mapped.slotsPerData, 0);
    for (std::size_t slot = 0; slot < mapped.slotsPerData; ++slot) {
        exposedDifference[slot] =
            (evaluatedSlots[mapped.slotsPerData + slot] + mapped.plainModulus - evaluatedSlots[slot]) %
            mapped.plainModulus;
    }
    EXPECT_NE(exposedDifference, expectedSeedSlots);

    const auto matchedFirstOffset = 2 * mapped.slotsPerData;
    const auto matchedSecondOffset = matchedFirstOffset + mapped.slotsPerData;
    EXPECT_TRUE(std::all_of(evaluatedSlots.begin() + static_cast<std::ptrdiff_t>(matchedFirstOffset),
                            evaluatedSlots.begin() + static_cast<std::ptrdiff_t>(matchedSecondOffset),
                            [](std::uint64_t slot) { return slot == 0; }));
    Slots matchedSeedSlots;
    ASSERT_EQ(encoder.EncodeSeed(providers.pirValue->encryptSeeds[1], mapped, matchedSeedSlots),
              PRIVACY_PROTOCOL_SUCCESS);
    const auto &expectedMatchedSecond = matchedSeedSlots;
    EXPECT_TRUE(std::equal(expectedMatchedSecond.begin(), expectedMatchedSecond.end(),
                           evaluatedSlots.begin() + static_cast<std::ptrdiff_t>(matchedSecondOffset)));
}

TEST(PrivacyProtocolProviderTest, NullOverridesRestoreOpenSslDefaults)
{
    ScopedProviderReset resetProviders;
    InstallRecordingProviders();
    SetHashProvider(nullptr);
    SetRandomSource(nullptr);
    SetPirValueCrypto(nullptr);
    const auto restoredHash = GetHashProvider();
    const auto restoredRandom = GetRandomSource();
    const auto restoredPirValue = GetPirValueCrypto();
    EXPECT_EQ(restoredHash.get(), GetOpenSslHashProvider().get());
    EXPECT_EQ(restoredRandom.get(), GetOpenSslRandomSource().get());
    EXPECT_EQ(restoredPirValue.get(), GetOpenSslPirValueCrypto().get());

    Bytes digest;
    ASSERT_EQ(restoredHash->Digest(HashAlg::SHA256, Bytes{'a', 'b', 'c'}, digest), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(digest, HexBytes("ba7816bf8f01cfea414140de5dae2223"
                               "b00361a396177a9cb410ff61f20015ad"));
}

TEST(PrivacyProtocolProviderTest, PirMaskVectorCoversAllSlotsAndSeparatesPairedGroups)
{
    ScopedProviderReset resetProviders;
    auto randomSource = std::make_shared<RecordingRandom>();
    SetRandomSource(randomSource);

    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, mapped), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolEncoder encoder;
    Slots maskVector;
    ASSERT_EQ(encoder.GenerateMaskVector(mapped, maskVector), PRIVACY_PROTOCOL_SUCCESS);

    ASSERT_EQ(maskVector.size(), SLOT_COUNT);
    ASSERT_EQ(randomSource->requestedSizes, std::vector<std::size_t>({SLOT_COUNT * 2U}));

    // Adjacent groups receive 0x31 and 0x32 in a single fill.
    // Independently sampled groups may coincide; no collision retry is required.
    constexpr std::uint64_t firstGroupMask = 0x3131U + 1U;
    constexpr std::uint64_t secondGroupMask = 0x3232U + 1U;
    for (std::size_t item = 0; item < mapped.dataNumPerPlaintext / 2; ++item) {
        const auto firstOffset = item * 2 * mapped.slotsPerData;
        const auto secondOffset = firstOffset + mapped.slotsPerData;
        for (std::size_t slot = 0; slot < mapped.slotsPerData; ++slot) {
            EXPECT_GE(maskVector[firstOffset + slot], 1U);
            EXPECT_LT(maskVector[firstOffset + slot], mapped.plainModulus);
            EXPECT_NE(maskVector[firstOffset + slot] % mapped.plainModulus, 0U);
            EXPECT_GE(maskVector[secondOffset + slot], 1U);
            EXPECT_LT(maskVector[secondOffset + slot], mapped.plainModulus);
            EXPECT_NE(maskVector[secondOffset + slot] % mapped.plainModulus, 0U);
            EXPECT_EQ(maskVector[firstOffset + slot], firstGroupMask);
            EXPECT_EQ(maskVector[secondOffset + slot], secondGroupMask);
            EXPECT_NE(maskVector[firstOffset + slot], maskVector[secondOffset + slot]);
        }
    }
}

TEST(PrivacyProtocolProviderTest, MaskVectorMapsBoundaryCandidatesToNonZeroFieldElements)
{
    ScopedProviderReset resetProviders;
    auto randomSource = std::make_shared<BoundaryRandom>();
    SetRandomSource(randomSource);

    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PRIVATE_SET_INTERSECTION, mapped), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolEncoder encoder;
    Slots maskVector;
    ASSERT_EQ(encoder.GenerateMaskVector(mapped, maskVector), PRIVACY_PROTOCOL_SUCCESS);

    ASSERT_EQ(maskVector.size(), SLOT_COUNT);
    ASSERT_EQ(randomSource->requestedSizes, std::vector<std::size_t>({SLOT_COUNT * 2U}));
    EXPECT_EQ(maskVector[0], 1U);     // 0x0000 + 1
    EXPECT_EQ(maskVector[1], 65536U); // 0xffff + 1
    EXPECT_TRUE(std::all_of(maskVector.begin(), maskVector.end(), [&mapped](std::uint64_t value) {
        return value >= 1 && value < mapped.plainModulus && value % mapped.plainModulus != 0;
    }));
}

TEST(PrivacyProtocolProviderTest, MaskVectorSupportsTheSmallestNonZeroField)
{
    ScopedProviderReset resetProviders;
    auto randomSource = std::make_shared<RecordingRandom>();
    SetRandomSource(randomSource);
    ProtocolParams params;
    params.plainModulus = 2;

    ProtocolEncoder encoder;
    Slots maskVector;
    ASSERT_EQ(encoder.GenerateMaskVector(params, maskVector), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(std::all_of(maskVector.begin(), maskVector.end(), [](std::uint64_t value) { return value == 1; }));
}

TEST(PrivacyProtocolTest, MapsTheThreeSupportedPrivacyProtocols)
{
    ProtocolParams intersection;
    ASSERT_EQ(MapPrivacyProtocol(PRIVATE_SET_INTERSECTION, intersection), PRIVACY_PROTOCOL_SUCCESS);
    ExpectCoreParams(intersection, ProtocolMode::INTERSECTION);
    EXPECT_EQ(intersection.symKeySize, 0U);
    EXPECT_EQ(intersection.symAlgorithm, SymAlgorithm::NONE);

    ProtocolParams pir;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, pir), PRIVACY_PROTOCOL_SUCCESS);
    ExpectCoreParams(pir, ProtocolMode::PIR);
    EXPECT_EQ(pir.symKeySize, 256U);
    EXPECT_EQ(pir.symAlgorithm, SymAlgorithm::AES_GCM);

    ProtocolParams pir128;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, pir128), PRIVACY_PROTOCOL_SUCCESS);
    ExpectCoreParams(pir128, ProtocolMode::PIR, 128, 8, 256);
    EXPECT_EQ(pir128.symKeySize, 256U);
    EXPECT_EQ(pir128.symAlgorithm, SymAlgorithm::AES_GCM);

    EXPECT_EQ(static_cast<std::uint32_t>(DataSetSize::SIZE_128), 0U);
    EXPECT_EQ(static_cast<std::uint32_t>(DataSetSize::SIZE_256), 1U);
    EXPECT_EQ(static_cast<std::uint32_t>(DataSetSize::SIZE_512), 2U);

    ProtocolParams unsupported;
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_256, ProtocolType::PRIVATE_SET_INTERSECTION}, unsupported),
              PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_512, ProtocolType::PIR_PROTOCOL}, unsupported),
              PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_128, ProtocolType::PRIVATE_SET_INTERSECTION}, unsupported),
              PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({static_cast<DataSetSize>(99), ProtocolType::PRIVATE_SET_INTERSECTION}, unsupported),
              PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_512, static_cast<ProtocolType>(99)}, unsupported),
              PRIVACY_PROTOCOL_INVALID_ARGUMENT);
}

TEST(PrivacyProtocolTest, FailedProtocolMappingDoesNotModifyOutput)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, params), PRIVACY_PROTOCOL_SUCCESS);
    const auto original = params;

    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_128, ProtocolType::PRIVATE_SET_INTERSECTION}, params),
              PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(params.mode, original.mode);
    EXPECT_EQ(params.compareBits, original.compareBits);
    EXPECT_EQ(params.bitsPerSlot, original.bitsPerSlot);
    EXPECT_EQ(params.slotsPerData, original.slotsPerData);
    EXPECT_EQ(params.dataNumPerPlaintext, original.dataNumPerPlaintext);
    EXPECT_EQ(params.plainModulus, original.plainModulus);
    EXPECT_EQ(params.polyModulusDegree, original.polyModulusDegree);
    EXPECT_EQ(params.fheAlgorithm, original.fheAlgorithm);
    EXPECT_EQ(params.symKeySize, original.symKeySize);
    EXPECT_EQ(params.symAlgorithm, original.symAlgorithm);
}

TEST(PrivacyProtocolTest, ProtocolValidationRejectsMismatchedAlgorithmParameters)
{
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, mapped), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolCodec codec;

    auto params = mapped;
    params.plainModulus = 65539;
    EXPECT_EQ(codec.ValidateProtocol(params), PRIVACY_PROTOCOL_INVALID_ARGUMENT);

    params = mapped;
    params.polyModulusDegree = 4096;
    EXPECT_EQ(codec.ValidateProtocol(params), PRIVACY_PROTOCOL_INVALID_ARGUMENT);

    params = mapped;
    params.fheAlgorithm = static_cast<FheAlgorithm>(99);
    EXPECT_EQ(codec.ValidateProtocol(params), PRIVACY_PROTOCOL_INVALID_ARGUMENT);

    params = mapped;
    params.symKeySize = 128;
    EXPECT_EQ(codec.ValidateProtocol(params), PRIVACY_PROTOCOL_INVALID_ARGUMENT);

    params = mapped;
    params.symAlgorithm = SymAlgorithm::NONE;
    EXPECT_EQ(codec.ValidateProtocol(params), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
}

TEST(PrivacyProtocolTest, BothPublicApisRejectUnsupportedPrivacyProtocols)
{
    const std::vector<PrivacyProtocol> unsupportedProtocols{
        {DataSetSize::SIZE_256, ProtocolType::PRIVATE_SET_INTERSECTION},
        {DataSetSize::SIZE_512, ProtocolType::PIR_PROTOCOL},
        {DataSetSize::SIZE_128, ProtocolType::PRIVATE_SET_INTERSECTION},
        {static_cast<DataSetSize>(99), ProtocolType::PRIVATE_SET_INTERSECTION},
        {DataSetSize::SIZE_512, static_cast<ProtocolType>(99)},
    };
    for (const auto &unsupported : unsupportedProtocols) {
        PrivacySearchResult response;
        SearchResult result;
        EXPECT_EQ(PrivacySearch({1}, {MakeElement(1)}, unsupported, response), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
        EXPECT_EQ(PostProcessSearchResult({}, std::nullopt, unsupported, result), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    }
}

TEST(PrivacyProtocolTest, InvalidElementHashIsRejectedBeforeHeEvaluation)
{
    SetHEEngine(nullptr);

    Element element = MakeElement(1);
    element.hashAlg = static_cast<HashAlg>(99);
    PrivacySearchResult response;
    EXPECT_EQ(PrivacySearch({1}, {element}, PRIVATE_SET_INTERSECTION, response), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
}

TEST(PrivacyProtocolTest, EmptyElementKeyKeepsTheExistingHashSemantics)
{
    ScopedProviderReset resetProviders;
    SetHEEngine(std::make_shared<CleartextHe>());

    Element element;
    element.elemKey = {};
    element.hashAlg = HashAlg::SHA256;
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTargetForKey({}, 4), {element}, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);

    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
}

TEST(PrivacyProtocolTest, EmptyEvaluationAndResolutionDoNotRequireHeEngine)
{
    SetHEEngine(nullptr);

    PrivacySearchResult intersectionResponse;
    EXPECT_EQ(PrivacySearch({1}, {}, PRIVATE_SET_INTERSECTION, intersectionResponse), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(intersectionResponse.resultCipherText.empty());
    EXPECT_FALSE(intersectionResponse.valueCipherText.has_value());
    SearchResult result;
    EXPECT_EQ(PostProcessSearchResult({}, std::nullopt, PRIVATE_SET_INTERSECTION, result), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(result.matchedResult);

    PrivacySearchResult pirResponse;
    EXPECT_EQ(PrivacySearch({1}, {}, PIR_PROTOCOL, pirResponse), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(pirResponse.resultCipherText.empty());
    ASSERT_TRUE(pirResponse.valueCipherText.has_value());
    EXPECT_TRUE(pirResponse.valueCipherText->empty());
    PrivacySearchResult emptyPir;
    emptyPir.valueCipherText.emplace();
    EXPECT_EQ(PostProcessSearchResult(emptyPir.resultCipherText, emptyPir.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
}

TEST(PrivacyProtocolTest, SingleLayerElementUsesExplicitSha256)
{
    ConfigureProtocol();
    Element element;
    element.elemKey = EightByteKey(2);
    element.hashAlg = HashAlg::SHA256;
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(2), {element}, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);
    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
}

TEST(PrivacyProtocolTest, ServerVectorUsesTeeSuffixAndLeastSignificantSlotFirst)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PRIVATE_SET_INTERSECTION, mapped), PRIVACY_PROTOCOL_SUCCESS);
    Element element;
    element.elemKey = {0xaa, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    element.hashAlg = HashAlg::SHA256;
    Slots encoded;
    ASSERT_EQ(encoder.EncodeServerVector(element, mapped, encoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(encoded, Slots({49489, 33421, 53375, 60560}));
}

TEST(PrivacyProtocolTest, OneHundredTwentyEightBitServerVectorUsesTeeSuffixAndLsbSlots)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, mapped), PRIVACY_PROTOCOL_SUCCESS);
    Element element;
    element.elemKey = {0xaa, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                       0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    element.hashAlg = HashAlg::SHA256;
    Slots encoded;
    ASSERT_EQ(encoder.EncodeServerVector(element, mapped, encoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(encoded, Slots({9739, 63044, 39120, 38765, 38004, 26788, 25817, 29794}));
}

TEST(PrivacyProtocolTest, PsiMatchesAndMissesAfterExternalHeDecryption)
{
    ConfigureProtocol();
    const std::vector<Element> elements{MakeElement(1, "ignored"), MakeElement(2, "also ignored"), MakeElement(3)};

    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(2), elements, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 1U);
    EXPECT_EQ(response.resultCipherText.front().size(), SLOT_COUNT * sizeof(std::uint64_t));
    EXPECT_FALSE(response.valueCipherText.has_value());

    auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());

    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(9), elements, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);
    decrypted = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PrivacyProtocolTest, PirReturnsOneAttachedValue)
{
    ConfigureProtocol();
    const std::vector<Element> elements{MakeElement(1, "one"), MakeElement(2, "two"), MakeElement(3, "three")};
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(2), elements, PIR_PROTOCOL, response), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 1U);
    EXPECT_EQ(response.resultCipherText.front().size(), SLOT_COUNT * sizeof(std::uint64_t));
    ASSERT_TRUE(response.valueCipherText.has_value());
    EXPECT_EQ(response.valueCipherText->size(), elements.size());

    const auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "two");
}

TEST(PrivacyProtocolTest, PirReturnsAllAttachedValuesAcrossBatchesInScanOrder)
{
    ConfigureProtocol();
    std::vector<Element> elements;
    elements.reserve(257);
    elements.push_back(MakeElement(2, "first"));
    for (std::size_t index = 0; index < 255; ++index) {
        elements.push_back(MakeElement(1000 + index, std::to_string(1000 + index)));
    }
    elements.push_back(MakeElement(2, "last"));

    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(2), elements, PIR_PROTOCOL, response), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 2U);

    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 2U);
    EXPECT_EQ(std::string(result.attachedValues->at(0).begin(), result.attachedValues->at(0).end()), "first");
    EXPECT_EQ(std::string(result.attachedValues->at(1).begin(), result.attachedValues->at(1).end()), "last");
}

TEST(PrivacyProtocolTest, PirMatchMayHaveNoAttachedValue)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(2), {MakeElement(1, "one"), MakeElement(2)}, PIR_PROTOCOL, response),
              PRIVACY_PROTOCOL_SUCCESS);

    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PrivacyProtocolTest, FixedCapacitiesAre512ForPsiAnd256ForPir)
{
    ConfigureProtocol();
    std::vector<Element> intersectionElements;
    for (std::size_t i = 0; i < 513; ++i) {
        intersectionElements.push_back(MakeElement(i + 1));
    }
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(513), intersectionElements, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 2U);
    auto decrypted = DecryptOutsideProtocol(response);
    ASSERT_EQ(decrypted.resultCipherText.size(), 2U);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);

    std::vector<Element> pirElements;
    for (std::size_t i = 0; i < 257; ++i) {
        pirElements.push_back(MakeElement(i + 1, std::to_string(i + 1)));
    }
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(257), pirElements, PIR_PROTOCOL, response), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 2U);
    ASSERT_TRUE(response.valueCipherText.has_value());
    EXPECT_EQ(response.valueCipherText->size(), pirElements.size());
    decrypted = DecryptOutsideProtocol(response);
    ASSERT_EQ(decrypted.resultCipherText.size(), 2U);
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "257");
}

TEST(PrivacyProtocolTest, OneHundredTwentyEightBitPirUsesCapacity128AcrossBothPublicApis)
{
    const auto he = ConfigureProtocol();
    std::vector<Element> elements;
    for (std::size_t i = 0; i < 129; ++i) {
        elements.push_back(Make128BitElement(i + 1, std::to_string(i + 1)));
    }

    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(Make128BitPrivacyTarget(129), elements, PIR_128_PROTOCOL, response),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 2U);
    for (const auto &batch : response.resultCipherText) {
        EXPECT_EQ(batch.size(), SLOT_COUNT * sizeof(std::uint64_t));
    }
    ASSERT_TRUE(response.valueCipherText.has_value());
    EXPECT_EQ(response.valueCipherText->size(), elements.size());

    const auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PIR_128_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "129");

    ASSERT_FALSE(he->compatibleParams.empty());
    ExpectCoreParams(he->compatibleParams.back(), ProtocolMode::PIR, 128, 8, 256);
}

TEST(PrivacyProtocolTest, GetSearchResultDoesNotDependOnHeProvider)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(
        PrivacySearch(MakePrivacyTarget(2), {MakeElement(1, "one"), MakeElement(2, "two")}, PIR_PROTOCOL, response),
        PRIVACY_PROTOCOL_SUCCESS);
    const auto decrypted = DecryptOutsideProtocol(response);

    SetHEEngine(nullptr);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "two");
}

TEST(PrivacyProtocolTest, PsiGetSearchResultNeedsNoHeEngine)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(2), {MakeElement(1), MakeElement(2)}, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);
    const auto decrypted = DecryptOutsideProtocol(response);

    SetHEEngine(nullptr);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PrivacyProtocolTest, GetSearchResultRejectsMalformedDecryptedStructure)
{
    SetHEEngine(nullptr);

    const auto expectCodecError = [](const PrivacySearchResult &input, const PrivacyProtocol &protocol) {
        SearchResult result;
        EXPECT_EQ(PostProcessSearchResult(input.resultCipherText, input.valueCipherText, protocol, result),
                  PRIVACY_PROTOCOL_CODEC_ERROR);
    };

    PrivacySearchResult malformed;
    malformed.resultCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT - 1, 1)));
    expectCodecError(malformed, PRIVATE_SET_INTERSECTION);

    malformed = {};
    malformed.resultCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.valueCipherText.emplace();
    expectCodecError(malformed, PRIVATE_SET_INTERSECTION);

    malformed = {};
    malformed.resultCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    expectCodecError(malformed, PIR_PROTOCOL);

    malformed = {};
    malformed.resultCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.valueCipherText.emplace();
    expectCodecError(malformed, PIR_PROTOCOL);

    malformed = {};
    malformed.resultCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.valueCipherText = std::vector<Bytes>{Bytes{}};
    expectCodecError(malformed, PIR_PROTOCOL);

    malformed = {};
    malformed.resultCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.valueCipherText = std::vector<Bytes>(257, Bytes{1});
    expectCodecError(malformed, PIR_PROTOCOL);
}

TEST(PrivacyProtocolTest, PlaintextCodecDecodesLittleEndianUint64Slots)
{
    Bytes plaintext(SLOT_COUNT * sizeof(std::uint64_t), 0);
    const Bytes firstSlot{0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    ASSERT_EQ(memcpy_s(plaintext.data(), plaintext.size(), firstSlot.data(), firstSlot.size()), EOK);

    PrivacySearchResult input;
    input.resultCipherText.push_back(std::move(plaintext));
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PRIVATE_SET_INTERSECTION, mapped), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolCodec codec;
    std::vector<Slots> parsed;
    ASSERT_EQ(codec.ParsePlaintexts(input.resultCipherText, input.valueCipherText, mapped, parsed),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(parsed.size(), 1U);
    ASSERT_EQ(parsed.front().size(), SLOT_COUNT);
    EXPECT_EQ(parsed.front().front(), std::uint64_t{0x0102030405060708ULL});
}

TEST(PrivacyProtocolTest, FailedPlaintextParsingDoesNotModifyOutput)
{
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PRIVATE_SET_INTERSECTION, mapped), PRIVACY_PROTOCOL_SUCCESS);
    ProtocolCodec codec;
    std::vector<Slots> parsed{Slots{7, 8, 9}};
    const auto original = parsed;
    PrivacySearchResult malformed;
    malformed.resultCipherText.push_back(Bytes{1});

    EXPECT_EQ(codec.ParsePlaintexts(malformed.resultCipherText, malformed.valueCipherText, mapped, parsed),
              PRIVACY_PROTOCOL_CODEC_ERROR);
    EXPECT_EQ(parsed, original);
}

TEST(PrivacyProtocolTest, NonZeroQueryDoesNotMatchZeroPaddedPsiCapacity)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(9), {MakeElement(1)}, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(response.resultCipherText.size(), 1U);

    const auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(PostProcessSearchResult(decrypted.resultCipherText, decrypted.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PrivacyProtocolTest, ZeroQueryShowsWhyPsiPaddingIsAnInvalidProtocolInput)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakeZeroPrivacyTarget(), {MakeElement(1)}, PRIVATE_SET_INTERSECTION, response),
              PRIVACY_PROTOCOL_SUCCESS);

    // With no item count in the northbound DTO, PostProcessSearchResult scans all 512 groups.
    // A zero query makes every zero-padded group decrypt to zero, so the wire
    // result cannot distinguish such padding from a real match. Target
    // generation must therefore reject the all-zero comparison vector.
    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PRIVATE_SET_INTERSECTION,
                                      result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
}

TEST(PrivacyProtocolTest, NonZeroQueryDoesNotMatchZeroPaddedPirCapacity)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakePrivacyTarget(9), {MakeElement(1, "one")}, PIR_PROTOCOL, response),
              PRIVACY_PROTOCOL_SUCCESS);

    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    ASSERT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PrivacyProtocolTest, ZeroQueryPirPaddingCannotIndexPastSymmetricCiphertexts)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(PrivacySearch(MakeZeroPrivacyTarget(), {MakeElement(1, "one")}, PIR_PROTOCOL, response),
              PRIVACY_PROTOCOL_SUCCESS);

    SearchResult result;
    const auto plaintexts = DecryptOutsideProtocol(response);
    EXPECT_EQ(PostProcessSearchResult(plaintexts.resultCipherText, plaintexts.valueCipherText, PIR_PROTOCOL, result),
              PRIVACY_PROTOCOL_CODEC_ERROR);
}

TEST(PrivacyProtocolTest, SeedPackingRoundTripsWithFixedPirPreset)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, mapped), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(seed, mapped, encoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(encoded, Slots({0xcdef, 0x89ab, 0x4567, 0x0123}));
    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, mapped, decoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(decoded, seed);
}

TEST(PrivacyProtocolTest, SeedPackingRoundTripsAllSixteenBytesFor128BitPir)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, mapped), PRIVACY_PROTOCOL_SUCCESS);
    const Bytes seed{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(seed, mapped, encoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(encoded, Slots({0xeeff, 0xccdd, 0xaabb, 0x8899, 0x6677, 0x4455, 0x2233, 0x0011}));
    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, mapped, decoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(decoded, seed);
}

TEST(PrivacyProtocolTest, SeedPackingSupportsSlotsThatAreNotByteAligned)
{
    ProtocolEncoder encoder;
    ProtocolParams params;
    params.compareBits = 72;
    params.bitsPerSlot = 12;
    params.slotsPerData = 6;
    params.plainModulus = 4097;

    const Bytes input{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(input, params, encoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(encoded, Slots({0x012, 0xdef, 0xabc, 0x789, 0x456, 0x123}));

    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, params, decoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(decoded, input);
}

TEST(PrivacyProtocolTest, SeedPackingSupportsTenBitSlotsAcrossByteBoundaries)
{
    ProtocolEncoder encoder;
    ProtocolParams params;
    params.compareBits = 80;
    params.bitsPerSlot = 10;
    params.slotsPerData = 8;
    params.plainModulus = 1025;

    const Bytes input{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(input, params, encoded), PRIVACY_PROTOCOL_SUCCESS);

    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, params, decoded), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(decoded, input);
}

} // namespace

namespace {
using namespace testing::ext;
/* @tc.name: SuccessfulEngineMustReturnHandle
 * @tc.desc: Reject a broken engine before arithmetic without publishing partial search results.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolHeContractTest, SuccessfulEngineMustReturnHandle, TestSize.Level2)
{
    ScopedProviderReset reset;
    auto engine = ConfigureProtocol();
    engine->emptyEvaluation = true;
    PrivacySearchResult output{{Bytes{0x99}}, std::nullopt};
    EXPECT_EQ(PrivacySearch(MakePrivacyTarget(2), {MakeElement(2)}, PRIVATE_SET_INTERSECTION, output),
              PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(output.resultCipherText, (std::vector<Bytes>{{0x99}}));
}
} // namespace
