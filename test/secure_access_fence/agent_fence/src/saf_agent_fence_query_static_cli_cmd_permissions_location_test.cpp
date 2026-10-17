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

#include "saf_agent_fence_query_static_cli_cmd_permissions_location_test.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceQueryStaticCliCmdPermissionsLocationTest {

const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.QUERY_TOOL_PERMISSIONS",
    "ohos.permission.cli.CONTROL_LOCATION_SWITCH",
};

class SafAgentFenceQueryStaticCliCmdPermissionsLocationTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static MockToken* mockToken_;
    static int32_t GetCallerTokenId()
    {
        return static_cast<int32_t>(GetSelfTokenID());
    }
};

MockToken* SafAgentFenceQueryStaticCliCmdPermissionsLocationTest::mockToken_ = nullptr;

void SafAgentFenceQueryStaticCliCmdPermissionsLocationTest::SetUpTestCase(void)
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

void SafAgentFenceQueryStaticCliCmdPermissionsLocationTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceQueryStaticCliCmdPermissionsLocationTest::SetUp(void)
{
}

void SafAgentFenceQueryStaticCliCmdPermissionsLocationTest::TearDown(void)
{
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions for ohos-location/enable, expect CONTROL_LOCATION_SWITCH permission.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsLocationTest, QueryStaticCliCmdPermissionsLocationEnable001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-location";
    cmd.subCmd = "enable";
    cmds.push_back(cmd);
    int32_t callerTokenId = GetCallerTokenId();
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_EQ(result, SAF_SUCCESS);
    ASSERT_EQ(cmdPermissions.size(), 1);
    ASSERT_EQ(cmdPermissions[0].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "ohos.permission.cli.CONTROL_LOCATION_SWITCH");
}
}
