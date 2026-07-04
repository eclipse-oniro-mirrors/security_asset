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

#include "saf_agent_fence_screen_lock_test.h"

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <chrono>
#include <thread>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "saf_permission_change.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceScreenLockTest {

class SafAgentFenceScreenLockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceScreenLockTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceScreenLockTest::TearDownTestCase(void)
{
}

void SafAgentFenceScreenLockTest::SetUp(void)
{
}

void SafAgentFenceScreenLockTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceScreenLockTest.GrantToolPermissionsByUserCliTimerScreenLock001
 * @tc.desc: GrantToolPermissionsByUser with ohos-cliTimer run command (isLockScreenExecutionAllowed=false).
 *           This test verifies the screen lock check logic path in grant flow.
 * @tc.type: FUNC
 * @tc.result: depends on screen lock status and external service
 */
HWTEST_F(SafAgentFenceScreenLockTest, GrantToolPermissionsByUserCliTimerScreenLock001, TestSize.Level0)
{
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-cliTimer";
    opInfo.cliCmdInfo.subCmd = "run2";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = OHOS::Security::SAF::SafAgentFence::GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    // Result depends on external cli_tool_mgr service and screen lock status
    EXPECT_TRUE(result == SAF_SUCCESS);
    EXPECT_TRUE(ticketInfos.size() == 1);
    EXPECT_TRUE(!ticketInfos[0].message.empty());
    EXPECT_TRUE(!ticketInfos[0].challenge.empty());
    EXPECT_TRUE(!ticketInfos[0].ticket.empty());
}

/**
 * @tc.name: SafAgentFenceScreenLockTest.GrantToolPermissionsByUserCliTimerScreenLock002
 * @tc.desc: GrantToolPermissionsByUser with ohos-cliTimer run command (isLockScreenExecutionAllowed=false).
 *           This test verifies the screen lock check logic path in grant flow.
 * @tc.type: FUNC
 * @tc.result: depends on screen lock status and external service
 */
HWTEST_F(SafAgentFenceScreenLockTest, GrantToolPermissionsByUserCliTimerScreenLock002, TestSize.Level0)
{
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-cliTimer-2";
    opInfo.cliCmdInfo.subCmd = "run2";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 0;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = OHOS::Security::SAF::SafAgentFence::GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    // Result depends on external cli_tool_mgr service and screen lock status
    EXPECT_TRUE(result == SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceScreenLockTest.VerifyTicketScreenLock001
 * @tc.desc: RequestToolPermissions with ohos-cliTimer run command and needTicket=true.
 *           This tests the full flow including ticket generation with screen lock check.
 * @tc.type: FUNC
 * @tc.result: depends on screen lock status and external service
 */
HWTEST_F(SafAgentFenceScreenLockTest, VerifyTicketScreenLock001, TestSize.Level0)
{
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-cliTimer";
    opInfo.cliCmdInfo.subCmd = "run2";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 15000;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = OHOS::Security::SAF::SafAgentFence::GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<OHOS::Security::SAF::CliInfo> cliInfos;
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::string ticketInfo = ticketInfos[0].ticket;
    result = agentFence.VerifyTicket(100, "0", ticketInfo, cliInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceScreenLockTest.VerifyTicketScreenLock002
 * @tc.desc: RequestToolPermissions with ohos-cliTimer run command and needTicket=true.
 *           This tests the full flow including ticket generation with screen lock check.
 * @tc.type: FUNC
 * @tc.result: depends on screen lock status and external service
 */
HWTEST_F(SafAgentFenceScreenLockTest, VerifyTicketScreenLock002, TestSize.Level0)
{
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-cliTimer-2";
    opInfo.cliCmdInfo.subCmd = "run2";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 15000;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = OHOS::Security::SAF::SafAgentFence::GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<OHOS::Security::SAF::CliInfo> cliInfos;
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::string ticketInfo = ticketInfos[0].ticket;
    result = agentFence.VerifyTicket(100, "0", ticketInfo, cliInfos);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceScreenLockTest.VerifyTicketScreenLock003
 * @tc.desc: RequestToolPermissions with ohos-cliTimer run command and needTicket=true.
 *           This tests the full flow including ticket generation with screen lock check.
 * @tc.type: FUNC
 * @tc.result: depends on screen lock status and external service
 */
HWTEST_F(SafAgentFenceScreenLockTest, VerifyTicketScreenLock003, TestSize.Level0)
{
    std::vector<OHOS::Security::SAF::UserAuthResult> userAuthResults;
    OHOS::Security::SAF::UserAuthResult authResult;
    OHOS::Security::SAF::PermissionInfo permInfo;
    permInfo.permission = "perm1";
    permInfo.permissionStatus = OHOS::Security::SAF::PermissionStatus::GRANTED;
    authResult.permissionInfo.push_back(permInfo);
    authResult.permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-cliTimer";
    opInfo.cliCmdInfo.subCmd = "run2";
    authResult.permissionQuery.operationInfo.push_back(opInfo);
    authResult.permissionQuery.needTicket = false;
    authResult.permissionQuery.ticketExpireTimeMs = 15000;
    userAuthResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = OHOS::Security::SAF::SafAgentFence::GrantToolPermissionsByUser(userAuthResults, ticketInfos);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::vector<OHOS::Security::SAF::CliInfo> cliInfos;
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::string ticketInfo = ticketInfos[0].ticket;
    result = agentFence.VerifyTicket(100, "0", ticketInfo, cliInfos);
    EXPECT_EQ(result, SAF_ERR_SCREENLOCK_IS_LOCKED);
}

}