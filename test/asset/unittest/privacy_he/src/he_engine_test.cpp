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
#include "../mock/hifhe_api_mock.h"
#include "privacy_error_code.h"
#include "protocol_mapper.h"
#include <gtest/gtest.h>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace OHOS::Security::PrivacyComputation::internal;

using PrivacyHeTest::HeOperation;
using PrivacyHeTest::g_heFault;

class ScopedHeFault {
public:
    ScopedHeFault(HeOperation operation, int32_t status)
    {
        g_heFault = {operation, status};
    }

    ~ScopedHeFault()
    {
        EXPECT_EQ(g_heFault.operation, HeOperation::NONE);
        g_heFault = {};
    }
};

constexpr uint64_t INPUT_SLOT_VALUE = 7;

class HeEngineTest : public testing::Test {
public:
    void SetUp() override
    {
        seed_.bytes.fill(0x45);
        ASSERT_EQ(SoftHeEngine().Encrypt(Slots(kDefaultPolyModulusDegree, INPUT_SLOT_VALUE),
            ProtocolParams{}, seed_, target_), PRIVACY_PROTOCOL_SUCCESS);
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

/*
 * @tc.name: RejectsHiFheFailuresAndMissingHandles
 * @tc.desc: Preserve output when HiFHE fails or reports success without a handle.
 * @tc.type: FUNC
 */
HWTEST_F(HeEngineTest, RejectsHiFheFailuresAndMissingHandles, TestSize.Level2)
{
    const ProtocolParams params;
    const SoftHeEngine engine;
    const Slots slots(params.polyModulusDegree, INPUT_SLOT_VALUE);
    const Bytes sentinel{0x99};
    const Slots sentinelSlots{0x99};
    for (const int32_t status : {HIFHE_PARAM_INVALID, HIFHE_SUCCESS}) {
        SCOPED_TRACE(status);
        for (const auto operation : {HeOperation::ENCODE, HeOperation::ENCRYPT}) {
            Bytes output = sentinel;
            const ScopedHeFault fault(operation, status);
            EXPECT_EQ(engine.Encrypt(slots, params, seed_, output), PRIVACY_PROTOCOL_HE_ERROR);
            EXPECT_EQ(output, sentinel);
        }
        for (const auto operation : {HeOperation::DESERIALIZE, HeOperation::DECRYPT}) {
            Slots output = sentinelSlots;
            const ScopedHeFault fault(operation, status);
            EXPECT_EQ(engine.Decrypt(target_, params, seed_, output), PRIVACY_PROTOCOL_CRYPTO_ERROR);
            EXPECT_EQ(output, sentinelSlots);
        }
    }
}

/*
 * @tc.name: RejectsMissingEvaluationHandles
 * @tc.desc: Reject missing add, multiply or rerandomize outputs and prevent serialization of failed batches.
 * @tc.type: FUNC
 */
HWTEST_F(HeEngineTest, RejectsMissingEvaluationHandles, TestSize.Level2)
{
    const SoftHeEngine engine;
    const Slots slots(kDefaultPolyModulusDegree, INPUT_SLOT_VALUE);
    for (const auto operation : {HeOperation::ADD, HeOperation::MULTIPLY, HeOperation::RERANDOMIZE}) {
        SCOPED_TRACE(static_cast<uint32_t>(operation));
        std::unique_ptr<HeEvaluation> evaluation;
        ASSERT_EQ(engine.CreateEvaluation(target_, evaluation), PRIVACY_PROTOCOL_SUCCESS);
        const ScopedHeFault fault(operation, HIFHE_SUCCESS);
        int32_t ret = PRIVACY_PROTOCOL_SUCCESS;
        if (operation == HeOperation::ADD) {
            ret = evaluation->AddPlain(slots);
        } else if (operation == HeOperation::MULTIPLY) {
            ret = evaluation->MultiplyPlain(slots);
        } else {
            ret = evaluation->Rerandomize();
        }
        EXPECT_EQ(ret, PRIVACY_PROTOCOL_HE_ERROR);
        const Bytes sentinel{0x99};
        Bytes output = sentinel;
        EXPECT_EQ(evaluation->Serialize(output), PRIVACY_PROTOCOL_HE_ERROR);
        EXPECT_EQ(output, sentinel);
    }
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
/*
 * @tc.name: CompatibilityRejectsUnsupportedParameters
 * @tc.desc: Accept every mapped preset and reject changes the native engine cannot support.
 * @tc.type: FUNC
 */
HWTEST(HeEngineCompatibilityTest, CompatibilityRejectsUnsupportedParameters, TestSize.Level2)
{
    const SoftHeEngine engine;
    const ProtocolParams defaults;
    ASSERT_TRUE(engine.IsCompatible(defaults));
    for (const auto field : {&ProtocolParams::compareBits, &ProtocolParams::slotsPerData,
                             &ProtocolParams::dataNumPerPlaintext, &ProtocolParams::bitsPerSlot,
                             &ProtocolParams::polyModulusDegree}) {
        auto params = defaults;
        ++(params.*field);
        EXPECT_FALSE(engine.IsCompatible(params));
    }
    auto params = defaults;
    ++params.plainModulus;
    EXPECT_FALSE(engine.IsCompatible(params));
    params = defaults;
    params.fheAlgorithm = static_cast<FheAlgorithm>(UINT32_MAX);
    EXPECT_FALSE(engine.IsCompatible(params));
    params = defaults;
    params.mode = static_cast<ProtocolMode>(UINT32_MAX);
    EXPECT_FALSE(engine.IsCompatible(params));

    for (const auto size : {DataSetSize::SIZE_128, DataSetSize::SIZE_256}) {
        ASSERT_EQ(MapPrivacyProtocol({size, ProtocolType::PIR_PROTOCOL}, params), PRIVACY_PROTOCOL_SUCCESS);
        EXPECT_TRUE(engine.IsCompatible(params));
        ++params.slotsPerData;
        EXPECT_FALSE(engine.IsCompatible(params));
    }
    ASSERT_EQ(MapPrivacyProtocol({DataSetSize::SIZE_128, ProtocolType::PIR_PROTOCOL}, params),
        PRIVACY_PROTOCOL_SUCCESS);
    ++params.dataNumPerPlaintext;
    EXPECT_FALSE(engine.IsCompatible(params));
}
} // namespace
