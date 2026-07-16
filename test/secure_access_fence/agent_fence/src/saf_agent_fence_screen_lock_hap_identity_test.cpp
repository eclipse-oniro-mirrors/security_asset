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

#include "saf_agent_fence_screen_lock_hap_identity_test.h"

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <chrono>
#include <thread>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceScreenLockHapIdentityTest {

class SafAgentFenceScreenLockHapIdentityTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    static MockToken* mockToken_;
};

MockToken* SafAgentFenceScreenLockHapIdentityTest::mockToken_ = nullptr;

void SafAgentFenceScreenLockHapIdentityTest::SetUpTestCase(void)
{
    std::vector<std::string> permissions = {
        "ohos.permission.QUERY_CLI_TOOL",
        "ohos.permission.GET_TICKET_INFO",
        "ohos.permission.QUERY_TOOL_PERMISSIONS",
    };

    mockToken_ = new MockToken(permissions, true, true);
    ASSERT_NE(mockToken_, nullptr);

    std::string errorMsg = mockToken_->GetMockErrorMsg();
    if (errorMsg != "success") {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "MockToken failed: " << errorMsg;
    } else {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "Failed to create MockToken";
    }
}

void SafAgentFenceScreenLockHapIdentityTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceScreenLockHapIdentityTest::SetUp(void)
{
}

void SafAgentFenceScreenLockHapIdentityTest::TearDown(void)
{
}

HWTEST_F(SafAgentFenceScreenLockHapIdentityTest, RequestToolPermissionsCliTimerScreenLock001, TestSize.Level0)
{
    OHOS::Security::SAF::PermissionQuery permissionQuery;
    permissionQuery.callerTokenId = 0;
    OHOS::Security::SAF::OperationInfo opInfo;
    opInfo.operationType = OHOS::Security::SAF::OperationType::CLI;
    opInfo.cliCmdInfo.cmdName = "ohos-cliTimer-3";
    opInfo.cliCmdInfo.subCmd = "run2";
    permissionQuery.operationInfo.push_back(opInfo);
    permissionQuery.needTicket = false;
    permissionQuery.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::PermissionQueryResult permissionQueryResult;
    int32_t result = OHOS::Security::SAF::SafAgentFence::RequestToolPermissions(permissionQuery, permissionQueryResult);
    EXPECT_TRUE(result == SAF_ERR_SCREENLOCK_IS_LOCKED);
}

}