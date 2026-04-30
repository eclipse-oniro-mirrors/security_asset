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

#include "saf_agent_fence_test.h"

#include <gtest/gtest.h>

#include "saf_agent_fence.h"
#include "saf_result.h"
#include "secure_access_fence_system_type.h"
#include "saf_permission_change.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceTest {
class SafAgentFenceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceTest::TearDownTestCase(void)
{
}

void SafAgentFenceTest::SetUp(void)
{
}

void SafAgentFenceTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchGenerateTicketTest001
 * @tc.desc: Batch generate tickets with valid parameters.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchGenerateTicketTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages = {"message1", "message2", "message3"};
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), messages.size());
    for (size_t i = 0; i < ticketInfos.size(); i++) {
        EXPECT_EQ(ticketInfos[i].message, messages[i]);
        EXPECT_FALSE(ticketInfos[i].ticket.empty());
    }
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchGenerateTicketTest002
 * @tc.desc: Batch generate tickets with empty messages.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchGenerateTicketTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchGenerateTicketTest003
 * @tc.desc: Batch generate tickets with large batch.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchGenerateTicketTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    for (int i = 0; i < 50; i++) {
        messages.push_back("message_" + std::to_string(i));
    }
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), messages.size());
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchVerifyTicketTest001
 * @tc.desc: Batch verify tickets with valid parameters.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchVerifyTicketTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"test_message1", "challenge1", "valid_ticket1"});
    verifyInfos.push_back({"test_message2", "challenge2", "valid_ticket2"});

    std::vector<int32_t> verifyRes;

    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), verifyInfos.size());
    for (size_t i = 0; i < verifyRes.size(); i++) {
        EXPECT_EQ(verifyRes[i], SEC_SAF_SUCCESS);
    }
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchVerifyTicketTest002
 * @tc.desc: Batch verify tickets with empty verify infos.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchVerifyTicketTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    std::vector<int32_t> verifyRes;

    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchVerifyTicketTest003
 * @tc.desc: Batch verify tickets with invalid ticket.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchVerifyTicketTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"test_message1", "challenge1", "invalid_ticket"});

    std::vector<int32_t> verifyRes;

    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), verifyInfos.size());
    EXPECT_NE(verifyRes[0], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceBatchVerifyTicketTest004
 * @tc.desc: Batch verify tickets with mixed valid/invalid tickets.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceBatchVerifyTicketTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"test_message1", "challenge1", "valid_ticket"});
    verifyInfos.push_back({"test_message2", "challenge2", "invalid_ticket"});
    verifyInfos.push_back({"test_message3", "challenge3", "valid_ticket"});

    std::vector<int32_t> verifyRes;

    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), verifyInfos.size());
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest001
 * @tc.desc: Batch query command permission with "ohos-cliTimer" and empty subcommand.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer", ""});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest002
 * @tc.desc: Batch query command permission with "ohos-cliTimer run".
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer", "run"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest003
 * @tc.desc: Batch query command permission with "ohos-cliTimer-2" and empty subcommand.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", ""});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 3);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test1");
    EXPECT_EQ(cmdPermissions[0].permissions[1], "test2");
    EXPECT_EQ(cmdPermissions[0].permissions[2], "test3");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest004
 * @tc.desc: Batch query command permission with "ohos-cliTimer-2 run".
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", "run"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "testsub1");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest005
 * @tc.desc: Batch query command permission with "ohos-cliTimer-2 stop".
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", "stop"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "stop");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 2);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "testsub2");
    EXPECT_EQ(cmdPermissions[0].permissions[1], "testsub3");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest006
 * @tc.desc: Batch query command permission with "ohos-cliTimer-2 notexist".
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", "notexist"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 1);
    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "notexist");
    EXPECT_TRUE(cmdPermissions[0].permissions.empty());
    EXPECT_EQ(cmdPermissions[0].queryRet, 1);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest007
 * @tc.desc: Batch query command permission with empty cmdName and "run" subcommand.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest007, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"", "run"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest008
 * @tc.desc: Batch query two commands with "ohos-cliTimer-2" empty subcmd and "run" subcmd.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest008, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", ""});
    cmds.push_back({"ohos-cliTimer-2", "run"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 2);

    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 3);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test1");
    EXPECT_EQ(cmdPermissions[0].permissions[1], "test2");
    EXPECT_EQ(cmdPermissions[0].permissions[2], "test3");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);

    EXPECT_EQ(cmdPermissions[1].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[1].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[1].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[1].permissions[0], "testsub1");
    EXPECT_EQ(cmdPermissions[1].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest009
 * @tc.desc: Batch query three commands with "ohos-cliTimer-2" empty, run and notexist subcmd.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest009, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", ""});
    cmds.push_back({"ohos-cliTimer-2", "run"});
    cmds.push_back({"ohos-cliTimer-2", "notexist"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 3);

    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 3);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test1");
    EXPECT_EQ(cmdPermissions[0].permissions[1], "test2");
    EXPECT_EQ(cmdPermissions[0].permissions[2], "test3");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);

    EXPECT_EQ(cmdPermissions[1].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[1].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[1].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[1].permissions[0], "testsub1");
    EXPECT_EQ(cmdPermissions[1].queryRet, 0);

    EXPECT_EQ(cmdPermissions[2].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[2].cmd.subCmd, "notexist");
    EXPECT_TRUE(cmdPermissions[2].permissions.empty());
    EXPECT_EQ(cmdPermissions[2].queryRet, 1);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest010
 * @tc.desc: Batch query four commands with ohos-cliTimer-2 and ohos-cliTimer.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest010, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", ""});
    cmds.push_back({"ohos-cliTimer-2", "run"});
    cmds.push_back({"ohos-cliTimer", ""});
    cmds.push_back({"ohos-cliTimer", "run"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 4);

    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 3);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test1");
    EXPECT_EQ(cmdPermissions[0].permissions[1], "test2");
    EXPECT_EQ(cmdPermissions[0].permissions[2], "test3");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);

    EXPECT_EQ(cmdPermissions[1].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[1].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[1].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[1].permissions[0], "testsub1");
    EXPECT_EQ(cmdPermissions[1].queryRet, 0);

    EXPECT_EQ(cmdPermissions[2].cmd.cmdName, "ohos-cliTimer");
    EXPECT_EQ(cmdPermissions[2].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[2].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[2].permissions[0], "test");
    EXPECT_EQ(cmdPermissions[2].queryRet, 0);

    EXPECT_EQ(cmdPermissions[3].cmd.cmdName, "ohos-cliTimer");
    EXPECT_EQ(cmdPermissions[3].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[3].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[3].permissions[0], "test");
    EXPECT_EQ(cmdPermissions[3].queryRet, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest011
 * @tc.desc: Batch query five commands with ohos-cliTimer-2, ohos-cliTimer and notexist.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest011, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer-2", ""});
    cmds.push_back({"ohos-cliTimer-2", "run"});
    cmds.push_back({"ohos-cliTimer", ""});
    cmds.push_back({"ohos-cliTimer", "run"});
    cmds.push_back({"ohos-cliTimer-2", "notexist"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 5);

    EXPECT_EQ(cmdPermissions[0].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[0].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[0].permissions.size(), 3);
    EXPECT_EQ(cmdPermissions[0].permissions[0], "test1");
    EXPECT_EQ(cmdPermissions[0].permissions[1], "test2");
    EXPECT_EQ(cmdPermissions[0].permissions[2], "test3");
    EXPECT_EQ(cmdPermissions[0].queryRet, 0);

    EXPECT_EQ(cmdPermissions[1].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[1].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[1].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[1].permissions[0], "testsub1");
    EXPECT_EQ(cmdPermissions[1].queryRet, 0);

    EXPECT_EQ(cmdPermissions[2].cmd.cmdName, "ohos-cliTimer");
    EXPECT_EQ(cmdPermissions[2].cmd.subCmd, "");
    EXPECT_EQ(cmdPermissions[2].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[2].permissions[0], "test");
    EXPECT_EQ(cmdPermissions[2].queryRet, 0);

    EXPECT_EQ(cmdPermissions[3].cmd.cmdName, "ohos-cliTimer");
    EXPECT_EQ(cmdPermissions[3].cmd.subCmd, "run");
    EXPECT_EQ(cmdPermissions[3].permissions.size(), 1);
    EXPECT_EQ(cmdPermissions[3].permissions[0], "test");
    EXPECT_EQ(cmdPermissions[3].queryRet, 0);

    EXPECT_EQ(cmdPermissions[4].cmd.cmdName, "ohos-cliTimer-2");
    EXPECT_EQ(cmdPermissions[4].cmd.subCmd, "notexist");
    EXPECT_TRUE(cmdPermissions[4].permissions.empty());
    EXPECT_EQ(cmdPermissions[4].queryRet, 1);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest012
 * @tc.desc: Batch query with zero commands, expect non-zero error return.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest012, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest013
 * @tc.desc: Batch query 99 commands with "ohos-cliTimer-2" empty subcmd.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest013, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    for (int i = 0; i < 99; i++) {
        cmds.push_back({"ohos-cliTimer-2", ""});
    }

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 99);

    for (int i = 0; i < 99; i++) {
        EXPECT_EQ(cmdPermissions[i].cmd.cmdName, "ohos-cliTimer-2");
        EXPECT_EQ(cmdPermissions[i].cmd.subCmd, "");
        EXPECT_EQ(cmdPermissions[i].permissions.size(), 3);
        EXPECT_EQ(cmdPermissions[i].permissions[0], "test1");
        EXPECT_EQ(cmdPermissions[i].permissions[1], "test2");
        EXPECT_EQ(cmdPermissions[i].permissions[2], "test3");
        EXPECT_EQ(cmdPermissions[i].queryRet, 0);
    }
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest014
 * @tc.desc: Batch query 100 commands, expect non-zero error return.
 * @tc.type: FUNC
 * @tc.result: non-zero
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest014, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    for (int i = 0; i < 100; i++) {
        cmds.push_back({"ohos-cliTimer-2", ""});
    }

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceGenerateAndVerifyTest001
 * @tc.desc: Generate tickets then verify them.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceGenerateAndVerifyTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = {"message1", "message2"};
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), messages.size());

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    for (size_t i = 0; i < verifyRes.size(); i++) {
        EXPECT_EQ(verifyRes[i], SEC_SAF_SUCCESS);
    }
}
}