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
#include "saf_agent_fence_request_tool_permissions_test.h"

#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceRequestToolPermissionsTest {

const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.QUERY_CLI_TOOL",
    "ohos.permission.GET_TICKET_INFO",
    "ohos.permission.QUERY_TOOL_PERMISSIONS",
    "ohos.permission.MANAGE_TOOL_RUNTIME_PERMISSIONS",
    "ohos.permission.cli.BUNDLE_ACTIVE_INFO"
};

class SafAgentFenceRequestToolPermissionsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static MockToken* mockToken_;
};

MockToken* SafAgentFenceRequestToolPermissionsTest::mockToken_ = nullptr;

void SafAgentFenceRequestToolPermissionsTest::SetUpTestCase(void)
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

void SafAgentFenceRequestToolPermissionsTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceRequestToolPermissionsTest::SetUp(void)
{
}

void SafAgentFenceRequestToolPermissionsTest::TearDown(void)
{
}

/**
 * @tc.desc: RequestToolPermissions with unknown OperationType returns SAF_ERROR.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsUnknownOperation001, TestSize.Level0)
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
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsEmptyCLICmdNameOperation001, TestSize.Level0)
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
 * @tc.desc: RequestToolPermissions with API operation, expect non-success due to empty permission.
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsEmptyAPIPermission001, TestSize.Level0)
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
}