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

#include "saf_agent_params_checker_test.h"

#include <gtest/gtest.h>

#include "saf_agent_params_checker.h"
#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"

using namespace testing::ext;
namespace UnitTest::SafAgentParamsCheckerTest {

class SafAgentParamsCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentParamsCheckerTest::SetUpTestCase(void)
{
}

void SafAgentParamsCheckerTest::TearDownTestCase(void)
{
}

void SafAgentParamsCheckerTest::SetUp(void)
{
}

void SafAgentParamsCheckerTest::TearDown(void)
{
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest001, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest002, TestSize.Level0)
{
    uint32_t osAccountId = 99;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest003, TestSize.Level0)
{
    uint32_t osAccountId = 0;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest004, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest005, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    for (size_t i = 0; i < 100; i++) {
        messages.push_back("test_message_" + std::to_string(i));
    }

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest006, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    for (size_t i = 0; i < 99; i++) {
        messages.push_back("test_message_" + std::to_string(i));
    }

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsTest007, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<std::string> messages;
    messages.push_back("test_message");

    int32_t result = OHOS::Security::SAF::CheckBatchGenerateTicketParams(osAccountId, callerId, messages);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest001, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest002, TestSize.Level0)
{
    uint32_t osAccountId = 99;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest003, TestSize.Level0)
{
    uint32_t osAccountId = 0;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest004, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest005, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    for (size_t i = 0; i < 100; i++) {
        verifyInfos.push_back({"message", "challenge", "ticket"});
    }

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest006, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    for (size_t i = 0; i < 99; i++) {
        verifyInfos.push_back({"message", "challenge", "ticket"});
    }

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsTest007, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    verifyInfos.push_back({"message", "challenge", "ticket"});

    int32_t result = OHOS::Security::SAF::CheckBatchVerifyTicketParams(osAccountId, callerId, verifyInfos);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest001, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = nullptr;
    size_t messagesCount = 1;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest002, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "";
    size_t messagesCount = 1;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest003, TestSize.Level0)
{
    uint32_t osAccountId = 99;
    const char* callerId = "test_caller";
    size_t messagesCount = 1;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest004, TestSize.Level0)
{
    uint32_t osAccountId = 0;
    const char* callerId = "test_caller";
    size_t messagesCount = 1;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest005, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t messagesCount = 0;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest006, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t messagesCount = 100;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest007, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t messagesCount = 99;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchGenerateTicketParamsCTest008, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t messagesCount = 1;

    int32_t result = CheckBatchGenerateTicketParamsC(osAccountId, callerId, messagesCount);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest001, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = nullptr;
    size_t verifyInfosCount = 1;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest002, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "";
    size_t verifyInfosCount = 1;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest003, TestSize.Level0)
{
    uint32_t osAccountId = 99;
    const char* callerId = "test_caller";
    size_t verifyInfosCount = 1;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest004, TestSize.Level0)
{
    uint32_t osAccountId = 0;
    const char* callerId = "test_caller";
    size_t verifyInfosCount = 1;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest005, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t verifyInfosCount = 0;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest006, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t verifyInfosCount = 100;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_PARAM_VERICATION_FAILED);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest007, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t verifyInfosCount = 99;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

HWTEST_F(SafAgentParamsCheckerTest, CheckBatchVerifyTicketParamsCTest008, TestSize.Level0)
{
    uint32_t osAccountId = 100;
    const char* callerId = "test_caller";
    size_t verifyInfosCount = 1;

    int32_t result = CheckBatchVerifyTicketParamsC(osAccountId, callerId, verifyInfosCount);
    EXPECT_EQ(result, SEC_SAF_SUCCESS);
}

}