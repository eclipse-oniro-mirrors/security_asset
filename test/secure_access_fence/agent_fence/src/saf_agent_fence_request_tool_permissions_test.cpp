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
#include <map>
#include <string>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "saf_permission_change.h"
#include "permission_manager.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceRequestToolPermissionsTest {

class SafAgentFenceRequestToolPermissionsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceRequestToolPermissionsTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceRequestToolPermissionsTest::TearDownTestCase(void)
{
}

void SafAgentFenceRequestToolPermissionsTest::SetUp(void)
{
}

void SafAgentFenceRequestToolPermissionsTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.StatusMappingTest001
 * @tc.desc: Verify statusMapping contains 4 entries and (GRANTED, NOT_EXIST) maps to AUTHORIZED.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, StatusMappingTest001, TestSize.Level0)
{
    EXPECT_EQ(OHOS::Security::SAF::statusMapping.size(), 4);

    auto it = OHOS::Security::SAF::statusMapping.find(
        std::make_pair(OHOS::Security::SAF::PermissionStatus::GRANTED,
                       OHOS::Security::SAF::PolicyStatus::NOT_EXIST));
    EXPECT_NE(it, OHOS::Security::SAF::statusMapping.end());
    EXPECT_EQ(it->second, OHOS::Security::SAF::AuthStatus::AUTHORIZED);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.StatusMappingTest002
 * @tc.desc: Verify (GRANTED, REQUIRE_AUTH) maps to REQUIRE_AUTH.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, StatusMappingTest002, TestSize.Level0)
{
    auto it = OHOS::Security::SAF::statusMapping.find(
        std::make_pair(OHOS::Security::SAF::PermissionStatus::GRANTED,
                       OHOS::Security::SAF::PolicyStatus::REQUIRE_AUTH));
    EXPECT_NE(it, OHOS::Security::SAF::statusMapping.end());
    EXPECT_EQ(it->second, OHOS::Security::SAF::AuthStatus::REQUIRE_AUTH);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.StatusMappingTest003
 * @tc.desc: Verify (DENIED, NOT_EXIST) maps to FORBIDDEN.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, StatusMappingTest003, TestSize.Level0)
{
    auto it = OHOS::Security::SAF::statusMapping.find(
        std::make_pair(OHOS::Security::SAF::PermissionStatus::DENIED,
                       OHOS::Security::SAF::PolicyStatus::NOT_EXIST));
    EXPECT_NE(it, OHOS::Security::SAF::statusMapping.end());
    EXPECT_EQ(it->second, OHOS::Security::SAF::AuthStatus::FORBIDDEN);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.StatusMappingTest004
 * @tc.desc: Verify (DENIED, REQUIRE_AUTH) maps to REQUIRE_AUTH.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, StatusMappingTest004, TestSize.Level0)
{
    auto it = OHOS::Security::SAF::statusMapping.find(
        std::make_pair(OHOS::Security::SAF::PermissionStatus::DENIED,
                       OHOS::Security::SAF::PolicyStatus::REQUIRE_AUTH));
    EXPECT_NE(it, OHOS::Security::SAF::statusMapping.end());
    EXPECT_EQ(it->second, OHOS::Security::SAF::AuthStatus::REQUIRE_AUTH);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.StatusMappingNotFoundTest001
 * @tc.desc: Verify unknown status pair is not found in statusMapping.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, StatusMappingNotFoundTest001, TestSize.Level0)
{
    auto it = OHOS::Security::SAF::statusMapping.find(
        std::make_pair(static_cast<OHOS::Security::SAF::PermissionStatus>(999),
                       static_cast<OHOS::Security::SAF::PolicyStatus>(999)));
    EXPECT_EQ(it, OHOS::Security::SAF::statusMapping.end());
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.TicketMessageInfoDefaultTest001
 * @tc.desc: Verify TicketMessageInfo struct default values are zero/empty.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, TicketMessageInfoDefaultTest001, TestSize.Level0)
{
    OHOS::Security::SAF::TicketMessageInfo tmi;
    EXPECT_EQ(tmi.startTime, 0);
    EXPECT_EQ(tmi.ticketExpireTimeMs, 0);
    EXPECT_TRUE(tmi.cliInfos.empty());
    EXPECT_TRUE(tmi.apiPermissions.empty());
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsUnknownOperation001
 * @tc.desc: RequestToolPermissions with unknown OperationType returns SAF_ERROR.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsUnknownOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = static_cast<OHOS::Security::SAF::OperationType>(999);
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_EQ(result, SAF_ERROR);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsCLIOperation001
 * @tc.desc: RequestToolPermissions with CLI operation, expect non-success due to external service dependency.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsCLIOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
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
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsAPIOperation001
 * @tc.desc: RequestToolPermissions with API operation, expect non-success due to external service dependency.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsAPIOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo.permission = "ohos.permission.TEST";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsEmptyOperation001
 * @tc.desc: RequestToolPermissions with empty operationInfo, expect non-success.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsEmptyOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    permissionQuery.operationInfo = {};
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsMixedOperation001
 * @tc.desc: RequestToolPermissions with mixed CLI+API operations, expect non-success.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsMixedOperation001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo1;
    opInfo1.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo1.cliCmdInfo.cmdName = "cmd1";
    opInfo1.cliCmdInfo.subCmd = "run";
    OHOS::Security::SAF::OperationInfo opInfo2;
    opInfo2.operationType = OHOS::Security::SAF::OperationType::API;
    opInfo2.permission = "perm_api";
    permissionQuery.operationInfo.push_back(opInfo1);
    permissionQuery.operationInfo.push_back(opInfo2);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsWithTokenId001
 * @tc.desc: RequestToolPermissions with non-zero callerTokenId, expect non-success.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsWithTokenId001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 12345;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "testCmd";
    opInfo.cliCmdInfo.subCmd = "run";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceRequestToolPermissionsTest.RequestToolPermissionsNoTicket001
 * @tc.desc: RequestToolPermissions with needTicket=false and CLI operation, expect non-success.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceRequestToolPermissionsTest, RequestToolPermissionsNoTicket001, TestSize.Level0)
{
    auto &manager = *OHOS::Security::SAF::PermissionManager::GetInstance();
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "testCmd";
    opInfo.cliCmdInfo.subCmd = "run";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = manager.RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

}
