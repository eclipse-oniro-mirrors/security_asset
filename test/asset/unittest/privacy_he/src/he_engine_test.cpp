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

#include "crypto_provider.h"
#include "protocol_evaluation.h"
#include "psi_pir_error_code.h"
#include <gtest/gtest.h>

namespace {
using namespace testing::ext;
using namespace psi_pir;
using namespace psi_pir::internal;

enum class Operation {
    SUBTRACT_FROM_PLAIN,
    MULTIPLY_PLAIN,
    RERANDOMIZE,
};

class RecordingEngine final : public IHEEngine {
public:
    bool IsCompatible(const ProtocolParams &) const noexcept override
    {
        return true;
    }
    int32_t AddPlain(const Bytes &input, const Slots &, Bytes &output) const noexcept override
    {
        return Record(Operation::SUBTRACT_FROM_PLAIN, input, output);
    }
    int32_t MultiplyPlain(const Bytes &input, const Slots &, Bytes &output) const noexcept override
    {
        return Record(Operation::MULTIPLY_PLAIN, input, output);
    }
    int32_t RandCipherText(const Bytes &input, Bytes &output) const noexcept override
    {
        return Record(Operation::RERANDOMIZE, input, output);
    }
    mutable std::vector<Operation> calls;
    size_t failAt = 0;
    bool emptyResult = false;

private:
    int32_t Record(Operation type, const Bytes &input, Bytes &output) const noexcept
    {
        calls.push_back(type);
        output = input;
        output.push_back(static_cast<uint8_t>(calls.size()));
        if (emptyResult) {
            output.clear();
        }
        return calls.size() == failAt ? PSI_PIR_CRYPTO_ERROR : PSI_PIR_SUCCESS;
    }
};

/* @tc.name: HandleKeepsOperationOrder
 * @tc.desc: Reuse one opaque handle across subtraction, masking, seed injection and rerandomization.
 * @tc.type: FUNC
 */
HWTEST(HeEngineTest, HandleKeepsOperationOrder, TestSize.Level1)
{
    RecordingEngine engine;
    const Bytes input{0xAA};
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(CreateHeEvaluation(engine, input, evaluation), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->AddPlain({10}), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->MultiplyPlain({3}), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->AddPlain({4}), PSI_PIR_SUCCESS);
    ASSERT_EQ(evaluation->Rerandomize(), PSI_PIR_SUCCESS);
    Bytes output;
    ASSERT_EQ(evaluation->Serialize(output), PSI_PIR_SUCCESS);
    EXPECT_EQ(output, Bytes({0xAA, 1, 2, 3, 4}));
    EXPECT_EQ(input, Bytes({0xAA}));
    EXPECT_EQ(engine.calls, (std::vector<Operation>{Operation::SUBTRACT_FROM_PLAIN, Operation::MULTIPLY_PLAIN,
                                                    Operation::SUBTRACT_FROM_PLAIN, Operation::RERANDOMIZE}));
}

/* @tc.name: FailedHandleCannotContinueOrSerialize
 * @tc.desc: A provider failure at any stage invalidates the handle without committing a partial result.
 * @tc.type: FUNC
 */
HWTEST(HeEngineTest, FailedHandleCannotContinueOrSerialize, TestSize.Level2)
{
    constexpr size_t OPERATION_COUNT = 4;
    for (size_t failAt = 1; failAt <= OPERATION_COUNT; ++failAt) {
        RecordingEngine engine;
        engine.failAt = failAt;
        std::unique_ptr<HeEvaluation> evaluation;
        ASSERT_EQ(CreateHeEvaluation(engine, {0xAA}, evaluation), PSI_PIR_SUCCESS);
        EXPECT_EQ(evaluation->AddPlain({10}), failAt == 1 ? PSI_PIR_CRYPTO_ERROR : PSI_PIR_SUCCESS);
        EXPECT_EQ(evaluation->MultiplyPlain({3}), failAt <= 2 ? PSI_PIR_CRYPTO_ERROR : PSI_PIR_SUCCESS);
        EXPECT_EQ(evaluation->AddPlain({4}), failAt <= 3 ? PSI_PIR_CRYPTO_ERROR : PSI_PIR_SUCCESS);
        EXPECT_EQ(evaluation->Rerandomize(), PSI_PIR_CRYPTO_ERROR);
        EXPECT_EQ(evaluation->AddPlain({5}), PSI_PIR_CRYPTO_ERROR);
        Bytes output{0x99};
        EXPECT_EQ(evaluation->Serialize(output), PSI_PIR_CRYPTO_ERROR);
        EXPECT_EQ(engine.calls.size(), failAt);
        EXPECT_EQ(output, Bytes({0x99}));
    }
}

/* @tc.name: EmptyResultInvalidatesHandle
 * @tc.desc: Provider success without a ciphertext cannot feed a later operation or be exported.
 * @tc.type: FUNC
 */
HWTEST(HeEngineTest, EmptyResultInvalidatesHandle, TestSize.Level2)
{
    RecordingEngine engine;
    engine.emptyResult = true;
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(CreateHeEvaluation(engine, {0xAA}, evaluation), PSI_PIR_SUCCESS);
    EXPECT_EQ(evaluation->AddPlain({10}), PSI_PIR_HE_ERROR);
    EXPECT_EQ(evaluation->MultiplyPlain({3}), PSI_PIR_HE_ERROR);
    Bytes output{0x99};
    EXPECT_EQ(evaluation->Serialize(output), PSI_PIR_HE_ERROR);
    EXPECT_EQ(engine.calls.size(), 1U);
    EXPECT_EQ(output, Bytes({0x99}));
}

/* @tc.name: FailedImportPreservesExistingHandle
 * @tc.desc: Invalid input must not replace an existing usable handle or invoke provider arithmetic.
 * @tc.type: FUNC
 */
HWTEST(HeEngineTest, FailedImportPreservesExistingHandle, TestSize.Level2)
{
    RecordingEngine engine;
    std::unique_ptr<HeEvaluation> evaluation;
    ASSERT_EQ(CreateHeEvaluation(engine, {0xAA}, evaluation), PSI_PIR_SUCCESS);
    const auto *original = evaluation.get();
    EXPECT_EQ(CreateHeEvaluation(engine, {}, evaluation), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(evaluation.get(), original);
    Bytes output;
    EXPECT_EQ(evaluation->Serialize(output), PSI_PIR_SUCCESS);
    EXPECT_EQ(output, Bytes({0xAA}));
    EXPECT_TRUE(engine.calls.empty());
}
} // namespace
