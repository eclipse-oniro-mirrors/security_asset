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

#include "openssl_crypto_provider.h"
#include "privacy_protocol_impl.h"
#include "provider_registry.h"
#include "soft_he_engine.h"
#include "../mock/openssl_mock.h"
#include <gtest/gtest.h>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace OHOS::Security::PrivacyComputation::internal;
using namespace PrivacyProtocolTest;

class ScopedFault final {
public:
    explicit ScopedFault(CryptoCall call, uint32_t occurrence = 1, FaultResult result = FaultResult::ERROR)
    {
        g_cryptoFault = {call, occurrence, result};
    }

    ScopedFault(size_t capacity, size_t size, uint32_t occurrence)
    {
        g_cryptoFault = {CryptoCall::COPY, occurrence, FaultResult::ERROR, capacity, size};
    }

    ~ScopedFault()
    {
        EXPECT_EQ(g_cryptoFault.remaining, 0U) << static_cast<uint32_t>(g_cryptoFault.call);
        g_cryptoFault = {};
    }
};

struct FailedCall {
    CryptoCall call;
    uint32_t occurrence = 1;
    FaultResult result = FaultResult::ERROR;
    int32_t status = PRIVACY_PROTOCOL_CRYPTO_ERROR;
};

const ProtocolParams PIR_PARAMS{ProtocolMode::PIR, kDefaultCompareBits, kDefaultBitsPerSlot, kDefaultSlotsPerData,
    kDefaultDataNumberPerPlaintext, kDefaultPlainModulus, kDefaultPolyModulusDegree, FheAlgorithm::BGV2048_48,
    256, SymAlgorithm::AES_GCM};
const Bytes SEED(kDefaultCompareBits / kBitsPerByte, 1);
const Bytes VALUE{1, 2, 3};
const Bytes SENTINEL{0x99};

class PrivacyCryptoFaultTest : public testing::Test {
public:
    void SetUp() override
    {
        crypto_ = GetOpenSslPirValueCrypto();
        ASSERT_NE(crypto_, nullptr);
        ASSERT_EQ(crypto_->Encrypt(SEED, VALUE, PIR_PARAMS, ciphertext_), PRIVACY_PROTOCOL_SUCCESS);
    }

    std::shared_ptr<const IPirValueCrypto> crypto_;
    Bytes ciphertext_;
};

/*
 * @tc.name: EncryptFailuresPreserveOutput
 * @tc.desc: Reject failures in frame preparation, HKDF and authenticated encryption without publishing output.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyCryptoFaultTest, EncryptFailuresPreserveOutput, TestSize.Level2)
{
    const FailedCall failures[]{
        {CryptoCall::COPY}, {CryptoCall::COPY, 2}, {CryptoCall::COPY, 3},
        {CryptoCall::RANDOM, 1, FaultResult::ERROR, PRIVACY_PROTOCOL_RANDOM_ERROR},
        {CryptoCall::NEW_PKEY_CTX}, {CryptoCall::DERIVE_INIT}, {CryptoCall::DERIVE},
        {CryptoCall::DERIVE, 1, FaultResult::WRONG_LENGTH},
        {CryptoCall::HKDF_PARAMETER}, {CryptoCall::HKDF_PARAMETER, 2}, {CryptoCall::HKDF_PARAMETER, 3},
        {CryptoCall::HKDF_PARAMETER, 4}, {CryptoCall::HKDF_PARAMETER, 5},
        {CryptoCall::NEW_CIPHER_CTX}, {CryptoCall::ENCRYPT_INIT}, {CryptoCall::ENCRYPT_INIT, 2},
        {CryptoCall::CIPHER_CTRL}, {CryptoCall::CIPHER_CTRL, 2},
        {CryptoCall::ENCRYPT_UPDATE}, {CryptoCall::ENCRYPT_UPDATE, 2},
        {CryptoCall::ENCRYPT_FINAL}, {CryptoCall::ENCRYPT_FINAL, 1, FaultResult::WRONG_LENGTH},
    };
    for (const auto &failure : failures) {
        SCOPED_TRACE(static_cast<uint32_t>(failure.call));
        SCOPED_TRACE(failure.occurrence);
        auto output = SENTINEL;
        const ScopedFault fault(failure.call, failure.occurrence, failure.result);
        EXPECT_EQ(crypto_->Encrypt(SEED, VALUE, PIR_PARAMS, output), failure.status);
        EXPECT_EQ(output, SENTINEL);
    }
}

/*
 * @tc.name: DecryptFailuresPreserveOutput
 * @tc.desc: Reject failures in HKDF and authenticated decryption without publishing plaintext.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyCryptoFaultTest, DecryptFailuresPreserveOutput, TestSize.Level2)
{
    const FailedCall failures[]{
        {CryptoCall::DERIVE}, {CryptoCall::NEW_CIPHER_CTX}, {CryptoCall::DECRYPT_INIT}, {CryptoCall::DECRYPT_INIT, 2},
        {CryptoCall::CIPHER_CTRL}, {CryptoCall::CIPHER_CTRL, 2},
        {CryptoCall::DECRYPT_UPDATE}, {CryptoCall::DECRYPT_UPDATE, 2},
        {CryptoCall::COPY}, {CryptoCall::DECRYPT_FINAL}, {CryptoCall::DECRYPT_FINAL, 1, FaultResult::WRONG_LENGTH},
    };
    for (const auto &failure : failures) {
        SCOPED_TRACE(static_cast<uint32_t>(failure.call));
        SCOPED_TRACE(failure.occurrence);
        std::optional<Bytes> output{SENTINEL};
        const ScopedFault fault(failure.call, failure.occurrence, failure.result);
        EXPECT_EQ(crypto_->Decrypt(SEED, ciphertext_, PIR_PARAMS, output), PRIVACY_PROTOCOL_CRYPTO_ERROR);
        EXPECT_EQ(output, std::optional<Bytes>{SENTINEL});
    }
}

/*
 * @tc.name: DigestAndRandomFailuresPreserveOutput
 * @tc.desc: Reject failed or incomplete digest output and failed random generation without modifying the caller buffer.
 * @tc.type: FUNC
 */
HWTEST(PrivacyCryptoBackendTest, DigestAndRandomFailuresPreserveOutput, TestSize.Level2)
{
    const auto hash = GetOpenSslHashProvider();
    const auto random = GetOpenSslRandomSource();
    ASSERT_NE(hash, nullptr);
    ASSERT_NE(random, nullptr);
    for (const auto result : {FaultResult::ERROR, FaultResult::WRONG_LENGTH}) {
        auto output = SENTINEL;
        const ScopedFault fault(CryptoCall::DIGEST, 1, result);
        EXPECT_EQ(hash->Digest(HashAlg::SHA256, VALUE, output), PRIVACY_PROTOCOL_HASH_ERROR);
        EXPECT_EQ(output, SENTINEL);
    }
    auto output = SENTINEL;
    const ScopedFault fault(CryptoCall::RANDOM);
    EXPECT_EQ(random->Fill(output), PRIVACY_PROTOCOL_RANDOM_ERROR);
    EXPECT_EQ(output, SENTINEL);
}

/*
 * @tc.name: CoreRejectsInvalidParametersAndInputs
 * @tc.desc: Reject invalid internal protocol parameters and malformed inputs before touching the outputs.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolErrorTest, CoreRejectsInvalidParametersAndInputs, TestSize.Level2)
{
    const PrivacyProtocolImpl impl;
    const ProtocolParams defaults;
    auto invalid = defaults;
    invalid.bitsPerSlot = 0;
    const CallerIdentity identity{20010001, 100};
    constexpr auto INVALID = PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    Bytes bytes = SENTINEL;
    const TargetInput target{VALUE, HashAlg::SHA256};
    EXPECT_EQ(impl.GenPrivacyTarget(target, invalid, identity, bytes), INVALID);
    EXPECT_EQ(impl.GenPrivacyTarget({}, defaults, identity, bytes), INVALID);
    const TargetInput invalidHash{VALUE, static_cast<HashAlg>(UINT32_MAX)};
    EXPECT_EQ(impl.GenPrivacyTarget(invalidHash, defaults, identity, bytes), INVALID);
    const TargetInput zeroTarget{Bytes(defaults.compareBits / kBitsPerByte, 0), HashAlg::NONE};
    EXPECT_EQ(impl.GenPrivacyTarget(zeroTarget, defaults, identity, bytes), INVALID);
    EXPECT_EQ(impl.DecryptCiphertext(VALUE, invalid, identity, bytes), INVALID);
    EXPECT_EQ(impl.DecryptCiphertext({}, defaults, identity, bytes), INVALID);
    EXPECT_EQ(bytes, SENTINEL);

    PrivacySearchResult search{{SENTINEL}, std::nullopt};
    EXPECT_EQ(impl.PrivacySearch(VALUE, {}, invalid, search), INVALID);
    EXPECT_EQ(search.resultCipherText, std::vector<Bytes>{SENTINEL});
    SearchResult result{true, std::vector<Bytes>{SENTINEL}};
    EXPECT_EQ(impl.PostProcessSearchResult({}, std::nullopt, invalid, result), INVALID);
    EXPECT_TRUE(result.matchedResult);
    EXPECT_EQ(result.attachedValues, (std::optional<std::vector<Bytes>>{{SENTINEL}}));
}

/*
 * @tc.name: EvaluationPreservesOutputOnSlotCopyFailure
 * @tc.desc: Propagate failures when copying server or PIR seed slots without committing the result.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolErrorTest, EvaluationPreservesOutputOnSlotCopyFailure, TestSize.Level2)
{
    const HeSeed seed{};
    Bytes target;
    ASSERT_EQ(SoftHeEngine().Encrypt(Slots(PIR_PARAMS.polyModulusDegree, 1), PIR_PARAMS, seed, target),
        PRIVACY_PROTOCOL_SUCCESS);
    SetHEEngine(nullptr);
    const std::vector<Element> elements{{VALUE, HashAlg::SHA256, VALUE}};
    const PrivacySearchResult sentinel{{SENTINEL}, std::nullopt};
    auto output = sentinel;
    const size_t serverCapacity = PIR_PARAMS.polyModulusDegree * sizeof(uint64_t);
    const size_t seedCapacity = (PIR_PARAMS.polyModulusDegree - PIR_PARAMS.slotsPerData) * sizeof(uint64_t);
    for (const size_t capacity : {serverCapacity, seedCapacity}) {
        // The smaller capacity first occurs in the second copy of the server vector.
        uint32_t occurrence = 1;
        if (capacity == seedCapacity) {
            occurrence = 2;
        }
        const ScopedFault fault(capacity, PIR_PARAMS.slotsPerData * sizeof(uint64_t), occurrence);
        EXPECT_EQ(PrivacyProtocolImpl().PrivacySearch(target, elements, PIR_PARAMS, output),
            PRIVACY_PROTOCOL_CODEC_ERROR);
        EXPECT_EQ(output.resultCipherText, sentinel.resultCipherText);
        EXPECT_EQ(output.valueCipherText, sentinel.valueCipherText);
    }
}
} // namespace
