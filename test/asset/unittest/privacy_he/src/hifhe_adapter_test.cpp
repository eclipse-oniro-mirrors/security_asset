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

#include "he_key_deriver.h"
#include "hifhe_adapter.h"
#include "protocol_evaluation.h"
#include "provider_registry.h"
#include "psi_pir_error_code.h"
#include "psi_pir_protocol.h"
#include "soft_he_engine.h"
#include <gtest/gtest.h>
#include <limits>

namespace {
using namespace testing::ext;
using namespace psi_pir;
using namespace psi_pir::internal;

Slots Pattern()
{
    Slots slots(kDefaultPolyModulusDegree);
    for (size_t i = 0; i < slots.size(); ++i) {
        slots[i] = i % kDefaultPlainModulus;
    }
    slots.back() = kDefaultPlainModulus - 1;
    return slots;
}

/* @tc.name: RecreatedContextRecoversKey
 * @tc.desc: Recover the same HE key in a fresh context and decrypt a serialized ciphertext.
 * @tc.type: FUNC
 */
HWTEST(HifheAdapterTest, RecreatedContextRecoversKey, TestSize.Level1)
{
    HeSeed seed;
    seed.bytes.fill(0x45);
    const auto slots = Pattern();
    Bytes ciphertext;
    ASSERT_EQ(SoftHeEngine().Encrypt(slots, ProtocolParams{}, seed, ciphertext), PSI_PIR_SUCCESS);
    Slots recovered;
    ASSERT_EQ(SoftHeEngine().Decrypt(ciphertext, ProtocolParams{}, seed, recovered), PSI_PIR_SUCCESS);
    EXPECT_EQ(recovered, slots);
}

/* @tc.name: KeylessEvaluationMatchesArithmetic
 * @tc.desc: A context without keys computes mask * (plaintext - ciphertext), then rerandomizes.
 * @tc.type: FUNC
 */
HWTEST(HifheAdapterTest, KeylessEvaluationMatchesArithmetic, TestSize.Level1)
{
    HeSeed seed;
    seed.bytes.fill(0x45);
    const auto query = Pattern();
    Slots bucket(query.size());
    Slots mask(query.size(), 3);
    for (size_t i = 0; i < bucket.size(); ++i) {
        bucket[i] = (query[i] + i % 4) % kDefaultPlainModulus;
    }
    HifheAdapter owner;
    ASSERT_EQ(owner.InitializeKeyed(ProtocolParams{}, seed), PSI_PIR_SUCCESS);
    HifheText plaintext;
    HifheText ciphertext;
    Bytes target;
    ASSERT_EQ(owner.Encode(query, plaintext), PSI_PIR_SUCCESS);
    ASSERT_EQ(owner.Encrypt(plaintext, ciphertext), PSI_PIR_SUCCESS);
    ASSERT_EQ(owner.Serialize(ciphertext, target), PSI_PIR_SUCCESS);
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(GetSoftHeEngine()->CreateEvaluation(target, evaluation), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->AddPlain(bucket), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->MultiplyPlain(mask), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->Rerandomize(), PSI_PIR_SUCCESS);
    Bytes result;
    ASSERT_EQ(evaluation->Serialize(result), PSI_PIR_SUCCESS);
    Slots recovered;
    ASSERT_EQ(owner.Deserialize(result, ciphertext), PSI_PIR_SUCCESS);
    ASSERT_EQ(owner.Decrypt(ciphertext, plaintext), PSI_PIR_SUCCESS);
    ASSERT_EQ(owner.Decode(plaintext, recovered), PSI_PIR_SUCCESS);
    ASSERT_EQ(recovered.size(), query.size());
    for (size_t i = 0; i < recovered.size(); ++i) {
        EXPECT_EQ(recovered[i], ((bucket[i] + kDefaultPlainModulus - query[i]) * mask[i]) % kDefaultPlainModulus);
    }
}

/* @tc.name: BatchFailurePreservesSerializedInputAndOutput
 * @tc.desc: A later operator failure cannot alter the source ciphertext or commit a partial serialized result.
 * @tc.type: FUNC
 */
HWTEST(HifheAdapterTest, BatchFailurePreservesSerializedInputAndOutput, TestSize.Level2)
{
    HeSeed seed;
    seed.bytes.fill(0x46);
    HifheAdapter source;
    ASSERT_EQ(source.InitializeKeyed(ProtocolParams{}, seed), PSI_PIR_SUCCESS);
    HifheText plaintext;
    HifheText ciphertext;
    ASSERT_EQ(source.Encode(Pattern(), plaintext), PSI_PIR_SUCCESS);
    ASSERT_EQ(source.Encrypt(plaintext, ciphertext), PSI_PIR_SUCCESS);
    Bytes target;
    ASSERT_EQ(source.Serialize(ciphertext, target), PSI_PIR_SUCCESS);
    const auto original = target;
    Bytes output{0x99};
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(GetSoftHeEngine()->CreateEvaluation(target, evaluation), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->AddPlain(Pattern()), PSI_PIR_SUCCESS);
    EXPECT_EQ(evaluation->MultiplyPlain(Slots(kDefaultPolyModulusDegree - 1)), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(evaluation->Rerandomize(), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(evaluation->Serialize(output), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(target, original);
    EXPECT_EQ(output, Bytes({0x99}));
}

/* @tc.name: RejectInvalidSlotsAndCiphertexts
 * @tc.desc: Reject malformed data and out-of-range slots while preserving the caller's output.
 * @tc.type: FUNC
 */
HWTEST(HifheAdapterTest, RejectInvalidSlotsAndCiphertexts, TestSize.Level2)
{
    HifheAdapter adapter;
    ASSERT_EQ(adapter.InitializeEvaluation(ProtocolParams{}), PSI_PIR_SUCCESS);
    HifheText output;
    ASSERT_EQ(adapter.Encode(Pattern(), output), PSI_PIR_SUCCESS);
    const auto *original = output.value;
    Slots invalid(kDefaultPolyModulusDegree - 1);
    EXPECT_EQ(adapter.Encode(invalid, output), PSI_PIR_INVALID_ARGUMENT);
    invalid.resize(kDefaultPolyModulusDegree);
    invalid.front() = kDefaultPlainModulus;
    EXPECT_EQ(adapter.Encode(invalid, output), PSI_PIR_INVALID_ARGUMENT);
    invalid.front() = std::numeric_limits<uint64_t>::max();
    EXPECT_EQ(adapter.Encode(invalid, output), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(output.value, original);
    EXPECT_EQ(adapter.Deserialize(Bytes{1}, output), PSI_PIR_HE_ERROR);
    EXPECT_EQ(output.value, original);
    HifheText empty;
    EXPECT_EQ(adapter.Rerandomize(empty, output), PSI_PIR_HE_ERROR);
    EXPECT_EQ(output.value, original);
    Slots slots{0x99};
    EXPECT_EQ(adapter.Decode(empty, slots), PSI_PIR_HE_ERROR);
    EXPECT_EQ(slots, Slots({0x99}));
}

/* @tc.name: EvaluationCannotDecrypt
 * @tc.desc: Evaluation initializes parameters only and never generates a private key.
 * @tc.type: FUNC
 */
HWTEST(HifheAdapterTest, EvaluationCannotDecrypt, TestSize.Level2)
{
    HeSeed seed;
    seed.bytes.fill(0x45);
    Bytes ciphertext;
    ASSERT_EQ(SoftHeEngine().Encrypt(Pattern(), ProtocolParams{}, seed, ciphertext), PSI_PIR_SUCCESS);
    HifheAdapter evaluator;
    ASSERT_EQ(evaluator.InitializeEvaluation(ProtocolParams{}), PSI_PIR_SUCCESS);
    HifheText encrypted;
    HifheText output;
    ASSERT_EQ(evaluator.Deserialize(ciphertext, encrypted), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluator.Encode(Pattern(), output), PSI_PIR_SUCCESS);
    const auto *original = output.value;
    EXPECT_EQ(evaluator.Decrypt(encrypted, output), PSI_PIR_HE_ERROR);
    EXPECT_EQ(output.value, original);
}

/* @tc.name: InvalidInitializationCanRetry
 * @tc.desc: Invalid parameters and repeated initialization do not replace an existing context.
 * @tc.type: FUNC
 */
HWTEST(HifheAdapterTest, InvalidInitializationCanRetry, TestSize.Level2)
{
    HifheAdapter adapter;
    ProtocolParams invalid;
    invalid.plainModulus = 17;
    EXPECT_EQ(adapter.InitializeEvaluation(invalid), PSI_PIR_INVALID_ARGUMENT);
    ASSERT_EQ(adapter.InitializeEvaluation(ProtocolParams{}), PSI_PIR_SUCCESS);
    EXPECT_EQ(adapter.InitializeEvaluation(ProtocolParams{}), PSI_PIR_INVALID_ARGUMENT);
    HifheText plaintext;
    EXPECT_EQ(adapter.Encode(Pattern(), plaintext), PSI_PIR_SUCCESS);
}
// Deterministic test provider exercises protocol orchestration without a device HUKS dependency.
class TestSeedDeriver final : public IHeSeedDeriver {
public:
    int32_t Derive(const CallerIdentity &, const std::array<uint8_t, kIdentityHashSize> &hash,
                   HeSeed &seed) const noexcept override
    {
        seed.bytes = hash;
        return PSI_PIR_SUCCESS;
    }
};

class HifheProtocolTest : public testing::Test {
public:
    void SetUp() override
    {
        SetHeSeedDeriver(std::make_shared<TestSeedDeriver>());
    }
    void TearDown() override
    {
        SetHeSeedDeriver(nullptr);
        SetHEEngine(nullptr);
    }
};

/* @tc.name: PsiCompleteWorkflow
 * @tc.desc: Four public protocol operations preserve hit and miss results with separate HE contexts.
 * @tc.type: FUNC
 */
HWTEST_F(HifheProtocolTest, PsiCompleteWorkflow, TestSize.Level1)
{
    const PrivacyProtocol protocol{DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};
    const CallerIdentity identity{20010001, 100};
    Bytes target;
    ASSERT_EQ(GenPrivacyTarget({Bytes{'q'}, HashAlg::SHA256}, protocol, identity, target), PSI_PIR_SUCCESS);
    for (const uint8_t key : {static_cast<uint8_t>('q'), static_cast<uint8_t>('z')}) {
        PrivacySearchResult encrypted;
        ASSERT_EQ(PrivacySearch(target, {{Bytes{key}, HashAlg::SHA256, std::nullopt}}, protocol, encrypted),
                  PSI_PIR_SUCCESS);
        SearchResult result;
        std::vector<Bytes> plaintexts;
        for (const auto &ciphertext : encrypted.resultCipherText) {
            Bytes plaintext;
            ASSERT_EQ(DecryptCiphertext(ciphertext, protocol, identity, plaintext), PSI_PIR_SUCCESS);
            plaintexts.push_back(std::move(plaintext));
        }
        ASSERT_EQ(PostProcessSearchResult(plaintexts, encrypted.valueCipherText, protocol, result), PSI_PIR_SUCCESS);
        EXPECT_EQ(result.matchedResult, key == 'q');
        EXPECT_FALSE(result.attachedValues.has_value());
    }
}

/* @tc.name: PirCompleteWorkflow
 * @tc.desc: Four public protocol operations recover associated values, including binary and empty values.
 * @tc.type: FUNC
 */
HWTEST_F(HifheProtocolTest, PirCompleteWorkflow, TestSize.Level1)
{
    const PrivacyProtocol protocol{DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL};
    const CallerIdentity identity{20010001, 100};
    Bytes target;
    ASSERT_EQ(GenPrivacyTarget({Bytes{'q'}, HashAlg::SHA256}, protocol, identity, target), PSI_PIR_SUCCESS);
    for (const auto &value : std::vector<Bytes>{{}, {0, 0xFF, 0x45}}) {
        PrivacySearchResult encrypted;
        ASSERT_EQ(PrivacySearch(target, {{Bytes{'q'}, HashAlg::SHA256, value}}, protocol, encrypted), PSI_PIR_SUCCESS);
        SearchResult result;
        std::vector<Bytes> plaintexts;
        for (const auto &ciphertext : encrypted.resultCipherText) {
            Bytes plaintext;
            ASSERT_EQ(DecryptCiphertext(ciphertext, protocol, identity, plaintext), PSI_PIR_SUCCESS);
            plaintexts.push_back(std::move(plaintext));
        }
        ASSERT_EQ(PostProcessSearchResult(plaintexts, encrypted.valueCipherText, protocol, result), PSI_PIR_SUCCESS);
        EXPECT_TRUE(result.matchedResult);
        ASSERT_TRUE(result.attachedValues.has_value());
        EXPECT_EQ(*result.attachedValues, std::vector<Bytes>{value});
    }
}
} // namespace
