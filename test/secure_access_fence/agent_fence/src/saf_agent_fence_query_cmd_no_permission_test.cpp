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
#include "secure_access_fence_system_type.h"
#include "saf_permission_change.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceTest {
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
    EXPECT_NE(result, SEC_SAF_SUCCESS);
}

}