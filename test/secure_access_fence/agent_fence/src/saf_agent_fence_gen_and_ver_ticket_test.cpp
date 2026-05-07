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
#include <random>
#include <ctime>

#include "saf_agent_fence.h"
#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"
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

class SafAgentFenceGenAndVerTicketTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceGenAndVerTicketTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceGenAndVerTicketTest::TearDownTestCase(void)
{
}

void SafAgentFenceGenAndVerTicketTest::SetUp(void)
{
}

void SafAgentFenceGenAndVerTicketTest::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest001
 * @tc.desc: 基础用例：osAccountId:100; callerId:"test_caller"; message列表长度:5; message长度:5-100随机选
 * @tc.type: FUNC
 * @tc.result: 生成和验证成功
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
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

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest002
 * @tc.desc: 反向用例：osAccountId:101
 * @tc.type: FUNC
 * @tc.result: 生成和验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 101;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(genResult, SEC_SAF_SUCCESS);

    std::vector<int32_t> verifyRes;
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"test_message", "challenge", "ticket"});
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(verifyResult, SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest003
 * @tc.desc: 反向用例：osAccountId:0
 * @tc.type: FUNC
 * @tc.result: 生成失败；验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 0;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(genResult, SEC_SAF_SUCCESS);

    std::vector<int32_t> verifyRes;
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"test_message", "challenge", "ticket"});
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(verifyResult, SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest004
 * @tc.desc: 反向用例：osAccountId:99
 * @tc.type: FUNC
 * @tc.result: 生成失败；验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 99;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(genResult, SEC_SAF_SUCCESS);

    std::vector<int32_t> verifyRes;
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"test_message", "challenge", "ticket"});
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(verifyResult, SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest005
 * @tc.desc: 反向用例：message列表长度:0
 * @tc.type: FUNC
 * @tc.result: 生成失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages;
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(genResult, SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest006
 * @tc.desc: 反向用例：message列表长度:100
 * @tc.type: FUNC
 * @tc.result: 生成失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(100, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(genResult, SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest007
 * @tc.desc: 反向用例：message列表中存在部分message长度为0
 * @tc.type: FUNC
 * @tc.result: 生成和验证接口调用成功，但message为0的元素，生成的ticket也为0，且该元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest007, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    messages[0] = "";
    messages[2] = "";
    messages[4] = "";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), messages.size());
    EXPECT_TRUE(ticketInfos[0].ticket.empty());
    EXPECT_EQ(ticketInfos[1].ticket.size(), 44);
    EXPECT_TRUE(ticketInfos[2].ticket.empty());
    EXPECT_EQ(ticketInfos[3].ticket.size(), 44);
    EXPECT_TRUE(ticketInfos[4].ticket.empty());

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_NE(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest008
 * @tc.desc: 反向用例：message列表中存在所有message长度为0
 * @tc.type: FUNC
 * @tc.result: 生成和验证接口调用成功，但message为0的元素，生成的ticket也为0，且该元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest008, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages(5, "");

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), messages.size());
    for (size_t i = 0; i < ticketInfos.size(); i++) {
        EXPECT_TRUE(ticketInfos[i].ticket.empty());
    }

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    for (size_t i = 0; i < verifyRes.size(); i++) {
        EXPECT_NE(verifyRes[i], SEC_SAF_SUCCESS);
    }
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest009
 * @tc.desc: 反向用例：verifyInfo列表长度:0
 * @tc.type: FUNC
 * @tc.result: 验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest009, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    std::vector<int32_t> verifyRes;

    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(verifyResult, SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest010
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中3个元素的message互换后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被互换的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest010, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    std::string temp = ticketInfos[0].message;
    ticketInfos[0].message = ticketInfos[2].message;
    ticketInfos[2].message = ticketInfos[4].message;
    ticketInfos[4].message = temp;

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_NE(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest011
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中2个元素的challenge互换后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被互换的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest011, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    std::string temp = ticketInfos[1].challenge;
    ticketInfos[1].challenge = ticketInfos[3].challenge;
    ticketInfos[3].challenge = temp;

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest012
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中2个元素的ticket互换后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被互换的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest012, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    std::string temp = ticketInfos[1].ticket;
    ticketInfos[1].ticket = ticketInfos[3].ticket;
    ticketInfos[3].ticket = temp;

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest013
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中1个元素的message篡改后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被篡改的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest013, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    ticketInfos[2].message = "tampered_message";

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest014
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中1个元素的challenge长度改长后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被篡改的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest014, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    ticketInfos[2].challenge += "extra_data_to_make_it_longer";

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest015
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中1个元素的challenge长度改短后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被篡改的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest015, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    if (ticketInfos[2].challenge.length() > 5) {
        ticketInfos[2].challenge = ticketInfos[2].challenge.substr(0, ticketInfos[2].challenge.length() - 5);
    } else {
        ticketInfos[2].challenge = "";
    }

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest016
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中1个元素的ticket长度改长后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被篡改的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest016, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    ticketInfos[2].ticket += "extra_data_to_make_it_longer";

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest017
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中其中1个元素的ticket长度改短后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被篡改的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest017, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    if (ticketInfos[2].ticket.length() > 5) {
        ticketInfos[2].ticket = ticketInfos[2].ticket.substr(0, ticketInfos[2].ticket.length() - 5);
    } else {
        ticketInfos[2].ticket = "";
    }

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    EXPECT_EQ(verifyRes[0], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[1], SEC_SAF_SUCCESS);
    EXPECT_NE(verifyRes[2], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[3], SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes[4], SEC_SAF_SUCCESS);
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTest.SafAgentFenceGenAndVerTicketTest018
 * @tc.desc: 反向用例：生成后，将verifyInfo列表中每个元素的message、challenge或ticket篡改后，验证
 * @tc.type: FUNC
 * @tc.result: 生成成功，验证接口调用成功，但被篡改的元素会验证失败
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTest, SafAgentFenceGenAndVerTicketTest018, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(5, 5, 100);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);

    ticketInfos[0].message = "tampered_message_0";
    ticketInfos[1].challenge = "tampered_challenge_1";
    ticketInfos[2].ticket = "tampered_ticket_2";
    ticketInfos[3].message = "tampered_message_3";
    ticketInfos[4].challenge = "tampered_challenge_4";

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, ticketInfos, verifyRes);
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), ticketInfos.size());
    for (size_t i = 0; i < verifyRes.size(); i++) {
        EXPECT_NE(verifyRes[i], SEC_SAF_SUCCESS);
    }
}

}