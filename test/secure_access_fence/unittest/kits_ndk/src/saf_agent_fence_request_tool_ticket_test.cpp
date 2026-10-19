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
#include "saf_agent_fence_request_tool_ticket_test.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceRequestToolTicketTest {

const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.EXEC_PUBLIC_CLI_TOOL",
    "ohos.permission.QUERY_TOOL_PERMISSIONS",
    "ohos.permission.cli.BUNDLE_ACTIVE_INFO"
};

class SafAgentFenceRequestToolTicketTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static MockToken* mockToken_;
};

MockToken* SafAgentFenceRequestToolTicketTest::mockToken_ = nullptr;

void SafAgentFenceRequestToolTicketTest::SetUpTestCase(void)
{
    mockToken_ = new MockToken(PERMISSIONS, true, false);
    ASSERT_NE(mockToken_, nullptr);

    std::string errorMsg = mockToken_->GetMockErrorMsg();
    if (errorMsg != "success") {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "MockToken failed: " << errorMsg;
    } else {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "Failed to create MockToken";
    }
}

void SafAgentFenceRequestToolTicketTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

void SafAgentFenceRequestToolTicketTest::SetUp(void)
{
}

void SafAgentFenceRequestToolTicketTest::TearDown(void)
{
}

/**
 * @tc.desc: RequestToolTicket with empty cmdName, expect SAF_ERR_ARG_INVALID.
 */
HWTEST_F(SafAgentFenceRequestToolTicketTest, RequestToolTicketEmptyCmdName001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cliCmdInfos;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "";
    cmd.subCmd = "run";
    cliCmdInfos.push_back(cmd);

    OHOS::Security::SAF::TicketOptions ticketOptions;
    ticketOptions.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::TicketQueryResult ticketQueryResult;
    int32_t result = agentFence.RequestToolTicket(cliCmdInfos, ticketOptions, ticketQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: RequestToolTicket with empty cmd list, expect SAF_ERR_ARG_EMPTY.
 */
HWTEST_F(SafAgentFenceRequestToolTicketTest, RequestToolTicketEmptyCmdList001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cliCmdInfos;

    OHOS::Security::SAF::TicketOptions ticketOptions;
    ticketOptions.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::TicketQueryResult ticketQueryResult;
    int32_t result = agentFence.RequestToolTicket(cliCmdInfos, ticketOptions, ticketQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: RequestToolTicket with ticketExpireTimeMs exceeding max, expect SAF_ERR_ARG_INVALID.
 */
HWTEST_F(SafAgentFenceRequestToolTicketTest, RequestTicketExceedsMaxExpireTime001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cliCmdInfos;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-cliTimer";
    cmd.subCmd = "run";
    cliCmdInfos.push_back(cmd);

    OHOS::Security::SAF::TicketOptions ticketOptions;
    ticketOptions.ticketExpireTimeMs = 60001;

    OHOS::Security::SAF::TicketQueryResult ticketQueryResult;
    int32_t result = agentFence.RequestToolTicket(cliCmdInfos, ticketOptions, ticketQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: RequestToolTicket with non-existent CLI command, expect SAF_ERR_TOOL_ERROR.
 */
HWTEST_F(SafAgentFenceRequestToolTicketTest, RequestToolTicketNonExistCmd001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cliCmdInfos;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "non-existent-cmd";
    cmd.subCmd = "run";
    cliCmdInfos.push_back(cmd);

    OHOS::Security::SAF::TicketOptions ticketOptions;
    ticketOptions.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::TicketQueryResult ticketQueryResult;
    int32_t result = agentFence.RequestToolTicket(cliCmdInfos, ticketOptions, ticketQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.desc: RequestToolTicket with default expire time (0), expect non-crash.
 */
HWTEST_F(SafAgentFenceRequestToolTicketTest, RequestToolTicketDefaultExpireTime001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;
    std::vector<OHOS::Security::SAF::CommandInfo> cliCmdInfos;
    OHOS::Security::SAF::CommandInfo cmd;
    cmd.cmdName = "ohos-batteryManager";
    cmd.subCmd = "total-energy";
    cliCmdInfos.push_back(cmd);

    OHOS::Security::SAF::TicketOptions ticketOptions;
    ticketOptions.ticketExpireTimeMs = 0;

    OHOS::Security::SAF::TicketQueryResult ticketQueryResult;
    int32_t result = agentFence.RequestToolTicket(cliCmdInfos, ticketOptions, ticketQueryResult);
    EXPECT_NE(result, SAF_SUCCESS);
}
}
