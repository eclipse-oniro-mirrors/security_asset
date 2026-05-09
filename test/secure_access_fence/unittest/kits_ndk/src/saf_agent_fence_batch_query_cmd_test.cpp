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

#include "saf_agent_fence_batch_query_cmd_test.h"

#include <gtest/gtest.h>

#include "saf_agent_fence.h"
#include "saf_result_defs.h"
#include "secure_access_fence_system_type.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceBatchQueryCmdTest {

class SafAgentFenceBatchQueryCmdTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceBatchQueryCmdTest::SetUpTestCase(void)
{
}

void SafAgentFenceBatchQueryCmdTest::TearDownTestCase(void)
{
}

void SafAgentFenceBatchQueryCmdTest::SetUp(void)
{
}

void SafAgentFenceBatchQueryCmdTest::TearDown(void)
{
}

HWTEST_F(SafAgentFenceBatchQueryCmdTest, BatchQueryCommandPermissionTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"", "run"});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, 0);
}

HWTEST_F(SafAgentFenceBatchQueryCmdTest, BatchQueryCommandPermissionTest002, TestSize.Level0)
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

HWTEST_F(SafAgentFenceBatchQueryCmdTest, BatchQueryCommandPermissionTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, 0);
}

HWTEST_F(SafAgentFenceBatchQueryCmdTest, BatchQueryCommandPermissionTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::CommandInfo> cmds;
    cmds.push_back({"validCmd", ""});

    std::vector<OHOS::Security::SAF::CommandPermissionInfo> cmdPermissions;

    int32_t result = agentFence.BatchQueryCommandPermission(cmds, cmdPermissions);
    EXPECT_NE(result, SEC_SAF_SUCCESS);
}

}