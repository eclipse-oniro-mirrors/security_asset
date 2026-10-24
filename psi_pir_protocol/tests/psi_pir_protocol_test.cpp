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

#include "psi_pir_protocol.h"
#include "search_result_processor.h"

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
using namespace psi_pir;
using namespace psi_pir::internal;

constexpr PrivacyProtocol PSI_PROTOCOL{DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};
constexpr PrivacyProtocol PIR_PROTOCOL{DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL};
constexpr PrivacyProtocol PIR_128_PROTOCOL{DataSetSize::SIZE_128, ProtocolType::PIR_PROTOCOL};
constexpr std::size_t SLOT_COUNT = 512U * 4U;
constexpr std::uint64_t PLAIN_MODULUS = 65537;

Bytes SerializeSlots(const Slots &slots)
{
    Bytes output;
    output.reserve(slots.size() * sizeof(std::uint64_t));
    for (const auto value : slots) {
        for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
            output.push_back(static_cast<std::uint8_t>(value >> (byte * 8)));
        }
    }
    return output;
}

int32_t DeserializeSlots(const Bytes &bytes, Slots &output)
{
    if (bytes.size() % sizeof(std::uint64_t) != 0) {
        return PSI_PIR_CODEC_ERROR;
    }
    Slots slots(bytes.size() / sizeof(std::uint64_t));
    for (std::size_t slot = 0; slot < slots.size(); ++slot) {
        for (std::size_t byte = 0; byte < sizeof(std::uint64_t); ++byte) {
            slots[slot] |= static_cast<std::uint64_t>(bytes[slot * sizeof(std::uint64_t) + byte]) << (byte * 8);
        }
    }
    output = std::move(slots);
    return PSI_PIR_SUCCESS;
}

// Builds a deterministic target fixture for protocol evaluation tests.
// The TEE takes the comparison suffix as big-endian bytes and places its least
// significant 16-bit word in the first slot, then repeats the group until all
// 2048 HE slots are filled.
Bytes MakePrivacyTargetFromSuffix(const Bytes &compareSuffix, std::size_t slotsPerData)
{
    Slots group;
    group.reserve(slotsPerData);
    for (std::size_t slot = 0; slot < slotsPerData; ++slot) {
        const auto end = compareSuffix.size() - slot * 2;
        group.push_back((static_cast<std::uint64_t>(compareSuffix[end - 2]) << 8) | compareSuffix[end - 1]);
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
        output[output.size() - byte - 1] = static_cast<std::uint8_t>(value >> (byte * 8));
    }
    return output;
}

Bytes HexBytes(const std::string &hex)
{
    if (hex.size() % 2 != 0) {
        return {};
    }
    const auto nibble = [](char value) -> std::uint8_t {
        if (value >= '0' && value <= '9') {
            return static_cast<std::uint8_t>(value - '0');
        }
        if (value >= 'a' && value <= 'f') {
            return static_cast<std::uint8_t>(value - 'a' + 10);
        }
        return 0xff;
    };
    Bytes output;
    output.reserve(hex.size() / 2);
    for (std::size_t index = 0; index < hex.size(); index += 2) {
        const auto high = nibble(hex[index]);
        const auto low = nibble(hex[index + 1]);
        if (high > 0x0f || low > 0x0f) {
            return {};
        }
        output.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return output;
}

int32_t BuiltInDigest(HashAlg algorithm, const Bytes &input, Bytes &output)
{
    const auto provider = GetOpenSslHashProvider();
    return provider ? provider->digest(algorithm, input, output) : PSI_PIR_HASH_ERROR;
}

int32_t BuiltInFillRandom(Bytes &output)
{
    const auto provider = GetOpenSslRandomSource();
    return provider ? provider->fill(output) : PSI_PIR_RANDOM_ERROR;
}

int32_t BuiltInEncryptOrderValue(const Bytes &seed, const std::optional<Bytes> &value, const ProtocolParams &params,
                                 Bytes &output)
{
    const auto provider = GetOpenSslPirValueCrypto();
    return provider ? provider->encrypt(seed, value, params, output) : PSI_PIR_CRYPTO_ERROR;
}

int32_t BuiltInDecryptOrderValue(const Bytes &seed, const Bytes &ciphertext, const ProtocolParams &params,
                                 std::optional<Bytes> &output)
{
    const auto provider = GetOpenSslPirValueCrypto();
    return provider ? provider->decrypt(seed, ciphertext, params, output) : PSI_PIR_CRYPTO_ERROR;
}

Bytes MakePrivacyTargetForKey(const Bytes &key, std::size_t slotsPerData)
{
    Bytes digest;
    EXPECT_EQ(BuiltInDigest(HashAlg::SHA256, key, digest), PSI_PIR_SUCCESS);
    if (digest.size() < slotsPerData * 2) {
        return {};
    }
    return MakePrivacyTargetFromSuffix(
        Bytes(digest.end() - static_cast<std::ptrdiff_t>(slotsPerData * 2), digest.end()), slotsPerData);
}

Bytes MakePrivacyTarget(std::uint64_t compareValue)
{
    return MakePrivacyTargetForKey(CompareSuffix(compareValue, 8), 4);
}

Bytes Make128BitPrivacyTarget(std::uint64_t compareValue)
{
    return MakePrivacyTargetForKey(CompareSuffix(compareValue, 16), 8);
}

Bytes MakeZeroPrivacyTarget()
{
    return MakePrivacyTargetFromSuffix(Bytes(8, 0), 4);
}

// Simulates raw HE decryption for internal post-processing tests, using the
// same PrivacySearchResult DTO before and after this step: before decryption
// fheCipherText contains HE ciphertexts; afterwards each entry contains one
// serialized 2048-slot HE plaintext.
PrivacySearchResult DecryptOutsideProtocol(const PrivacySearchResult &response)
{
    PrivacySearchResult decrypted;
    decrypted.symCipherText = response.symCipherText;
    for (const auto &batch : response.fheCipherText) {
        Slots slots;
        const auto ret = DeserializeSlots(batch, slots);
        EXPECT_EQ(ret, PSI_PIR_SUCCESS);
        if (ret != PSI_PIR_SUCCESS) {
            return {};
        }
        EXPECT_EQ(slots.size(), SLOT_COUNT);
        if (slots.size() != SLOT_COUNT) {
            return {};
        }
        decrypted.fheCipherText.push_back(SerializeSlots(slots));
    }
    return decrypted;
}

class RecordingHash final : public IHashProvider {
public:
    int32_t digest(HashAlg algorithm, const Bytes &input, Bytes &output) const noexcept override
    {
        algorithms.push_back(algorithm);
        inputs.push_back(input);
        Bytes digest(32, 0);
        const auto copySize = std::min(digest.size(), input.size());
        std::copy(input.end() - static_cast<std::ptrdiff_t>(copySize), input.end(),
                  digest.end() - static_cast<std::ptrdiff_t>(copySize));
        output = std::move(digest);
        return PSI_PIR_SUCCESS;
    }

    mutable std::vector<HashAlg> algorithms;
    mutable std::vector<Bytes> inputs;
};

class RecordingRandom final : public IRandomSource {
public:
    int32_t fill(Bytes &output) noexcept override
    {
        requestedSizes.push_back(output.size());
        std::fill(output.begin(), output.end(), nextValue_++);
        return PSI_PIR_SUCCESS;
    }

    std::vector<std::size_t> requestedSizes;

private:
    std::uint8_t nextValue_{0x31};
};

class BoundaryRandom final : public IRandomSource {
public:
    int32_t fill(Bytes &output) noexcept override
    {
        requestedSizes.push_back(output.size());
        std::fill(output.begin(), output.end(), 0x12);
        if (output.size() >= 4) {
            output[0] = 0x00;
            output[1] = 0x00;
            output[2] = 0xff;
            output[3] = 0xff;
        }
        return PSI_PIR_SUCCESS;
    }

    std::vector<std::size_t> requestedSizes;
};

class RecordingPirValueCrypto final : public IPirValueCrypto {
public:
    int32_t encrypt(const Bytes &seed, const std::optional<Bytes> &value, const ProtocolParams &params,
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
        return PSI_PIR_SUCCESS;
    }

    int32_t decrypt(const Bytes &seed, const Bytes &ciphertext, const ProtocolParams &params,
                    std::optional<Bytes> &output) const noexcept override
    {
        decryptParams.push_back(params);
        if (ciphertext.empty()) {
            return PSI_PIR_CRYPTO_ERROR;
        }
        std::size_t offset = 0;
        const auto seedSize = ciphertext[offset++];
        if (seed.size() != seedSize || offset + seedSize > ciphertext.size() ||
            !std::equal(seed.begin(), seed.end(), ciphertext.begin() + static_cast<std::ptrdiff_t>(offset))) {
            return PSI_PIR_CRYPTO_ERROR;
        }
        offset += seedSize;
        if (offset >= ciphertext.size()) {
            return PSI_PIR_CRYPTO_ERROR;
        }
        if (ciphertext[offset++] == 0) {
            if (offset != ciphertext.size()) {
                return PSI_PIR_CRYPTO_ERROR;
            }
            output = std::nullopt;
            return PSI_PIR_SUCCESS;
        }
        if (offset >= ciphertext.size()) {
            return PSI_PIR_CRYPTO_ERROR;
        }
        const auto size = ciphertext[offset++];
        if (offset + size != ciphertext.size()) {
            return PSI_PIR_CRYPTO_ERROR;
        }
        output = Bytes(ciphertext.begin() + static_cast<std::ptrdiff_t>(offset), ciphertext.end());
        return PSI_PIR_SUCCESS;
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

class CleartextHe final : public IHEEngine {
public:
    bool IsCompatible(const ProtocolParams &params) const noexcept override
    {
        compatibleParams.push_back(params);
        return compatible;
    }

    int32_t addPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept override
    {
        Slots slots;
        if (DeserializeSlots(ciphertext, slots) != PSI_PIR_SUCCESS || slots.size() != plaintext.size()) {
            return PSI_PIR_HE_ERROR;
        }
        // Model the externally injected TA contract: addPlain(C, P) decrypts to P - C.
        for (std::size_t i = 0; i < slots.size(); ++i) {
            const auto subtrahend = plaintext[i] % PLAIN_MODULUS;
            slots[i] = (subtrahend + PLAIN_MODULUS - slots[i]) % PLAIN_MODULUS;
        }
        output = SerializeSlots(slots);
        return PSI_PIR_SUCCESS;
    }

    int32_t multiplyPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept override
    {
        Slots slots;
        if (DeserializeSlots(ciphertext, slots) != PSI_PIR_SUCCESS || slots.size() != plaintext.size()) {
            return PSI_PIR_HE_ERROR;
        }
        for (std::size_t i = 0; i < slots.size(); ++i) {
            slots[i] = (slots[i] * plaintext[i]) % PLAIN_MODULUS;
        }
        output = SerializeSlots(slots);
        return PSI_PIR_SUCCESS;
    }

    int32_t RandCipherText(const Bytes &ciphertext, Bytes &output) const noexcept override
    {
        output = ciphertext; // Test-only identity rerandomization, never a production backend.
        return PSI_PIR_SUCCESS;
    }

    bool compatible{true};
    mutable std::vector<ProtocolParams> compatibleParams;
};

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
    Bytes output(8);
    for (std::size_t byte = 0; byte < output.size(); ++byte) {
        output[output.size() - byte - 1] = static_cast<std::uint8_t>(value >> (byte * 8));
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
    element.elemKey = CompareSuffix(key, 16);
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

TEST(PsiPirProtocolTargetTest, RepeatsComparisonGroupAcrossAllSlots)
{
    const Element element{Bytes{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, HashAlg::NONE, std::nullopt};
    const Slots expected{0x0f10, 0x0d0e, 0x0b0c, 0x090a, 0x0708, 0x0506, 0x0304, 0x0102};
    for (const auto &protocol : {PSI_PROTOCOL, PIR_PROTOCOL, PIR_128_PROTOCOL}) {
        ProtocolParams params;
        ASSERT_EQ(MapPrivacyProtocol(protocol, params), PSI_PIR_SUCCESS);
        Slots target;
        ASSERT_EQ(ProtocolEncoder().EncodeTargetVector(element, params, target), PSI_PIR_SUCCESS);
        ASSERT_EQ(target.size(), 2048U);
        EXPECT_EQ(target.size() / params.slotsPerData, params.slotsPerData == 4 ? 512U : 256U);
        for (std::size_t slot = 0; slot < target.size(); ++slot) {
            EXPECT_EQ(target[slot], expected[slot % params.slotsPerData]) << "slot=" << slot;
        }
    }
}

TEST(PsiPirProtocolTargetTest, InvalidTargetPackingPreservesOutput)
{
    ProtocolEncoder encoder;
    const Element element{Bytes{1, 2, 3, 4, 5, 6, 7, 8}, HashAlg::NONE, std::nullopt};
    Slots target{99};
    ProtocolParams params;
    params.polyModulusDegree = 2047;
    EXPECT_EQ(encoder.EncodeTargetVector(element, params, target), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    params = ProtocolParams{};
    params.slotsPerData = 0;
    EXPECT_EQ(encoder.EncodeTargetVector(element, params, target), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    params = ProtocolParams{};
    params.dataNumPerPlaintext = 256;
    EXPECT_EQ(encoder.EncodeTargetVector(element, params, target), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    params = ProtocolParams{};
    const Element zero{Bytes(8, 0), HashAlg::NONE, std::nullopt};
    EXPECT_EQ(encoder.EncodeTargetVector(zero, params, target), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
    const Element shortInput{Bytes{1}, HashAlg::NONE, std::nullopt};
    EXPECT_EQ(encoder.EncodeTargetVector(shortInput, params, target), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(target, Slots({99}));
}

TEST(PsiPirProtocolHeContractTest, AddPlainMeansPlaintextMinusCiphertext)
{
    CleartextHe he;
    const Slots ciphertextSlots{1, 0, PLAIN_MODULUS - 1};
    const Slots plaintext{2, 1, PLAIN_MODULUS - 1};
    Bytes output;
    ASSERT_EQ(he.addPlain(SerializeSlots(ciphertextSlots), plaintext, output), PSI_PIR_SUCCESS);

    Slots result;
    ASSERT_EQ(DeserializeSlots(output, result), PSI_PIR_SUCCESS);
    EXPECT_EQ(result, Slots({1, 1, 0}));
}

TEST(PsiPirProtocolOpenSslTest, HashAlgorithmsMatchKnownAnswerVectors)
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
        ASSERT_EQ(BuiltInDigest(algorithm, input, digest), PSI_PIR_SUCCESS);
        EXPECT_EQ(digest, HexBytes(expectedHex));
    }

    Bytes unchanged{1, 2, 3};
    EXPECT_NE(BuiltInDigest(static_cast<HashAlg>(99), input, unchanged), PSI_PIR_SUCCESS);
    EXPECT_EQ(unchanged, Bytes({1, 2, 3}));
}

TEST(PsiPirProtocolOpenSslTest, Sha256SupportsEmptyInput)
{
    Bytes digest;
    ASSERT_EQ(BuiltInDigest(HashAlg::SHA256, {}, digest), PSI_PIR_SUCCESS);
    EXPECT_EQ(digest, HexBytes("e3b0c44298fc1c149afbf4c8996fb924"
                               "27ae41e4649b934ca495991b7852b855"));
}

TEST(PsiPirProtocolOpenSslTest, RandomGenerationFillsRequestedBuffer)
{
    Bytes first(32, 0);
    Bytes second(32, 0);
    ASSERT_EQ(BuiltInFillRandom(first), PSI_PIR_SUCCESS);
    ASSERT_EQ(BuiltInFillRandom(second), PSI_PIR_SUCCESS);
    EXPECT_NE(first, Bytes(32, 0));
    EXPECT_NE(second, Bytes(32, 0));
    EXPECT_NE(first, second);

    Bytes empty;
    EXPECT_NE(BuiltInFillRandom(empty), PSI_PIR_SUCCESS);
    EXPECT_TRUE(empty.empty());
}

TEST(PsiPirProtocolOpenSslTest, AesGcmRoundTripsWithBothSupportedSeedSizes)
{
    const std::array<std::pair<PrivacyProtocol, Bytes>, 2> cases{{
        {PIR_PROTOCOL, Bytes{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}},
        {PIR_128_PROTOCOL,
         Bytes{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}},
    }};
    for (const auto &[protocol, seed] : cases) {
        ProtocolParams params;
        ASSERT_EQ(MapPrivacyProtocol(protocol, params), PSI_PIR_SUCCESS);
        const std::optional<Bytes> value{Bytes{'v', 'a', 'l', 'u', 'e'}};
        Bytes ciphertext;
        ASSERT_EQ(BuiltInEncryptOrderValue(seed, value, params, ciphertext), PSI_PIR_SUCCESS);
        EXPECT_FALSE(ciphertext.empty());

        std::optional<Bytes> plaintext;
        ASSERT_EQ(BuiltInDecryptOrderValue(seed, ciphertext, params, plaintext), PSI_PIR_SUCCESS);
        EXPECT_EQ(plaintext, value);
    }
}

TEST(PsiPirProtocolOpenSslTest, AesGcmPreservesAbsentAndPresentEmptyValues)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PSI_PIR_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

    Bytes absentCiphertext;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, std::nullopt, params, absentCiphertext), PSI_PIR_SUCCESS);
    std::optional<Bytes> absentOutput{Bytes{9}};
    ASSERT_EQ(BuiltInDecryptOrderValue(seed, absentCiphertext, params, absentOutput), PSI_PIR_SUCCESS);
    EXPECT_FALSE(absentOutput.has_value());

    Bytes emptyCiphertext;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, Bytes{}, params, emptyCiphertext), PSI_PIR_SUCCESS);
    std::optional<Bytes> emptyOutput;
    ASSERT_EQ(BuiltInDecryptOrderValue(seed, emptyCiphertext, params, emptyOutput), PSI_PIR_SUCCESS);
    ASSERT_TRUE(emptyOutput.has_value());
    EXPECT_TRUE(emptyOutput->empty());
}

TEST(PsiPirProtocolOpenSslTest, AesGcmRejectsTamperingAndWrongSeedWithoutChangingOutput)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PSI_PIR_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    Bytes ciphertext;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, Bytes{'s', 'e', 'c', 'r', 'e', 't'}, params, ciphertext), PSI_PIR_SUCCESS);

    const std::array<std::size_t, 4> tamperOffsets{4, 16, 28, ciphertext.size() - 1};
    for (const auto offset : tamperOffsets) {
        auto tampered = ciphertext;
        tampered[offset] ^= 0x01;
        std::optional<Bytes> output{Bytes{7, 8, 9}};
        EXPECT_NE(BuiltInDecryptOrderValue(seed, tampered, params, output), PSI_PIR_SUCCESS);
        EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));
    }

    auto wrongSeed = seed;
    wrongSeed.front() ^= 0x01;
    std::optional<Bytes> output{Bytes{7, 8, 9}};
    EXPECT_NE(BuiltInDecryptOrderValue(wrongSeed, ciphertext, params, output), PSI_PIR_SUCCESS);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));

    auto truncated = ciphertext;
    truncated.pop_back();
    EXPECT_NE(BuiltInDecryptOrderValue(seed, truncated, params, output), PSI_PIR_SUCCESS);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));

    auto extended = ciphertext;
    extended.push_back(0);
    EXPECT_NE(BuiltInDecryptOrderValue(seed, extended, params, output), PSI_PIR_SUCCESS);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7, 8, 9}));
}

TEST(PsiPirProtocolOpenSslTest, AesGcmUsesFreshNonceAndAuthenticatedFrameHeader)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PSI_PIR_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    const std::optional<Bytes> value{Bytes{'v'}};
    Bytes first;
    Bytes second;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, value, params, first), PSI_PIR_SUCCESS);
    ASSERT_EQ(BuiltInEncryptOrderValue(seed, value, params, second), PSI_PIR_SUCCESS);
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

TEST(PsiPirProtocolOpenSslTest, AesGcmDecryptsFixedInteroperabilityVector)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PSI_PIR_SUCCESS);
    const Bytes seed = HexBytes("0123456789abcdef");
    const Bytes frame = HexBytes("5049523101010c100100004000000006"
                                 "000102030405060708090a0b"
                                 "66ebe4ec5a3c"
                                 "9549d1d36f92a3c89ae969621786be62");
    std::optional<Bytes> value;
    ASSERT_EQ(BuiltInDecryptOrderValue(seed, frame, params, value), PSI_PIR_SUCCESS);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, Bytes({'v', 'a', 'l', 'u', 'e'}));
}

TEST(PsiPirProtocolOpenSslTest, AesGcmRejectsProtocolMismatchWithoutChangingOutput)
{
    ProtocolParams params64;
    ProtocolParams params128;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params64), PSI_PIR_SUCCESS);
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, params128), PSI_PIR_SUCCESS);
    const Bytes seed64 = HexBytes("0123456789abcdef");
    Bytes frame;
    ASSERT_EQ(BuiltInEncryptOrderValue(seed64, Bytes{'v'}, params64, frame), PSI_PIR_SUCCESS);

    std::optional<Bytes> output{Bytes{7}};
    EXPECT_EQ(BuiltInDecryptOrderValue(seed64, frame, params128, output), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7}));

    const Bytes seed128(16, 1);
    EXPECT_EQ(BuiltInDecryptOrderValue(seed128, frame, params128, output), PSI_PIR_CRYPTO_ERROR);
    EXPECT_EQ(output, std::optional<Bytes>(Bytes{7}));
}

TEST(PsiPirProtocolProviderTest, CustomProvidersAreSelected)
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

TEST(PsiPirProtocolProviderTest, CustomProvidersDriveEvaluationAndResolution)
{
    ScopedProviderReset resetProviders;
    const auto providers = InstallRecordingProviders();

    const auto target = MakePrivacyTargetFromSuffix(CompareSuffix(2, 8), 4);
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(target, {MakeElement(1, "one"), MakeElement(2, "two")}, PIR_PROTOCOL, response),
              PSI_PIR_SUCCESS);

    ASSERT_EQ(response.fheCipherText.size(), 1U);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "two");

    EXPECT_EQ(providers.hash->algorithms, std::vector<HashAlg>({HashAlg::SHA256, HashAlg::SHA256}));
    EXPECT_EQ(providers.hash->inputs, std::vector<Bytes>({EightByteKey(1), EightByteKey(2)}));
    EXPECT_EQ(providers.randomSource->requestedSizes, std::vector<std::size_t>({8U, 8U, SLOT_COUNT * 2U, SLOT_COUNT}));
    ASSERT_EQ(providers.pirValue->encryptParams.size(), 2U);
    ASSERT_EQ(providers.pirValue->decryptParams.size(), 1U);
    ExpectCoreParams(providers.pirValue->encryptParams.back(), ProtocolMode::PIR);
    ExpectCoreParams(providers.pirValue->decryptParams.back(), ProtocolMode::PIR);
}

TEST(PsiPirProtocolProviderTest, GetSearchResultReturnsAllPirMatchesAcrossBatchesInOrder)
{
    ScopedProviderReset resetProviders;
    auto pirValueCrypto = std::make_shared<RecordingPirValueCrypto>();
    SetPirValueCrypto(pirValueCrypto);
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, params), PSI_PIR_SUCCESS);
    ProtocolEncoder encoder;

    const Bytes firstSeed{1, 2, 3, 4, 5, 6, 7, 8};
    const Bytes lastSeed{8, 7, 6, 5, 4, 3, 2, 1};
    Bytes firstCiphertext;
    Bytes lastCiphertext;
    ASSERT_EQ(pirValueCrypto->encrypt(firstSeed, Bytes{'f', 'i', 'r', 's', 't'}, params, firstCiphertext),
              PSI_PIR_SUCCESS);
    ASSERT_EQ(pirValueCrypto->encrypt(lastSeed, Bytes{'l', 'a', 's', 't'}, params, lastCiphertext), PSI_PIR_SUCCESS);

    Slots firstBatch(SLOT_COUNT, 1);
    Slots lastBatch(SLOT_COUNT, 1);
    std::fill_n(firstBatch.begin(), params.slotsPerData, 0);
    std::fill_n(lastBatch.begin(), params.slotsPerData, 0);
    Slots firstSeedSlots;
    Slots lastSeedSlots;
    ASSERT_EQ(encoder.EncodeSeed(firstSeed, params, firstSeedSlots), PSI_PIR_SUCCESS);
    ASSERT_EQ(encoder.EncodeSeed(lastSeed, params, lastSeedSlots), PSI_PIR_SUCCESS);
    std::copy(firstSeedSlots.begin(), firstSeedSlots.end(),
              firstBatch.begin() + static_cast<std::ptrdiff_t>(params.slotsPerData));
    std::copy(lastSeedSlots.begin(), lastSeedSlots.end(),
              lastBatch.begin() + static_cast<std::ptrdiff_t>(params.slotsPerData));

    PrivacySearchResult input;
    input.fheCipherText = {SerializeSlots(firstBatch), SerializeSlots(lastBatch)};
    input.symCipherText.emplace(257, Bytes{1});
    input.symCipherText->front() = firstCiphertext;
    input.symCipherText->back() = lastCiphertext;
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(input, PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 2U);
    EXPECT_EQ(result.attachedValues->at(0), Bytes({'f', 'i', 'r', 's', 't'}));
    EXPECT_EQ(result.attachedValues->at(1), Bytes({'l', 'a', 's', 't'}));
    EXPECT_EQ(pirValueCrypto->decryptParams.size(), 2U);
}

TEST(PsiPirProtocolProviderTest, IndependentPirMasksHideNonMatchingSeed)
{
    ScopedProviderReset resetProviders;
    const auto providers = InstallRecordingProviders();
    const auto target = MakePrivacyTargetFromSuffix(CompareSuffix(2, 8), 4);
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(target, {MakeElement(1, "one"), MakeElement(2, "two")}, PIR_PROTOCOL, response),
              PSI_PIR_SUCCESS);

    Slots evaluatedSlots;
    ASSERT_EQ(DeserializeSlots(response.fheCipherText.front(), evaluatedSlots), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluatedSlots.size(), SLOT_COUNT);
    ASSERT_EQ(providers.pirValue->encryptSeeds.size(), 2U);

    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    ProtocolEncoder encoder;

    Slots nonMatchSeedSlots;
    ASSERT_EQ(encoder.EncodeSeed(providers.pirValue->encryptSeeds[0], mapped, nonMatchSeedSlots), PSI_PIR_SUCCESS);
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
    ASSERT_EQ(encoder.EncodeSeed(providers.pirValue->encryptSeeds[1], mapped, matchedSeedSlots), PSI_PIR_SUCCESS);
    const auto &expectedMatchedSecond = matchedSeedSlots;
    EXPECT_TRUE(std::equal(expectedMatchedSecond.begin(), expectedMatchedSecond.end(),
                           evaluatedSlots.begin() + static_cast<std::ptrdiff_t>(matchedSecondOffset)));
}

TEST(PsiPirProtocolProviderTest, NullOverridesRestoreOpenSslDefaults)
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
    ASSERT_EQ(restoredHash->digest(HashAlg::SHA256, Bytes{'a', 'b', 'c'}, digest), PSI_PIR_SUCCESS);
    EXPECT_EQ(digest, HexBytes("ba7816bf8f01cfea414140de5dae2223"
                               "b00361a396177a9cb410ff61f20015ad"));
}

TEST(PsiPirProtocolProviderTest, PirMaskVectorCoversAllSlotsAndSeparatesPairedGroups)
{
    ScopedProviderReset resetProviders;
    auto randomSource = std::make_shared<RecordingRandom>();
    SetRandomSource(randomSource);

    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    ProtocolEncoder encoder;
    Slots maskVector;
    ASSERT_EQ(encoder.GenerateMaskVector(mapped, maskVector), PSI_PIR_SUCCESS);

    ASSERT_EQ(maskVector.size(), SLOT_COUNT);
    ASSERT_EQ(randomSource->requestedSizes, std::vector<std::size_t>({SLOT_COUNT * 2U, SLOT_COUNT}));

    // RecordingRandom fills the first request with 0x31 and the collision
    // retry with 0x32. The sampler maps each 16-bit candidate c to c + 1.
    constexpr std::uint64_t FIRST_GROUP_MASK = 0x3131U + 1U;
    constexpr std::uint64_t SECOND_GROUP_MASK = 0x3232U + 1U;
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
            EXPECT_EQ(maskVector[firstOffset + slot], FIRST_GROUP_MASK);
            EXPECT_EQ(maskVector[secondOffset + slot], SECOND_GROUP_MASK);
            EXPECT_NE(maskVector[firstOffset + slot], maskVector[secondOffset + slot]);
        }
    }
}

TEST(PsiPirProtocolProviderTest, MaskVectorMapsBoundaryCandidatesToNonZeroFieldElements)
{
    ScopedProviderReset resetProviders;
    auto randomSource = std::make_shared<BoundaryRandom>();
    SetRandomSource(randomSource);

    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PSI_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    ProtocolEncoder encoder;
    Slots maskVector;
    ASSERT_EQ(encoder.GenerateMaskVector(mapped, maskVector), PSI_PIR_SUCCESS);

    ASSERT_EQ(maskVector.size(), SLOT_COUNT);
    ASSERT_EQ(randomSource->requestedSizes, std::vector<std::size_t>({SLOT_COUNT * 2U}));
    EXPECT_EQ(maskVector[0], 1U);     // 0x0000 + 1
    EXPECT_EQ(maskVector[1], 65536U); // 0xffff + 1
    EXPECT_TRUE(std::all_of(maskVector.begin(), maskVector.end(), [&mapped](std::uint64_t value) {
        return value >= 1 && value < mapped.plainModulus && value % mapped.plainModulus != 0;
    }));
}

TEST(PsiPirProtocolProviderTest, MaskVectorSupportsTheSmallestNonZeroField)
{
    ScopedProviderReset resetProviders;
    auto randomSource = std::make_shared<RecordingRandom>();
    SetRandomSource(randomSource);
    ProtocolParams params;
    params.plainModulus = 2;

    ProtocolEncoder encoder;
    Slots maskVector;
    ASSERT_EQ(encoder.GenerateMaskVector(params, maskVector), PSI_PIR_SUCCESS);
    EXPECT_TRUE(std::all_of(maskVector.begin(), maskVector.end(), [](std::uint64_t value) { return value == 1; }));
}

TEST(PsiPirProtocolTest, MapsTheThreeSupportedPrivacyProtocols)
{
    ProtocolParams psi;
    ASSERT_EQ(MapPrivacyProtocol(PSI_PROTOCOL, psi), PSI_PIR_SUCCESS);
    ExpectCoreParams(psi, ProtocolMode::PSI);
    EXPECT_EQ(psi.symKeySize, 0U);
    EXPECT_EQ(psi.symAlgorithm, SymAlgorithm::NONE);

    ProtocolParams pir;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, pir), PSI_PIR_SUCCESS);
    ExpectCoreParams(pir, ProtocolMode::PIR);
    EXPECT_EQ(pir.symKeySize, 256U);
    EXPECT_EQ(pir.symAlgorithm, SymAlgorithm::AES_GCM);

    ProtocolParams pir128;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, pir128), PSI_PIR_SUCCESS);
    ExpectCoreParams(pir128, ProtocolMode::PIR, 128, 8, 256);
    EXPECT_EQ(pir128.symKeySize, 256U);
    EXPECT_EQ(pir128.symAlgorithm, SymAlgorithm::AES_GCM);

    EXPECT_EQ(static_cast<std::uint32_t>(DataSetSize::SIZE_128), 0U);
    EXPECT_EQ(static_cast<std::uint32_t>(DataSetSize::SIZE_256), 1U);
    EXPECT_EQ(static_cast<std::uint32_t>(DataSetSize::SIZE_512), 2U);

    ProtocolParams unsupported;
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_256, ProtocolType::PSI_PROTOCOL}, unsupported),
              PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_512, ProtocolType::PIR_PROTOCOL}, unsupported),
              PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_128, ProtocolType::PSI_PROTOCOL}, unsupported),
              PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({static_cast<DataSetSize>(99), ProtocolType::PSI_PROTOCOL}, unsupported),
              PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_512, static_cast<ProtocolType>(99)}, unsupported),
              PSI_PIR_INVALID_ARGUMENT);
}

TEST(PsiPirProtocolTest, FailedProtocolMappingDoesNotModifyOutput)
{
    ProtocolParams params;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, params), PSI_PIR_SUCCESS);
    const auto original = params;

    EXPECT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_128, ProtocolType::PSI_PROTOCOL}, params),
              PSI_PIR_INVALID_ARGUMENT);
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

TEST(PsiPirProtocolTest, ProtocolValidationRejectsMismatchedAlgorithmParameters)
{
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    ProtocolCodec codec;

    auto params = mapped;
    params.plainModulus = 65539;
    EXPECT_EQ(codec.ValidateProtocol(params), PSI_PIR_INVALID_ARGUMENT);

    params = mapped;
    params.polyModulusDegree = 4096;
    EXPECT_EQ(codec.ValidateProtocol(params), PSI_PIR_INVALID_ARGUMENT);

    params = mapped;
    params.fheAlgorithm = static_cast<FheAlgorithm>(99);
    EXPECT_EQ(codec.ValidateProtocol(params), PSI_PIR_INVALID_ARGUMENT);

    params = mapped;
    params.symKeySize = 128;
    EXPECT_EQ(codec.ValidateProtocol(params), PSI_PIR_INVALID_ARGUMENT);

    params = mapped;
    params.symAlgorithm = SymAlgorithm::NONE;
    EXPECT_EQ(codec.ValidateProtocol(params), PSI_PIR_INVALID_ARGUMENT);
}

TEST(PsiPirProtocolTest, BothPublicApisRejectUnsupportedPrivacyProtocols)
{
    const std::vector<PrivacyProtocol> unsupportedProtocols{
        {DataSetSize::SIZE_256, ProtocolType::PSI_PROTOCOL},
        {DataSetSize::SIZE_512, ProtocolType::PIR_PROTOCOL},
        {DataSetSize::SIZE_128, ProtocolType::PSI_PROTOCOL},
        {static_cast<DataSetSize>(99), ProtocolType::PSI_PROTOCOL},
        {DataSetSize::SIZE_512, static_cast<ProtocolType>(99)},
    };
    for (const auto &unsupported : unsupportedProtocols) {
        PrivacySearchResult response;
        SearchResult result;
        EXPECT_EQ(privacySearch({1}, {MakeElement(1)}, unsupported, response), PSI_PIR_INVALID_ARGUMENT);
        EXPECT_EQ(ProcessDecryptedSearchResult({}, unsupported, result), PSI_PIR_INVALID_ARGUMENT);
    }
}

TEST(PsiPirProtocolTest, InvalidElementHashIsRejectedBeforeHeEvaluation)
{
    SetHEEngine(nullptr);

    Element element = MakeElement(1);
    element.hashAlg = static_cast<HashAlg>(99);
    PrivacySearchResult response;
    EXPECT_EQ(privacySearch({1}, {element}, PSI_PROTOCOL, response), PSI_PIR_INVALID_ARGUMENT);
}

TEST(PsiPirProtocolTest, EmptyElementKeyKeepsTheExistingHashSemantics)
{
    ScopedProviderReset resetProviders;
    SetHEEngine(std::make_shared<CleartextHe>());

    Element element;
    element.elemKey = {};
    element.hashAlg = HashAlg::SHA256;
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTargetForKey({}, 4), {element}, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);

    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
}

TEST(PsiPirProtocolTest, EmptyEvaluationAndResolutionDoNotRequireHeEngine)
{
    SetHEEngine(nullptr);

    PrivacySearchResult psiResponse;
    EXPECT_EQ(privacySearch({1}, {}, PSI_PROTOCOL, psiResponse), PSI_PIR_SUCCESS);
    EXPECT_TRUE(psiResponse.fheCipherText.empty());
    EXPECT_FALSE(psiResponse.symCipherText.has_value());
    SearchResult result;
    EXPECT_EQ(ProcessDecryptedSearchResult({}, PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_FALSE(result.matchedResult);

    PrivacySearchResult pirResponse;
    EXPECT_EQ(privacySearch({1}, {}, PIR_PROTOCOL, pirResponse), PSI_PIR_SUCCESS);
    EXPECT_TRUE(pirResponse.fheCipherText.empty());
    ASSERT_TRUE(pirResponse.symCipherText.has_value());
    EXPECT_TRUE(pirResponse.symCipherText->empty());
    PrivacySearchResult emptyPir;
    emptyPir.symCipherText.emplace();
    EXPECT_EQ(ProcessDecryptedSearchResult(emptyPir, PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
}

TEST(PsiPirProtocolTest, SingleLayerElementUsesExplicitSha256)
{
    ConfigureProtocol();
    Element element;
    element.elemKey = EightByteKey(2);
    element.hashAlg = HashAlg::SHA256;
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(2), {element}, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
}

TEST(PsiPirProtocolTest, ServerVectorUsesTeeSuffixAndLeastSignificantSlotFirst)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PSI_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    Element element;
    element.elemKey = {0xaa, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    element.hashAlg = HashAlg::SHA256;
    Slots encoded;
    ASSERT_EQ(encoder.EncodeServerVector(element, mapped, encoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(encoded, Slots({49489, 33421, 53375, 60560}));
}

TEST(PsiPirProtocolTest, OneHundredTwentyEightBitServerVectorUsesTeeSuffixAndLsbSlots)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    Element element;
    element.elemKey = {0xaa, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                       0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    element.hashAlg = HashAlg::SHA256;
    Slots encoded;
    ASSERT_EQ(encoder.EncodeServerVector(element, mapped, encoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(encoded, Slots({9739, 63044, 39120, 38765, 38004, 26788, 25817, 29794}));
}

TEST(PsiPirProtocolTest, PsiMatchesAndMissesAfterExternalHeDecryption)
{
    ConfigureProtocol();
    const std::vector<Element> elements{MakeElement(1, "ignored"), MakeElement(2, "also ignored"), MakeElement(3)};

    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(2), elements, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 1U);
    EXPECT_EQ(response.fheCipherText.front().size(), SLOT_COUNT * sizeof(std::uint64_t));
    EXPECT_FALSE(response.symCipherText.has_value());

    auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());

    ASSERT_EQ(privacySearch(MakePrivacyTarget(9), elements, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);
    decrypted = DecryptOutsideProtocol(response);
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PsiPirProtocolTest, PirReturnsOneAttachedValue)
{
    ConfigureProtocol();
    const std::vector<Element> elements{MakeElement(1, "one"), MakeElement(2, "two"), MakeElement(3, "three")};
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(2), elements, PIR_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 1U);
    EXPECT_EQ(response.fheCipherText.front().size(), SLOT_COUNT * sizeof(std::uint64_t));
    ASSERT_TRUE(response.symCipherText.has_value());
    EXPECT_EQ(response.symCipherText->size(), elements.size());

    const auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "two");
}

TEST(PsiPirProtocolTest, PirReturnsAllAttachedValuesAcrossBatchesInScanOrder)
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
    ASSERT_EQ(privacySearch(MakePrivacyTarget(2), elements, PIR_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 2U);

    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 2U);
    EXPECT_EQ(std::string(result.attachedValues->at(0).begin(), result.attachedValues->at(0).end()), "first");
    EXPECT_EQ(std::string(result.attachedValues->at(1).begin(), result.attachedValues->at(1).end()), "last");
}

TEST(PsiPirProtocolTest, PirMatchMayHaveNoAttachedValue)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(2), {MakeElement(1, "one"), MakeElement(2)}, PIR_PROTOCOL, response),
              PSI_PIR_SUCCESS);

    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_TRUE(result.attachedValues->front().empty());
}

TEST(PsiPirProtocolTest, FixedCapacitiesAre512ForPsiAnd256ForPir)
{
    ConfigureProtocol();
    std::vector<Element> psiElements;
    for (std::size_t i = 0; i < 513; ++i) {
        psiElements.push_back(MakeElement(i + 1));
    }
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(513), psiElements, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 2U);
    auto decrypted = DecryptOutsideProtocol(response);
    ASSERT_EQ(decrypted.fheCipherText.size(), 2U);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);

    std::vector<Element> pirElements;
    for (std::size_t i = 0; i < 257; ++i) {
        pirElements.push_back(MakeElement(i + 1, std::to_string(i + 1)));
    }
    ASSERT_EQ(privacySearch(MakePrivacyTarget(257), pirElements, PIR_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 2U);
    ASSERT_TRUE(response.symCipherText.has_value());
    EXPECT_EQ(response.symCipherText->size(), pirElements.size());
    decrypted = DecryptOutsideProtocol(response);
    ASSERT_EQ(decrypted.fheCipherText.size(), 2U);
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "257");
}

TEST(PsiPirProtocolTest, OneHundredTwentyEightBitPirUsesCapacity128AcrossBothPublicApis)
{
    const auto he = ConfigureProtocol();
    std::vector<Element> elements;
    for (std::size_t i = 0; i < 129; ++i) {
        elements.push_back(Make128BitElement(i + 1, std::to_string(i + 1)));
    }

    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(Make128BitPrivacyTarget(129), elements, PIR_128_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 2U);
    for (const auto &batch : response.fheCipherText) {
        EXPECT_EQ(batch.size(), SLOT_COUNT * sizeof(std::uint64_t));
    }
    ASSERT_TRUE(response.symCipherText.has_value());
    EXPECT_EQ(response.symCipherText->size(), elements.size());

    const auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PIR_128_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "129");

    ASSERT_FALSE(he->compatibleParams.empty());
    ExpectCoreParams(he->compatibleParams.back(), ProtocolMode::PIR, 128, 8, 256);
}

TEST(PsiPirProtocolTest, GetSearchResultDoesNotDependOnHeProvider)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(
        privacySearch(MakePrivacyTarget(2), {MakeElement(1, "one"), MakeElement(2, "two")}, PIR_PROTOCOL, response),
        PSI_PIR_SUCCESS);
    const auto decrypted = DecryptOutsideProtocol(response);

    SetHEEngine(nullptr);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    ASSERT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    ASSERT_EQ(result.attachedValues->size(), 1U);
    EXPECT_EQ(std::string(result.attachedValues->front().begin(), result.attachedValues->front().end()), "two");
}

TEST(PsiPirProtocolTest, PsiGetSearchResultNeedsNoHeEngine)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(2), {MakeElement(1), MakeElement(2)}, PSI_PROTOCOL, response),
              PSI_PIR_SUCCESS);
    const auto decrypted = DecryptOutsideProtocol(response);

    SetHEEngine(nullptr);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PsiPirProtocolTest, GetSearchResultRejectsMalformedDecryptedStructure)
{
    SetHEEngine(nullptr);

    const auto expectCodecError = [](const PrivacySearchResult &input, const PrivacyProtocol &protocol) {
        SearchResult result;
        EXPECT_EQ(ProcessDecryptedSearchResult(input, protocol, result), PSI_PIR_CODEC_ERROR);
    };

    PrivacySearchResult malformed;
    malformed.fheCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT - 1, 1)));
    expectCodecError(malformed, PSI_PROTOCOL);

    malformed = {};
    malformed.fheCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.symCipherText.emplace();
    expectCodecError(malformed, PSI_PROTOCOL);

    malformed = {};
    malformed.fheCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    expectCodecError(malformed, PIR_PROTOCOL);

    malformed = {};
    malformed.fheCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.symCipherText.emplace();
    expectCodecError(malformed, PIR_PROTOCOL);

    malformed = {};
    malformed.fheCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.symCipherText = std::vector<Bytes>{Bytes{}};
    expectCodecError(malformed, PIR_PROTOCOL);

    malformed = {};
    malformed.fheCipherText.push_back(SerializeSlots(Slots(SLOT_COUNT, 1)));
    malformed.symCipherText = std::vector<Bytes>(257, Bytes{1});
    expectCodecError(malformed, PIR_PROTOCOL);
}

TEST(PsiPirProtocolTest, PlaintextCodecDecodesLittleEndianUint64Slots)
{
    Bytes plaintext(SLOT_COUNT * sizeof(std::uint64_t), 0);
    const Bytes firstSlot{0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    std::copy(firstSlot.begin(), firstSlot.end(), plaintext.begin());

    PrivacySearchResult input;
    input.fheCipherText.push_back(std::move(plaintext));
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PSI_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    ProtocolCodec codec;
    std::vector<Slots> parsed;
    ASSERT_EQ(codec.ParsePlaintexts(input, mapped, parsed), PSI_PIR_SUCCESS);
    ASSERT_EQ(parsed.size(), 1U);
    ASSERT_EQ(parsed.front().size(), SLOT_COUNT);
    EXPECT_EQ(parsed.front().front(), std::uint64_t{0x0102030405060708ULL});
}

TEST(PsiPirProtocolTest, FailedPlaintextParsingDoesNotModifyOutput)
{
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PSI_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    ProtocolCodec codec;
    std::vector<Slots> parsed{Slots{7, 8, 9}};
    const auto original = parsed;
    PrivacySearchResult malformed;
    malformed.fheCipherText.push_back(Bytes{1});

    EXPECT_EQ(codec.ParsePlaintexts(malformed, mapped, parsed), PSI_PIR_CODEC_ERROR);
    EXPECT_EQ(parsed, original);
}

TEST(PsiPirProtocolTest, NonZeroQueryDoesNotMatchZeroPaddedPsiCapacity)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(9), {MakeElement(1)}, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);
    ASSERT_EQ(response.fheCipherText.size(), 1U);

    const auto decrypted = DecryptOutsideProtocol(response);
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(decrypted, PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PsiPirProtocolTest, ZeroQueryShowsWhyPsiPaddingIsAnInvalidProtocolInput)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakeZeroPrivacyTarget(), {MakeElement(1)}, PSI_PROTOCOL, response), PSI_PIR_SUCCESS);

    // With no item count in the northbound DTO, getSearchResult scans all 512 groups.
    // A zero query makes every zero-padded group decrypt to zero, so the wire
    // result cannot distinguish such padding from a real match. Target
    // generation must therefore reject the all-zero comparison vector.
    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PSI_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_TRUE(result.matchedResult);
}

TEST(PsiPirProtocolTest, NonZeroQueryDoesNotMatchZeroPaddedPirCapacity)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakePrivacyTarget(9), {MakeElement(1, "one")}, PIR_PROTOCOL, response), PSI_PIR_SUCCESS);

    SearchResult result;
    ASSERT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PIR_PROTOCOL, result), PSI_PIR_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

TEST(PsiPirProtocolTest, ZeroQueryPirPaddingCannotIndexPastSymmetricCiphertexts)
{
    ConfigureProtocol();
    PrivacySearchResult response;
    ASSERT_EQ(privacySearch(MakeZeroPrivacyTarget(), {MakeElement(1, "one")}, PIR_PROTOCOL, response), PSI_PIR_SUCCESS);

    SearchResult result;
    EXPECT_EQ(ProcessDecryptedSearchResult(DecryptOutsideProtocol(response), PIR_PROTOCOL, result),
              PSI_PIR_CODEC_ERROR);
}

TEST(PsiPirProtocolTest, SeedPackingRoundTripsWithFixedPirPreset)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    const Bytes seed{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(seed, mapped, encoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(encoded, Slots({0xcdef, 0x89ab, 0x4567, 0x0123}));
    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, mapped, decoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(decoded, seed);
}

TEST(PsiPirProtocolTest, SeedPackingRoundTripsAllSixteenBytesFor128BitPir)
{
    ProtocolEncoder encoder;
    ProtocolParams mapped;
    ASSERT_EQ(MapPrivacyProtocol(PIR_128_PROTOCOL, mapped), PSI_PIR_SUCCESS);
    const Bytes seed{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(seed, mapped, encoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(encoded, Slots({0xeeff, 0xccdd, 0xaabb, 0x8899, 0x6677, 0x4455, 0x2233, 0x0011}));
    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, mapped, decoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(decoded, seed);
}

TEST(PsiPirProtocolTest, SeedPackingSupportsSlotsThatAreNotByteAligned)
{
    ProtocolEncoder encoder;
    ProtocolParams params;
    params.compareBits = 72;
    params.bitsPerSlot = 12;
    params.slotsPerData = 6;
    params.plainModulus = 4097;

    const Bytes input{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(input, params, encoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(encoded, Slots({0x012, 0xdef, 0xabc, 0x789, 0x456, 0x123}));

    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, params, decoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(decoded, input);
}

TEST(PsiPirProtocolTest, SeedPackingSupportsTenBitSlotsAcrossByteBoundaries)
{
    ProtocolEncoder encoder;
    ProtocolParams params;
    params.compareBits = 80;
    params.bitsPerSlot = 10;
    params.slotsPerData = 8;
    params.plainModulus = 1025;

    const Bytes input{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23};
    Slots encoded;
    ASSERT_EQ(encoder.EncodeSeed(input, params, encoded), PSI_PIR_SUCCESS);

    Bytes decoded;
    ASSERT_EQ(encoder.ExtractSeed(encoded, 0, params, decoded), PSI_PIR_SUCCESS);
    EXPECT_EQ(decoded, input);
}

} // namespace
