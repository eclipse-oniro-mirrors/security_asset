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
#include "saf_agent_fence_grant_tool_permissions_by_user_local_test.h"

#include <gtest/gtest.h>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceGrantToolPermissionsByUserLocalTest {

const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.QUERY_CLI_TOOL",
    "ohos.permission.GET_TICKET_INFO",
    "ohos.permission.QUERY_TOOL_PERMISSIONS",
    "ohos.permission.MANAGE_TOOL_RUNTIME_PERMISSIONS",
    "ohos.permission.cli.BUNDLE_ACTIVE_INFO"
};

class SafAgentFenceGrantToolPermissionsByUserLocalTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static MockToken* mockToken_;
};

MockToken* SafAgentFenceGrantToolPermissionsByUserLocalTest::mockToken_ = nullptr;

void SafAgentFenceGrantToolPermissionsByUserLocalTest::SetUpTestCase(void)
{
    mockToken_ = new MockToken(PERMISSIONS, true, true);
    ASSERT_NE(mockToken_, nullptr);

    std::string errorMsg = mockToken_->GetMockErrorMsg();
    if (errorMsg != "success") {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "MockToken failed: " << errorMsg;
    } else {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "Failed to create MockToken";
    }
}

void SafAgentFenceGrantToolPermissionsByUserLocalTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceGrantToolPermissionsByUserLocalTest::SetUp(void)
{
}

void SafAgentFenceGrantToolPermissionsByUserLocalTest::TearDown(void)
{
}

/**
 * @tc.desc: GrantToolPermissionsByUser with empty permissionInfo returns SAF_SUCCESS.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserEmptyPermissionInfo001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    authResult.permissionInfo = {};
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with DENIED permission returns SAF_SUCCESS.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserDeniedPermission001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::DENIED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with NOT_DETERMINED permission returns SAF_SUCCESS.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserNotDeterminedPermission001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::NOT_DETERMINED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with RESTRICTED permission returns SAF_SUCCESS.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserRestrictedPermission001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::RESTRICTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with unknown OperationType returns SAF_SUCCESS.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserUnknownOperation001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = static_cast<OHOS::Security::SAF::OperationType>(999);
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with empty userAuthResults returns SAF_SUCCESS.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserEmptyList001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_TRUE(ticketInfos.empty());
}

/**
 * @tc.desc: GrantToolPermissionsByUser with CLI operation, expect success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserCLIOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "testCmd";
    opInfo.cliCmdInfo.subCmd = "run";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with API operation, expect success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserAPIOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo.permission = "ohos.permission.TEST";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with mixed CLI+API operations, expect success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserMixedOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo1;
    opInfo1.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo1.cliCmdInfo.cmdName = "cmd1";
    opInfo1.cliCmdInfo.subCmd = "run";
    OHOS::Security::SAF::OperationInfo opInfo2;
    opInfo2.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo2.permission = "perm_api";
    authResult.permissionQuery.operationInfo.push_back(opInfo1);
    authResult.permissionQuery.operationInfo.push_back(opInfo2);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with empty operationInfo, expect success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserEmptyOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = true;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: GrantToolPermissionsByUser with exceed expire time limit, expect success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, GrantToolPermissionsByUserExceedExpireTime001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = true;
    authResult.permissionQuery.ticketExpireTimeMs = 24 * 60 * 60 * 1000 + 1;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: VerifyTicket with empty ticketInfo, expect non-success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, VerifyTicketWithEmptyTicketInfo001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = true;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<OHOS::Security::SAF::CliInfo> cliInfos;
    std::string ticketInfo = ticketInfos[0].ticket;
    result = agentFence.VerifyTicket(100, "0", ticketInfo, cliInfos);
    EXPECT_EQ(result, 196633);
}

/**
 * @tc.desc: VerifyTicket with invalid ticketInfo, expect non-success.
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserLocalTest, VerifyTicketWithInvalidTicketInfo001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CliInfo> cliInfos;
    std::string ticketInfo = "12345";
    int32_t result = agentFence.VerifyTicket(100, "0", ticketInfo, cliInfos);
    EXPECT_EQ(result, 196633);
}
}