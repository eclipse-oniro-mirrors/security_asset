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

#include "saf_agent_fence_task_identity_ndk_test.h"

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>

#include "saf_agent_fence.h"
#include "saf_result_code.h"

#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "access_token.h"
#include "access_token_basic_type.h"
#include "permission_state_full.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceTaskIdentityNdkTest {

// Holds the system HAP token (userID=100) allocated in GrantTestPermission.
static int32_t g_testHapTokenId = 0;

class SafAgentFenceTaskIdentityNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

// Creates a system HAP token with userID=100 (osAccountId=100) and grants
// ohos.permission.QUERY_TOOL_PERMISSIONS, then switches the calling identity to
// it. A separate system_core native token with MANAGE_HAP_TOKENID is created
// first so that InitHapToken passes its VerifyAccessToken check.
static void GrantTestPermission()
{
    // Step 1: native token with MANAGE_HAP_TOKENID (system_core APL required).
    const char *hapManagePerms[] = { "ohos.permission.MANAGE_HAP_TOKENID" };
    NativeTokenInfoParams manageInfo = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = hapManagePerms,
        .acls = nullptr,
        .processName = "saf_ndk_task_identity_test",
        .aplStr = "system_core",
    };
    uint64_t manageTokenId = GetAccessTokenId(&manageInfo);
    SetSelfTokenID(manageTokenId);

    // Step 2: allocate a system HAP token with userID=100.
    HapInfoParams info = {
        .userID = 100,
        .bundleName = "saf_ndk_task_identity_test",
        .instIndex = 0,
        .appIDDesc = "saf_ndk_task_identity_test",
        .apiVersion = 11,
        .isSystemApp = true,
    };
    PermissionStateFull permState = {
        .permissionName = "ohos.permission.QUERY_TOOL_PERMISSIONS",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {PERMISSION_GRANTED},
        .grantFlags = {PERMISSION_DEFAULT_FLAG},
    };
    HapPolicyParams policy = {
        .apl = APL_SYSTEM_BASIC,
        .domain = "saf_ndk_task_identity_test",
        .permStateList = {permState},
    };
    AccessTokenIDEx tokenIdEx = {0};
    ASSERT_EQ(RET_SUCCESS, AccessTokenKit::InitHapToken(info, policy, tokenIdEx));
    ASSERT_NE(INVALID_TOKENID, tokenIdEx.tokenIdExStruct.tokenID);
    g_testHapTokenId = static_cast<int32_t>(tokenIdEx.tokenIdExStruct.tokenID);
    SetSelfTokenID(tokenIdEx.tokenIDEx);
}

void SafAgentFenceTaskIdentityNdkTest::SetUpTestCase(void)
{
    GrantTestPermission();
}

void SafAgentFenceTaskIdentityNdkTest::TearDownTestCase(void)
{
}

void SafAgentFenceTaskIdentityNdkTest::SetUp(void)
{
}

void SafAgentFenceTaskIdentityNdkTest::TearDown(void)
{
}

static TaskIdentityInfo MakeLowRiskInfo(const std::string &taskId, int32_t expireTime)
{
    TaskIdentityInfo info = {};
    info.taskId = taskId;
    info.taskExpireTime = expireTime;
    info.executeType = ExecuteType::INSTANT;
    info.allowScreenLockExecution = false;
    info.allowableDuration = 0;
    return info;
}

static TaskIdentityInfo MakeDelayedInfo(const std::string &taskId, int32_t expireTime)
{
    TaskIdentityInfo info = {};
    info.taskId = taskId;
    info.taskExpireTime = expireTime;
    info.executeType = ExecuteType::DELAYED;
    info.allowScreenLockExecution = false;
    info.allowableDuration = 0;
    return info;
}

// ==================== GenerateTaskChallenge ====================

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, GenerateTaskChallengeTest001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_challenge_001", 3600);

    std::vector<int32_t> challenge;
    int32_t result = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    std::vector<std::string> taskIds = {"ndk_challenge_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, GenerateTaskChallengeTest002, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("ndk_challenge_002", -1);

    std::vector<int32_t> challenge;
    int32_t result = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    std::vector<std::string> taskIds = {"ndk_challenge_002"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, GenerateTaskChallengeTest003, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_challenge_003", 3600);

    std::vector<int32_t> challenge1;
    int32_t result1 = SafAgentFence::GenerateTaskChallenge(info, challenge1);
    EXPECT_EQ(result1, SAF_SUCCESS);
    EXPECT_EQ(challenge1.size(), 32u);

    std::vector<int32_t> challenge2;
    int32_t result2 = SafAgentFence::GenerateTaskChallenge(info, challenge2);
    EXPECT_EQ(result2, SAF_SUCCESS);
    EXPECT_EQ(challenge2.size(), 32u);

    std::vector<std::string> taskIds = {"ndk_challenge_003"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== GenerateTaskIdentity ====================

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, GenerateTaskIdentityTest001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_identity_001", 3600);

    TaskIdentity identity;
    int32_t result = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_FALSE(identity.taskAttributes.empty());
    EXPECT_FALSE(identity.identity.empty());
    EXPECT_FALSE(identity.challenge.empty());

    std::vector<std::string> taskIds = {"ndk_identity_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, GenerateTaskIdentityTest002, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("ndk_identity_002", -1);

    TaskIdentity identity;
    int32_t result = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_FALSE(identity.taskAttributes.empty());
    EXPECT_FALSE(identity.identity.empty());
    EXPECT_FALSE(identity.challenge.empty());

    std::vector<std::string> taskIds = {"ndk_identity_002"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, GenerateTaskIdentityTest003, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_identity_003", 3600);

    TaskIdentity identity1;
    int32_t result1 = SafAgentFence::GenerateTaskIdentity(info, identity1);
    EXPECT_EQ(result1, SAF_SUCCESS);
    EXPECT_FALSE(identity1.identity.empty());

    TaskIdentity identity2;
    int32_t result2 = SafAgentFence::GenerateTaskIdentity(info, identity2);
    EXPECT_EQ(result2, SAF_SUCCESS);
    EXPECT_FALSE(identity2.identity.empty());

    std::vector<std::string> taskIds = {"ndk_identity_003"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== RevokeTaskIdentity ====================

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RevokeTaskIdentityTest001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_revoke_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"ndk_revoke_001"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<int32_t> authToken;
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_revoke_001", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RevokeTaskIdentityTest002, TestSize.Level0)
{
    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("ndk_revoke_002a", 3600), identity), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("ndk_revoke_002b", 3600), identity), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("ndk_revoke_002c", 3600), identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"ndk_revoke_002a", "ndk_revoke_002b", "ndk_revoke_002c"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<int32_t> authToken;
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_revoke_002a", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_revoke_002b", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_revoke_002c", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RevokeTaskIdentityTest003, TestSize.Level0)
{
    std::vector<std::string> taskIds = {"ndk_revoke_nonexistent_003"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RevokeTaskIdentityTest004, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_revoke_twice_004", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"ndk_revoke_twice_004"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);
}

// ==================== RetrieveAuthTokenByTaskId ====================

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RetrieveAuthTokenByTaskIdTest001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_retrieve_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_retrieve_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_TRUE(authToken.empty());

    std::vector<std::string> taskIds = {"ndk_retrieve_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RetrieveAuthTokenByTaskIdTest002, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_retrieve_002", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"ndk_retrieve_002"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_retrieve_002", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
}

HWTEST_F(SafAgentFenceTaskIdentityNdkTest, RetrieveAuthTokenByTaskIdTest003, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("ndk_retrieve_003", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    int32_t wrongCallerToken = g_testHapTokenId + 1;
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(wrongCallerToken, "ndk_retrieve_003", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);

    result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "ndk_retrieve_003", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<std::string> taskIds = {"ndk_retrieve_003"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

}
