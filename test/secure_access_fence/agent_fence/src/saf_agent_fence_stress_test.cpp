/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at the following address:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <random>
#include <ctime>

#include "saf_agent_fence.h"
#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"
#include "saf_permission_change.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceStressTest {

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

class SafAgentFenceStressTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceStressTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceStressTest::TearDownTestCase(void)
{
}

void SafAgentFenceStressTest::SetUp(void)
{
}

void SafAgentFenceStressTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceStressTest.SafAgentFenceStressTest001
 * @tc.desc: 压测用例：message列表长度:99; 且循环调用100次
 * @tc.type: FUNC
 * @tc.result: 生成和验证成功
 */
HWTEST_F(SafAgentFenceStressTest, SafAgentFenceStressTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    for (int loop = 0; loop < 100; loop++) {
        std::vector<std::string> messages = GenerateMessages(99, 5, 100);
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

/**
 * @tc.name: SafAgentFenceStressTest.SafAgentFenceStressTest002
 * @tc.desc: 反向用例：verifyInfo列表长度:100
 * @tc.type: FUNC
 * @tc.result: 验证失败
 */
HWTEST_F(SafAgentFenceStressTest, SafAgentFenceStressTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(99, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    ticketInfos.push_back({"extra_message", "challenge", "ticket"});

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_NE(verifyResult, SEC_SAF_SUCCESS);
}

}