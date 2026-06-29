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

#include <gtest/gtest.h>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "saf_permission_change.h"
#include "permission_manager.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceGrantToolPermissionsByUserTest {

class SafAgentFenceGrantToolPermissionsByUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceGrantToolPermissionsByUserTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceGrantToolPermissionsByUserTest::TearDownTestCase(void)
{
}

void SafAgentFenceGrantToolPermissionsByUserTest::SetUp(void)
{
}

void SafAgentFenceGrantToolPermissionsByUserTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserEmptyPermissionInfo001
 * @tc.desc: GrantToolPermissionsByUser with empty permissionInfo returns SAF_ERR_ARG_EMPTY.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserEmptyPermissionInfo001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    authResult.permissionInfo = {};
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_ERR_ARG_EMPTY);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserDeniedPermission001
 * @tc.desc: GrantToolPermissionsByUser with DENIED permission returns SAF_ERR_PERMISSION_DENIED.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserDeniedPermission001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_ERR_PERMISSION_DENIED);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserNotDeterminedPermission001
 * @tc.desc: GrantToolPermissionsByUser with NOT_DETERMINED permission returns SAF_ERR_PERMISSION_DENIED.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserNotDeterminedPermission001,
    TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_ERR_PERMISSION_DENIED);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserRestrictedPermission001
 * @tc.desc: GrantToolPermissionsByUser with RESTRICTED permission returns SAF_ERR_PERMISSION_DENIED.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserRestrictedPermission001,
    TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_ERR_PERMISSION_DENIED);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserUnknownOperation001
 * @tc.desc: GrantToolPermissionsByUser with unknown OperationType returns SAF_ERROR.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserUnknownOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_ERROR);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserEmptyList001
 * @tc.desc: GrantToolPermissionsByUser with empty userAuthResults returns SAF_SUCCESS.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserEmptyList001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_TRUE(ticketInfos.empty());
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserCLIOperation001
 * @tc.desc: GrantToolPermissionsByUser with CLI operation, expect non-success due to external service dependency.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserCLIOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserAPIOperation001
 * @tc.desc: GrantToolPermissionsByUser with API operation, expect non-success due to external service dependency.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserAPIOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserMixedOperation001
 * @tc.desc: GrantToolPermissionsByUser with mixed CLI+API operations, expect non-success.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserMixedOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGrantToolPermissionsByUserTest.GrantToolPermissionsByUserEmptyOperation001
 * @tc.desc: GrantToolPermissionsByUser with empty operationInfo, expect non-success.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceGrantToolPermissionsByUserTest, GrantToolPermissionsByUserEmptyOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    authResult.permissionQuery.operationInfo = {};
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = manager.GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

}
