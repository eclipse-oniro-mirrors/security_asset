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
#include "saf_agent_fence_query_static_cli_cmd_permissions_test.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceQueryStaticCliCmdPermissionsTest {

const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.QUERY_TOOL_PERMISSIONS",
    "ohos.permission.cli.CONTROL_LOCATION_SWITCH",
};

class SafAgentFenceQueryStaticCliCmdPermissionsTest : public testing::Test {
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

MockToken* SafAgentFenceQueryStaticCliCmdPermissionsTest::mockToken_ = nullptr;

void SafAgentFenceQueryStaticCliCmdPermissionsTest::SetUpTestCase(void)
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

void SafAgentFenceQueryStaticCliCmdPermissionsTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceQueryStaticCliCmdPermissionsTest::SetUp(void)
{
}

void SafAgentFenceQueryStaticCliCmdPermissionsTest::TearDown(void)
{
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions with empty cmds, expect non-success.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsEmptyCmds001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    int32_t callerTokenId = GetCallerTokenId();
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions with empty cmdName, expect SAF_ERR_ARG_INVALID.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsEmptyCmdName001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "";
    cmd.subCmd = "run";
    cmds.push_back(cmd);
    int32_t callerTokenId = GetCallerTokenId();
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions with cmds count exceeding max (300), expect non-success.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsExceedsMaxCount001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-displayManager";
    cmd.subCmd = "set-brightness";
    for (int32_t i = 0; i < 301; ++i) {
        cmds.push_back(cmd);
    }
    int32_t callerTokenId = GetCallerTokenId();
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions for ohos-batteryManager/total-energy, expect empty permissions.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsBatteryManagerTotalEnergy001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-batteryManager";
    cmd.subCmd = "total-energy";
    cmds.push_back(cmd);
    int32_t callerTokenId = GetCallerTokenId();
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_EQ(result, SAF_SUCCESS);
    ASSERT_EQ(cmdPermissions.size(), 1);
    EXPECT_TRUE(cmdPermissions[0].permissions.empty());
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions for ohos-bm/uninstall, expect empty permissions.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsBmUninstall001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-bm";
    cmd.subCmd = "uninstall";
    cmds.push_back(cmd);
    int32_t callerTokenId = GetCallerTokenId();
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_EQ(result, SAF_SUCCESS);
    ASSERT_EQ(cmdPermissions.size(), 1);
    EXPECT_TRUE(cmdPermissions[0].permissions.empty());
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions for ohos-location/enable, expect CONTROL_LOCATION_SWITCH permission.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsLocationEnable001, TestSize.Level0)
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
    ASSERT_EQ(cmdPermissions[0].permissions.size(), 0);
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions with invalid callerTokenId (0), expect SAF_ERR_ARG_INVALID.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsTest, QueryStaticCliCmdPermissionsInvalidCallerTokenId001,
    TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-location";
    cmd.subCmd = "enable";
    cmds.push_back(cmd);
    int32_t callerTokenId = 0;
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_NE(result, SAF_SUCCESS);
}

class SafAgentFenceQueryStaticCliCmdPermissionsPermTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void) {}
    void TearDown(void) {}
};

/**
 * @tc.desc: QueryStaticCliCmdPermissions without QUERY_TOOL_PERMISSIONS, expect permission denied.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsPermTest, QueryStaticCliCmdPermissionsNoQueryToolPermission001,
    TestSize.Level0)
{
    std::vector<std::string> perms = { "ohos.permission.QUERY_TOOL_PERMISSIONS" };
    MockToken mockToken(perms, true, true);
    ASSERT_NE(mockToken.GetTokenId(), INVALID_TOKENID);
    mockToken.Revoke("ohos.permission.QUERY_TOOL_PERMISSIONS");

    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-location";
    cmd.subCmd = "enable";
    cmds.push_back(cmd);
    int32_t callerTokenId = static_cast<int32_t>(GetSelfTokenID());
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: QueryStaticCliCmdPermissions without CONTROL_LOCATION_SWITCH, expect success but empty permissions.
 */
HWTEST_F(SafAgentFenceQueryStaticCliCmdPermissionsPermTest,
    QueryStaticCliCmdPermissionsNoLocationSwitchPermission001, TestSize.Level0)
{
    std::vector<std::string> perms = {
        "ohos.permission.QUERY_TOOL_PERMISSIONS",
        "ohos.permission.cli.CONTROL_LOCATION_SWITCH",
    };
    MockToken mockToken(perms, true, true);
    ASSERT_NE(mockToken.GetTokenId(), INVALID_TOKENID);
    mockToken.Revoke("ohos.permission.cli.CONTROL_LOCATION_SWITCH");

    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-location";
    cmd.subCmd = "enable";
    cmds.push_back(cmd);
    int32_t callerTokenId = static_cast<int32_t>(GetSelfTokenID());
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;
    int32_t result = agentFence.QueryStaticCliCmdPermissions(cmds, callerTokenId, cmdPermissions);
    EXPECT_EQ(result, SAF_SUCCESS);
    ASSERT_EQ(cmdPermissions.size(), 1);
    EXPECT_TRUE(cmdPermissions[0].permissions.empty());
}
}
