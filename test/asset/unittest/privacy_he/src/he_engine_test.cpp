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

#include "soft_he_engine.h"
#include "privacy_error_code.h"
#include <gtest/gtest.h>

namespace {
using namespace testing::ext;
using namespace ::PrivacyComputation;
using namespace ::PrivacyComputation::internal;

constexpr uint64_t INPUT_SLOT_VALUE = 7;

class HeEngineTest : public testing::Test {
public:
    void SetUp() override
    {
        seed_.bytes.fill(0x45);
        ASSERT_EQ(SoftHeEngine().Encrypt(Slots(kDefaultPolyModulusDegree, INPUT_SLOT_VALUE), ProtocolParams{}, seed_,
                                         target_),
                  PRIVACY_PROTOCOL_SUCCESS);
    }
    HeSeed seed_;
    Bytes target_;
};

/* @tc.name: HandleKeepsOperationOrderAndOutlivesEngine
 * @tc.desc: A batch owns its context and preserves subtraction, masking, seed and rerandomization order.
 * @tc.type: FUNC
 */
HWTEST_F(HeEngineTest, HandleKeepsOperationOrderAndOutlivesEngine, TestSize.Level1)
{
    std::unique_ptr<HeEvaluation> evaluation;
    {
        const SoftHeEngine engine;
        ASSERT_EQ(engine.CreateEvaluation(target_, evaluation), PRIVACY_PROTOCOL_SUCCESS);
    }
    ASSERT_EQ(evaluation->AddPlain(Slots(kDefaultPolyModulusDegree, 10)), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(evaluation->MultiplyPlain(Slots(kDefaultPolyModulusDegree, 3)), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(evaluation->AddPlain(Slots(kDefaultPolyModulusDegree, 20)), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(evaluation->Rerandomize(), PRIVACY_PROTOCOL_SUCCESS);
    Bytes output;
    ASSERT_EQ(evaluation->Serialize(output), PRIVACY_PROTOCOL_SUCCESS);
    Slots recovered;
    ASSERT_EQ(SoftHeEngine().Decrypt(output, ProtocolParams{}, seed_, recovered), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(recovered, Slots(kDefaultPolyModulusDegree, 11)); // 20 - (10 - 7) * 3.
}

/* @tc.name: FailedHandleCannotContinueOrSerialize
 * @tc.desc: Invalid plaintext invalidates the batch and preserves the caller's output and input.
 * @tc.type: FUNC
 */
HWTEST_F(HeEngineTest, FailedHandleCannotContinueOrSerialize, TestSize.Level2)
{
    const Bytes original = target_;
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(SoftHeEngine().CreateEvaluation(target_, evaluation), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(evaluation->MultiplyPlain(Slots(kDefaultPolyModulusDegree - 1)), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(evaluation->AddPlain(Slots(kDefaultPolyModulusDegree, 10)), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(evaluation->Rerandomize(), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    Bytes output{0x99};
    EXPECT_EQ(evaluation->Serialize(output), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(output, (Bytes{0x99}));
    EXPECT_EQ(target_, original);
}

/* @tc.name: FailedImportPreservesExistingHandle
 * @tc.desc: Empty or malformed input cannot replace an existing usable batch handle.
 * @tc.type: FUNC
 */
HWTEST_F(HeEngineTest, FailedImportPreservesExistingHandle, TestSize.Level2)
{
    const SoftHeEngine engine;
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(engine.CreateEvaluation(target_, evaluation), PRIVACY_PROTOCOL_SUCCESS);
    const auto *original = evaluation.get();
    EXPECT_EQ(engine.CreateEvaluation({}, evaluation), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(evaluation.get(), original);
    EXPECT_EQ(engine.CreateEvaluation({1}, evaluation), PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(evaluation.get(), original);
    ASSERT_EQ(evaluation->AddPlain(Slots(kDefaultPolyModulusDegree, 10)), PRIVACY_PROTOCOL_SUCCESS);
    Bytes output;
    ASSERT_EQ(evaluation->Serialize(output), PRIVACY_PROTOCOL_SUCCESS);
    Slots recovered;
    ASSERT_EQ(SoftHeEngine().Decrypt(output, ProtocolParams{}, seed_, recovered), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(recovered, Slots(kDefaultPolyModulusDegree, 3));
}

/* @tc.name: SeparateHandlesKeepIndependentCiphertexts
 * @tc.desc: Two batches from the same engine retain independent contexts and intermediate results.
 * @tc.type: FUNC
 */
HWTEST_F(HeEngineTest, SeparateHandlesKeepIndependentCiphertexts, TestSize.Level1)
{
    const SoftHeEngine engine;
    std::unique_ptr<HeEvaluation> first;
    std::unique_ptr<HeEvaluation> second;
    ASSERT_EQ(engine.CreateEvaluation(target_, first), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(engine.CreateEvaluation(target_, second), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(first->AddPlain(Slots(kDefaultPolyModulusDegree, 10)), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(second->MultiplyPlain(Slots(kDefaultPolyModulusDegree, 3)), PRIVACY_PROTOCOL_SUCCESS);
    Bytes output;
    Slots recovered;
    ASSERT_EQ(first->Serialize(output), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(engine.Decrypt(output, ProtocolParams{}, seed_, recovered), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(recovered, Slots(kDefaultPolyModulusDegree, 3));
    ASSERT_EQ(second->Serialize(output), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(engine.Decrypt(output, ProtocolParams{}, seed_, recovered), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(recovered, Slots(kDefaultPolyModulusDegree, 21));
}
} // namespace
