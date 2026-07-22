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
#include "saf_agent_fence_request_tool_permissions_local_test.h"

#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceRequestToolPermissionsLocalTest {

const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.QUERY_CLI_TOOL",
    "ohos.permission.GET_TICKET_INFO",
    "ohos.permission.QUERY_TOOL_PERMISSIONS",
    "ohos.permission.MANAGE_TOOL_RUNTIME_PERMISSIONS",
    "ohos.permission.cli.BUNDLE_ACTIVE_INFO"
};

class SafAgentFenceRequestToolPermissionsLocalTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static MockToken* mockToken_;
};

MockToken* SafAgentFenceRequestToolPermissionsLocalTest::mockToken_ = nullptr;

void SafAgentFenceRequestToolPermissionsLocalTest::SetUpTestCase(void)
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

void SafAgentFenceRequestToolPermissionsLocalTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceRequestToolPermissionsLocalTest::SetUp(void)
{
}

void SafAgentFenceRequestToolPermissionsLocalTest::TearDown(void)
{
}

/**
 * @tc.desc: RequestToolPermissions with unknown OperationType returns SAF_ERROR.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsUnknownOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = static_cast<OHOS::Security::SAF::OperationType>(999);
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

/**
 * @tc.desc: RequestToolPermissions with CLI operation, expect non-success due to empty cmdName.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsEmptyCLICmdNameOperation001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "";
    opInfo.cliCmdInfo.subCmd = "";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

/**
 * @tc.desc: RequestToolPermissions with CLI operation, expect non-success due to external service dependency.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsCLIOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "nonexist_cmd";
    opInfo.cliCmdInfo.subCmd = "";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERR_TOOL_ERROR);
}

/**
 * @tc.desc: RequestToolPermissions with API operation, expect non-success due to empty permission.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsEmptyAPIPermission001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo.permission = "";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

/**
 * @tc.desc: RequestToolPermissions with API operation, expect non-success due to external service dependency.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsAPIOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo.permission = "ohos.permission.TEST";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERROR);
}

/**
 * @tc.desc: RequestToolPermissions with empty operationInfo, expect non-success.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsEmptyOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    permissionQuery.operationInfo = {};
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.desc: RequestToolPermissions with mixed CLI+API operations, expect non-success.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsMixedOperation001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo1;
    opInfo1.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo1.cliCmdInfo.cmdName = "ohos-usageStatsQuery";
    opInfo1.cliCmdInfo.subCmd = "query-latest-used-time";
    OHOS::Security::SAF::OperationInfo opInfo2;
    opInfo2.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo2.permission = "ohos.permission.QUERY_TOOL_PERMISSIONS";
    permissionQuery.operationInfo.push_back(opInfo1);
    permissionQuery.operationInfo.push_back(opInfo2);
    permissionQuery.needTicket = true;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERROR);
}

/**
 * @tc.desc: RequestToolPermissions with exceeds max expire time limit, expect non-success.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsExceedsMaxExpireTimeLimit001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo1;
    opInfo1.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo1.cliCmdInfo.cmdName = "ohos-usageStatsQuery";
    opInfo1.cliCmdInfo.subCmd = "query-latest-used-time";
    OHOS::Security::SAF::OperationInfo opInfo2;
    opInfo2.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo2.permission = "ohos.permission.QUERY_TOOL_PERMISSIONS";
    permissionQuery.operationInfo.push_back(opInfo1);
    permissionQuery.operationInfo.push_back(opInfo2);
    permissionQuery.needTicket = true;
    permissionQuery.ticketExpireTimeMs = 24 * 60 * 60 * 1000 + 1;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERROR);
}

/**
 * @tc.desc: RequestToolPermissions with non-zero callerTokenId, expect non-success.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsWithTokenId001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 12345;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-usageStatsQuery";
    opInfo.cliCmdInfo.subCmd = "query-latest-used-time";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, 12100004);
}

/**
 * @tc.desc: RequestToolPermissions with needTicket=false and CLI operation, expect non-success.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsLocalTest, RequestToolPermissionsNoTicket001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-usageStatsQuery";
    opInfo.cliCmdInfo.subCmd = "query-latest-used-time";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = agentFence.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERROR);
}
}