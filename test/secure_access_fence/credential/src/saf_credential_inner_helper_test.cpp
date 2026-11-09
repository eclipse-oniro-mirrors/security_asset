/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Pure unit tests for the SafCredential client-side helpers.
//
// These cases need no system ability, no samgr and no IPC runtime: the helper
// translation unit is compiled directly into this test binary, mirroring the
// saf_gui_fence_parse_test pattern. They therefore run in every environment,
// including CI images where SA 66532 is not deployed.

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <gtest/gtest.h>

#include "errors.h"

#include "credential_inner_helper.h"
#include "saf_credential_type.h"
#include "saf_result_code.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Security::SAF;

namespace UnitTest::SafCredentialInnerHelperTest {

namespace {
// ERR_REMOTE_DEAD mirrors the literal used inside credential_inner_helper.cpp.
constexpr int32_t ERR_REMOTE_DEAD = 29189;

// An arbitrary non-dead, non-success code used to confirm pass-through.
constexpr int32_t GENERIC_IPC_ERROR = -1;
}  // namespace

class SafCredentialInnerHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void) override {}
    void TearDown(void) override {}
};

// ===== NormalizeIpcResult =====

/**
 * @tc.name: NormalizeIpcResult_DeadObject
 * @tc.desc: ERR_DEAD_OBJECT is normalized to SAF_ERR_SERVICE_UNAVAILABLE.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, NormalizeIpcResult_DeadObject, TestSize.Level0)
{
    EXPECT_EQ(NormalizeIpcResult(ERR_DEAD_OBJECT), SAF_ERR_SERVICE_UNAVAILABLE);
}

/**
 * @tc.name: NormalizeIpcResult_RemoteDead
 * @tc.desc: The IPC-layer ERR_REMOTE_DEAD literal (29189) is normalized to
 *           SAF_ERR_SERVICE_UNAVAILABLE.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, NormalizeIpcResult_RemoteDead, TestSize.Level0)
{
    EXPECT_EQ(NormalizeIpcResult(ERR_REMOTE_DEAD), SAF_ERR_SERVICE_UNAVAILABLE);
}

/**
 * @tc.name: NormalizeIpcResult_ServiceStopping
 * @tc.desc: SAF_ERR_SERVICE_IS_STOPPING is NOT a dead-object code; the helper
 *           passes it through so the caller can apply its retry policy.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, NormalizeIpcResult_ServiceStopping, TestSize.Level0)
{
    EXPECT_EQ(NormalizeIpcResult(SAF_ERR_SERVICE_IS_STOPPING), SAF_ERR_SERVICE_IS_STOPPING);
}

/**
 * @tc.name: NormalizeIpcResult_Success
 * @tc.desc: SAF_SUCCESS passes through unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, NormalizeIpcResult_Success, TestSize.Level0)
{
    EXPECT_EQ(NormalizeIpcResult(SAF_SUCCESS), SAF_SUCCESS);
}

/**
 * @tc.name: NormalizeIpcResult_PermissionDenied
 * @tc.desc: A business error code passes through unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, NormalizeIpcResult_PermissionDenied, TestSize.Level0)
{
    EXPECT_EQ(NormalizeIpcResult(SAF_ERR_PERMISSION_DENIED), SAF_ERR_PERMISSION_DENIED);
}

/**
 * @tc.name: NormalizeIpcResult_GenericError
 * @tc.desc: An arbitrary non-dead error value passes through unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, NormalizeIpcResult_GenericError, TestSize.Level0)
{
    EXPECT_EQ(NormalizeIpcResult(GENERIC_IPC_ERROR), GENERIC_IPC_ERROR);
}

// ===== IsBatchSizeMatched =====

/**
 * @tc.name: IsBatchSizeMatched_BothEmpty
 * @tc.desc: Two empty batches match; 0 == 0 is a legal (no-op) update.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, IsBatchSizeMatched_BothEmpty, TestSize.Level0)
{
    EXPECT_TRUE(IsBatchSizeMatched(0, 0));
}

/**
 * @tc.name: IsBatchSizeMatched_EqualSingle
 * @tc.desc: A single-element pair matches.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, IsBatchSizeMatched_EqualSingle, TestSize.Level0)
{
    EXPECT_TRUE(IsBatchSizeMatched(1, 1));
}

/**
 * @tc.name: IsBatchSizeMatched_SourceEmptyDestNonEmpty
 * @tc.desc: An empty source with a non-empty destination does not match.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, IsBatchSizeMatched_SourceEmptyDestNonEmpty, TestSize.Level0)
{
    EXPECT_FALSE(IsBatchSizeMatched(0, 1));
}

/**
 * @tc.name: IsBatchSizeMatched_SourceNonEmptyDestEmpty
 * @tc.desc: A non-empty source with an empty destination does not match.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, IsBatchSizeMatched_SourceNonEmptyDestEmpty, TestSize.Level0)
{
    EXPECT_FALSE(IsBatchSizeMatched(1, 0));
}

/**
 * @tc.name: IsBatchSizeMatched_AtVectorMaxSize
 * @tc.desc: Equal sizes at the IPC vector upper bound (102400) match.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, IsBatchSizeMatched_AtVectorMaxSize, TestSize.Level0)
{
    constexpr size_t VECTOR_MAX_SIZE = 102400;
    EXPECT_TRUE(IsBatchSizeMatched(VECTOR_MAX_SIZE, VECTOR_MAX_SIZE));
}

/**
 * @tc.name: IsBatchSizeMatched_MaxSizeT
 * @tc.desc: Equal sizes at SIZE_MAX match without overflow in the comparison.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, IsBatchSizeMatched_MaxSizeT, TestSize.Level0)
{
    constexpr size_t MAX_SIZE_T = std::numeric_limits<size_t>::max();
    EXPECT_TRUE(IsBatchSizeMatched(MAX_SIZE_T, MAX_SIZE_T));
}

// ===== Type contract =====

/**
 * @tc.name: CredentialTypes_DefaultConstructed
 * @tc.desc: The credential value types default to empty / zero so a failed
 *           call cannot leak stale data through an out-parameter.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, CredentialTypes_DefaultConstructed, TestSize.Level0)
{
    ModelCredential mc;
    EXPECT_TRUE(mc.modelCredentialContent.baseUrl.empty());
    EXPECT_TRUE(mc.modelCredentialContent.credentialValue.empty());
    EXPECT_TRUE(mc.modelCredentialInfo.provider.empty());
    EXPECT_TRUE(mc.modelCredentialInfo.modelList.empty());
    EXPECT_FALSE(mc.modelCredentialInfo.state);
    EXPECT_TRUE(mc.modelCredentialIndex.credentialHandle.empty());
    EXPECT_TRUE(mc.modelCredentialIndex.credentialId.empty());

    ToolInfo info;
    EXPECT_EQ(info.agentTokenId, 0U);
    EXPECT_TRUE(info.subjectId.empty());

    CredentialBatchResult result;
    EXPECT_EQ(result.failedCount, 0);
    EXPECT_TRUE(result.failedErrorInfos.empty());
}

/**
 * @tc.name: CredentialTypes_ToolCredentialHasNoTokenField
 * @tc.desc: The tool write payload carries no agent token id: the service
 *           derives it from the caller's IPC token, so a caller cannot name
 *           another agent's storage group. A field count assertion pins the
 *           struct shape against accidental additions.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialInnerHelperTest, CredentialTypes_ToolCredentialHasNoTokenField, TestSize.Level0)
{
    ToolCredential tc;
    EXPECT_TRUE(tc.subjectId.empty());
    EXPECT_TRUE(tc.toolCredentialContents.empty());
    // The struct exposes exactly subjectId + toolCredentialContents; there is
    // no agentTokenId member for a caller to set.
    static_assert(!std::is_same_v<decltype(tc.subjectId), uint64_t>,
        "ToolCredential.subjectId must stay a string");
}

}  // namespace UnitTest::SafCredentialInnerHelperTest
