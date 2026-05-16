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

#include "saf_agent_fence_no_permission_test.h"

#include <gtest/gtest.h>

#include "saf_agent_fence.h"
#include "saf_result_defs.h"
#include "saf_permission_change.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceTest {
static std::string GenerateRandomMessage(int minLen, int maxLen)
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));
    static std::uniform_int_distribution<int> lenDist(minLen, maxLen);
    static std::uniform_int_distribution<int> charDist(0, sizeof(charset) - 2);

    int len = lenDist(rng);
    std::string result;
    result.reserve(len);
    for (int i = 0; i < len; i++) {
        result += charset[charDist(rng)];
    }
    return result;
}

static std::vector<std::string> GenerateMessages(int count, int minLen, int maxLen)
{
    std::vector<std::string> messages;
    for (int i = 0; i < count; i++) {
        messages.push_back(GenerateRandomMessage(minLen, maxLen));
    }
    return messages;
}

class SafAgentFenceNoPermissionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceNoPermissionTest::SetUpTestCase(void)
{
}

void SafAgentFenceNoPermissionTest::TearDownTestCase(void)
{
}

void SafAgentFenceNoPermissionTest::SetUp(void)
{
}

void SafAgentFenceNoPermissionTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceNoPermissionTest.SafAgentFenceQueryNoPermissionTest001
 * @tc.desc: Batch query command permission with "ohos-cliTimer" and empty subcommand.
 * @tc.type: FUNC
 * @tc.result: 0
 */
HWTEST_F(SafAgentFenceNoPermissionTest, SafAgentFenceQueryNoPermissionTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"ohos-cliTimer", ""});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, SAF_SUCCESS);
}


/**
 * @tc.name: SafAgentFenceNoPermissionTest.SafAgentFenceGenAndVerTicketTest002
 * @tc.desc: osAccountId:100; callerId:"test_caller"; message列表长度:5; message长度:5-100随机选; 无权限
 * @tc.type: FUNC
 * @tc.result: 生成失败
 */
HWTEST_F(SafAgentFenceNoPermissionTest, SafAgentFenceGenAndVerTicketTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(genResult, SAF_SUCCESS);
}

}