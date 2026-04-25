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
#include "secure_access_fence_system_type.h"

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
 * @tc.desc: Batch query permission by sub command with valid parameters.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"cmd1", "sub1"});
    cmds.push_back({"cmd2", "sub2"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), cmds.size());
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest002
 * @tc.desc: Batch query command permission with empty commands.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), 0);
}

/**
 * @tc.name: SafAgentFenceTest.SafAgentFenceQueryPermissionTest003
 * @tc.desc: Batch query command permission with large batch.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceTest, SafAgentFenceQueryPermissionTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    for (int i = 0; i < 30; i++) {
        cmds.push_back({"cmd_" + std::to_string(i), "sub_" + std::to_string(i)});
    }

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
    EXPECT_EQ(cmdPermissions.size(), cmds.size());
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