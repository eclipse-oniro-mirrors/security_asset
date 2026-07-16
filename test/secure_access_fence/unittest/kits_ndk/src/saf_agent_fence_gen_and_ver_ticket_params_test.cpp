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

#include "saf_agent_fence_gen_and_ver_ticket_params_test.h"

#include <gtest/gtest.h>

#include "saf_agent_fence.h"
#include "saf_result_code.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceGenAndVerTicketParamsTest {

class SafAgentFenceGenAndVerTicketParamsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceGenAndVerTicketParamsTest::SetUpTestCase(void)
{
}

void SafAgentFenceGenAndVerTicketParamsTest::TearDownTestCase(void)
{
}

void SafAgentFenceGenAndVerTicketParamsTest::SetUp(void)
{
}

void SafAgentFenceGenAndVerTicketParamsTest::TearDown(void)
{
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 99;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 0;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    for (size_t i = 0; i < 100; i++) {
        messages.push_back("test_message_" + std::to_string(i));
    }

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    for (size_t i = 0; i < 99; i++) {
        messages.push_back("test_message_" + std::to_string(i));
    }

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchGenerateTicketParamsTest007, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;
    int32_t result = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchVerifyTicketParamsTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    std::vector<int32_t> verifyRes;
    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchVerifyTicketParamsTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 99;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    std::vector<int32_t> verifyRes;
    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchVerifyTicketParamsTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 0;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    std::vector<int32_t> verifyRes;
    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchVerifyTicketParamsTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;

    std::vector<int32_t> verifyRes;
    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchVerifyTicketParamsTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    for (size_t i = 0; i < 100; i++) {
        verifyInfos.push_back({"message", "challenge", "ticket"});
    }

    std::vector<int32_t> verifyRes;
    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceGenAndVerTicketParamsTest, BatchVerifyTicketParamsTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    int32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    std::vector<int32_t> verifyRes;
    int32_t result = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    EXPECT_NE(result, SAF_ERR_SERVICE_UNAVAILABLE);
}

}