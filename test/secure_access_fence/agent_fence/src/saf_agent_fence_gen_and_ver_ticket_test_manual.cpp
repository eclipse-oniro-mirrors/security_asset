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

#include <gtest/gtest.h>
#include <fstream>
#include <random>
#include <ctime>

#include "saf_agent_fence.h"
#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"
#include "saf_permission_change.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceManualTest {

constexpr const char* DATA_DIR = "/data/service/el1/public/secure_access_fence/tdd_data";
constexpr size_t MESSAGE_COUNT = 5;
constexpr int MIN_MESSAGE_LEN = 5;
constexpr int MAX_MESSAGE_LEN = 100;

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

static bool RemoveDir(const std::string& path)
{
    std::string cmd = "rm -rf " + path;
    return system(cmd.c_str()) == 0;
}

static bool CreateDir(const std::string& path)
{
    std::string cmd = "mkdir -p " + path;
    return system(cmd.c_str()) == 0;
}

static bool WriteTicketInfoToFile(const std::string& filePath, const OHOS::Security::SAF::VerifyTicketInfo& info)
{
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs.is_open()) {
        return false;
    }

    uint32_t msgLen = static_cast<uint32_t>(info.message.length());
    uint32_t challengeLen = static_cast<uint32_t>(info.challenge.length());
    uint32_t ticketLen = static_cast<uint32_t>(info.ticket.length());

    ofs.write(reinterpret_cast<const char*>(&msgLen), sizeof(msgLen));
    ofs.write(info.message.c_str(), msgLen);
    ofs.write(reinterpret_cast<const char*>(&challengeLen), sizeof(challengeLen));
    ofs.write(info.challenge.c_str(), challengeLen);
    ofs.write(reinterpret_cast<const char*>(&ticketLen), sizeof(ticketLen));
    ofs.write(info.ticket.c_str(), ticketLen);

    ofs.close();
    return ofs.good();
}

static bool ReadTicketInfoFromFile(const std::string& filePath, OHOS::Security::SAF::VerifyTicketInfo& info)
{
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }

    uint32_t msgLen = 0;
    ifs.read(reinterpret_cast<char*>(&msgLen), sizeof(msgLen));
    if (!ifs.good()) {
        ifs.close();
        return false;
    }
    info.message.resize(msgLen);
    ifs.read(&info.message[0], msgLen);

    uint32_t challengeLen = 0;
    ifs.read(reinterpret_cast<char*>(&challengeLen), sizeof(challengeLen));
    if (!ifs.good()) {
        ifs.close();
        return false;
    }
    info.challenge.resize(challengeLen);
    ifs.read(&info.challenge[0], challengeLen);

    uint32_t ticketLen = 0;
    ifs.read(reinterpret_cast<char*>(&ticketLen), sizeof(ticketLen));
    if (!ifs.good()) {
        ifs.close();
        return false;
    }
    info.ticket.resize(ticketLen);
    ifs.read(&info.ticket[0], ticketLen);

    ifs.close();
    return true;
}

class SafAgentFenceGenAndVerTicketTestManual : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    static std::vector<OHOS::Security::SAF::VerifyTicketInfo> savedTicketInfos;
};

std::vector<OHOS::Security::SAF::VerifyTicketInfo> SafAgentFenceGenAndVerTicketTestManual::savedTicketInfos;

void SafAgentFenceGenAndVerTicketTestManual::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void SafAgentFenceGenAndVerTicketTestManual::TearDownTestCase(void)
{
}

void SafAgentFenceGenAndVerTicketTestManual::SetUp(void)
{
}

void SafAgentFenceGenAndVerTicketTestManual::TearDown(void)
{
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTestManual.TestCase001_GenerateTicket
 * @tc.desc: Generate ticket with osAccountId:100, callerId:"test_caller", message count:5, message length:5-100
 * @tc.type: FUNC
 * @tc.result: Generate success, write to file success
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTestManual, TestCase001_GenerateTicket, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(MESSAGE_COUNT, MIN_MESSAGE_LEN, MAX_MESSAGE_LEN);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    std::cout << "GenerateTicket return code: " << genResult << std::endl;
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), MESSAGE_COUNT);

    std::cout << "Generated ticket infos:" << std::endl;
    for (size_t i = 0; i < ticketInfos.size(); i++) {
        std::cout << "  [" << i << "] message length: " << ticketInfos[i].message.length()
                  << ", challenge length: " << ticketInfos[i].challenge.length()
                  << ", ticket length: " << ticketInfos[i].ticket.length() << std::endl;
    }

    RemoveDir(DATA_DIR);
    ASSERT_TRUE(CreateDir(DATA_DIR));

    for (size_t i = 0; i < ticketInfos.size(); i++) {
        std::string filePath = std::string(DATA_DIR) + "/ticket_" + std::to_string(i) + ".dat";
        ASSERT_TRUE(WriteTicketInfoToFile(filePath, ticketInfos[i]));
        std::cout << "Written: " << filePath << std::endl;
    }

    savedTicketInfos = ticketInfos;
    std::cout << "TestCase 1 completed successfully." << std::endl;
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTestManual.TestCase002_VerifyTicket
 * @tc.desc: Read ticket info from file and verify ticket
 * @tc.type: FUNC
 * @tc.result: Verify success
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTestManual, TestCase002_VerifyTicket, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 100;
    std::string callerId = "test_caller";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    for (size_t i = 0; i < MESSAGE_COUNT; i++) {
        std::string filePath = std::string(DATA_DIR) + "/ticket_" + std::to_string(i) + ".dat";
        OHOS::Security::SAF::VerifyTicketInfo info;
        ASSERT_TRUE(ReadTicketInfoFromFile(filePath, info));
        verifyInfos.push_back(info);
        std::cout << "Read: " << filePath << std::endl;
    }

    std::cout << "VerifyTicket parameters:" << std::endl;
    std::cout << "  osAccountId: " << osAccountId << std::endl;
    std::cout << "  callerId: " << callerId << std::endl;
    std::cout << "  verifyInfos count: " << verifyInfos.size() << std::endl;

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    std::cout << "VerifyTicket return code: " << verifyResult << std::endl;
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), MESSAGE_COUNT);

    std::cout << "Verification results:" << std::endl;
    for (size_t i = 0; i < verifyRes.size(); i++) {
        std::cout << "  [" << i << "] result: " << verifyRes[i] << std::endl;
        EXPECT_EQ(verifyRes[i], SEC_SAF_SUCCESS);
    }

    std::cout << "TestCase 2 completed." << std::endl;
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTestManual.TestCase003_GenerateTicket
 * @tc.desc: Generate ticket with osAccountId:101, callerId:"test_caller", message count:5, message length:5-100
 * @tc.type: FUNC
 * @tc.result: Generate success, write to file success
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTestManual, TestCase003_GenerateTicket, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 101;
    std::string callerId = "test_caller";

    std::vector<std::string> messages = GenerateMessages(MESSAGE_COUNT, MIN_MESSAGE_LEN, MAX_MESSAGE_LEN);
    std::vector<OHOS::Security::SAF::VerifyTicketInfo> ticketInfos;

    int32_t genResult = agentFence.BatchGenerateTicket(osAccountId, callerId, messages, ticketInfos);
    std::cout << "GenerateTicket return code: " << genResult << std::endl;
    EXPECT_EQ(genResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(ticketInfos.size(), MESSAGE_COUNT);

    std::cout << "Generated ticket infos:" << std::endl;
    for (size_t i = 0; i < ticketInfos.size(); i++) {
        std::cout << "  [" << i << "] message length: " << ticketInfos[i].message.length()
                  << ", challenge length: " << ticketInfos[i].challenge.length()
                  << ", ticket length: " << ticketInfos[i].ticket.length() << std::endl;
    }

    RemoveDir(DATA_DIR);
    ASSERT_TRUE(CreateDir(DATA_DIR));

    for (size_t i = 0; i < ticketInfos.size(); i++) {
        std::string filePath = std::string(DATA_DIR) + "/ticket_" + std::to_string(i) + ".dat";
        ASSERT_TRUE(WriteTicketInfoToFile(filePath, ticketInfos[i]));
        std::cout << "Written: " << filePath << std::endl;
    }

    savedTicketInfos = ticketInfos;
    std::cout << "TestCase 3 completed successfully." << std::endl;
}

/**
 * @tc.name: SafAgentFenceGenAndVerTicketTestManual.TestCase004_VerifyTicket
 * @tc.desc: Read ticket info from file and verify ticket with osAccountId:101
 * @tc.type: FUNC
 * @tc.result: Verify success
 */
HWTEST_F(SafAgentFenceGenAndVerTicketTestManual, TestCase004_VerifyTicket, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    uint32_t osAccountId = 101;
    std::string callerId = "test_caller";

    std::vector<OHOS::Security::SAF::VerifyTicketInfo> verifyInfos;
    for (size_t i = 0; i < MESSAGE_COUNT; i++) {
        std::string filePath = std::string(DATA_DIR) + "/ticket_" + std::to_string(i) + ".dat";
        OHOS::Security::SAF::VerifyTicketInfo info;
        ASSERT_TRUE(ReadTicketInfoFromFile(filePath, info));
        verifyInfos.push_back(info);
        std::cout << "Read: " << filePath << std::endl;
    }

    std::cout << "VerifyTicket parameters:" << std::endl;
    std::cout << "  osAccountId: " << osAccountId << std::endl;
    std::cout << "  callerId: " << callerId << std::endl;
    std::cout << "  verifyInfos count: " << verifyInfos.size() << std::endl;

    std::vector<int32_t> verifyRes;
    int32_t verifyResult = agentFence.BatchVerifyTicket(osAccountId, callerId, verifyInfos, verifyRes);
    std::cout << "VerifyTicket return code: " << verifyResult << std::endl;
    EXPECT_EQ(verifyResult, SEC_SAF_SUCCESS);
    EXPECT_EQ(verifyRes.size(), MESSAGE_COUNT);

    std::cout << "Verification results:" << std::endl;
    for (size_t i = 0; i < verifyRes.size(); i++) {
        std::cout << "  [" << i << "] result: " << verifyRes[i] << std::endl;
        EXPECT_EQ(verifyRes[i], SEC_SAF_SUCCESS);
    }

    std::cout << "TestCase 4 completed." << std::endl;
}

}